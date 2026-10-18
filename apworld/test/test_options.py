from .bases import SR2TestBase
from ..options import (
    RONIN_ARC_NAME,
    BROTHERHOOD_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)
from Options import OptionError


class TestRoninOnly(SR2TestBase):
    options = {"required_gang_arcs": {RONIN_ARC_NAME}}


class TestSamediOnly(SR2TestBase):
    options = {"required_gang_arcs": {SAMEDI_ARC_NAME}}


class TestBrotherhoodOnly(SR2TestBase):
    options = {"required_gang_arcs": {BROTHERHOOD_ARC_NAME}}


class TestUltorOnly(SR2TestBase):
    options = {"required_gang_arcs": {ULTOR_EPILOGUE_ARC_NAME}}


class TestFullGame(SR2TestBase):
    options = {
        "required_gang_arcs": {
            BROTHERHOOD_ARC_NAME,
            SAMEDI_ARC_NAME,
            RONIN_ARC_NAME,
            ULTOR_EPILOGUE_ARC_NAME,
        }
    }


class TestInvalidOptions(SR2TestBase):
    auto_construct = False
    run_default_tests = False

    def test_requires_at_least_one_arc(self) -> None:
        self.options = {"required_gang_arcs": set()}

        with self.assertRaisesRegex(OptionError, "enable any gang arcs"):
            self.world_setup()
