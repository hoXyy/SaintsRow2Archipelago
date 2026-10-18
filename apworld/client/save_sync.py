from __future__ import annotations

from dataclasses import dataclass

from CommonClient import logger


@dataclass(frozen=True)
class Inventory:
    """The items AP has sent this slot, in order, by name."""

    names: list[str]
    persistent_names: frozenset[str]

    def contains_cursor(self, cursor: int) -> bool:
        return 0 <= cursor <= len(self.names)

    def persistent_before(self, cursor: int) -> list[str]:
        """Persistent items among the first `cursor` received items."""
        return sorted(
            {name for name in self.names[:cursor] if name in self.persistent_names}
        )


@dataclass(frozen=True)
class SaveContext:
    next_index: int
    persistent_items: list[str]
    # The client ledger doesn't have this cursor yet and should store it.
    persist: bool
    from_server: bool = False


def resolve_save_context(
    checksum: int,
    plugin_cursor: int | None,
    plugin_items: list[str],
    local_cursor: int | None,
    remote_cursor: int | None,
    inventory: Inventory,
) -> SaveContext | None:
    """Pick the cursor for a loaded save, or return None to reject it.

    `plugin_cursor` is None when the plugin's journal doesn't know the save.
    """
    if plugin_cursor is not None:
        return _from_plugin(
            checksum, plugin_cursor, plugin_items, local_cursor, inventory
        )

    if plugin_items:
        logger.error(
            f"Rejected unknown plugin snapshot with persistent items "
            f"checksum={checksum:08X}"
        )
        return None
    if local_cursor is not None:
        return _from_client(checksum, local_cursor, inventory)
    return _from_server(checksum, remote_cursor, inventory)


def _from_plugin(
    checksum: int,
    cursor: int,
    plugin_items: list[str],
    local_cursor: int | None,
    inventory: Inventory,
) -> SaveContext | None:
    if not inventory.contains_cursor(cursor):
        logger.error(
            f"Rejected plugin save snapshot cursor checksum={checksum:08X} "
            f"next_index={cursor}"
        )
        return None
    expected_items = inventory.persistent_before(cursor)
    if sorted(plugin_items) != expected_items:
        logger.error(
            f"Rejected plugin save snapshot with conflicting persistent items "
            f"checksum={checksum:08X}"
        )
        return None
    if local_cursor is not None and local_cursor != cursor:
        logger.error(
            f"Conflicting SR2 save snapshot cursor checksum={checksum:08X}: "
            f"plugin={cursor} client={local_cursor}"
        )
        return None
    return SaveContext(cursor, expected_items, persist=local_cursor is None)


def _from_client(
    checksum: int, cursor: int, inventory: Inventory
) -> SaveContext | None:
    if not inventory.contains_cursor(cursor):
        logger.error(
            f"Rejected client save snapshot cursor checksum={checksum:08X} "
            f"next_index={cursor}"
        )
        return None
    return SaveContext(cursor, inventory.persistent_before(cursor), persist=False)


def _from_server(
    checksum: int, cursor: int | None, inventory: Inventory
) -> SaveContext:
    # Neither local copy knows this save. Use the server mirror if it has a
    # usable cursor, otherwise treat it as a brand new save.
    if cursor is not None and not inventory.contains_cursor(cursor):
        logger.warning(
            f"Ignored server save revision cursor checksum={checksum:08X} "
            f"next_index={cursor}"
        )
        cursor = None
    if cursor is None:
        return SaveContext(0, [], persist=True)
    return SaveContext(
        cursor, inventory.persistent_before(cursor), persist=True, from_server=True
    )


def accept_save_revision(
    checksum: int, next_index: int, local_cursor: int | None, item_count: int
) -> bool:
    """Check a cursor the plugin wants to record after the game saved."""
    if not (0 <= checksum <= 0xFFFFFFFF and 0 <= next_index <= item_count):
        logger.error(
            f"Rejected SR2 save revision checksum={checksum} next_index={next_index}"
        )
        return False
    if local_cursor is not None and local_cursor != next_index:
        logger.error(
            "Conflicting SR2 save revision cursor for "
            f"checksum {checksum:08X}: stored={local_cursor} plugin={next_index}"
        )
        return False
    return True
