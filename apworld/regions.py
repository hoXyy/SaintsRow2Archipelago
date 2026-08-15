from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .world import SR2World


from BaseClasses import Region


def create_regions(world: SR2World) -> None:
    # Saints Row 2 is very non-linear, so a single region should be enough for this.
    # To be tested at least, if needed I can think of splitting it.
    main = Region("Stilwater", world.player, world.multiworld)

    regions = [main]

    world.multiworld.regions += regions
