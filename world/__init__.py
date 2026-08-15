from worlds.LauncherComponents import (
    Component,
    Type,
    components,
    icon_paths,
    launch as launch_component,
)

from .world import SR2World as SR2World

icon_paths["sr2"] = f"ap:{__name__}/icon.png"


def launch_client(*args: str) -> None:
    from .client import launch

    launch_component(launch, name="Saints Row 2 Client", args=args)


components.append(
    Component(
        "Saints Row 2 Client",
        game_name="Saints Row 2",
        func=launch_client,
        component_type=Type.CLIENT,
        icon="sr2",
        supports_uri=True,
    )
)
