from BaseClasses import CollectionState
from test.bases import WorldTestBase
from ..world import SR2World


class SR2TestBase(WorldTestBase):
    game = "Saints Row 2"
    world: SR2World

    def get_fresh_state(self) -> CollectionState:
        return CollectionState(self.world.multiworld)
