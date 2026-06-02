import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class DDSOskLifetimeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / "TFT+AD7060B/Core/Src/dds_app.c").read_text(encoding="utf-8")

    def _function_body(self, name):
        pattern = rf"static void {name}\([^)]*\)\n\{{(?P<body>.*?)\n\}}"
        match = re.search(pattern, self.source, re.S)
        self.assertIsNotNone(match, f"{name} not found")
        return match.group("body")

    def test_non_osk_modes_disable_auto_osk_state(self):
        self.assertIn("static void DDS_AppDisableOsk", self.source)
        for name in ("DDS_AppApplySingle", "DDS_AppApplyRam", "DDS_AppApplyDrg"):
            self.assertIn("DDS_AppDisableOsk();", self._function_body(name))

    def test_osk_tick_only_runs_while_osk_is_active_mode(self):
        tick_body = re.search(r"void DDS_AppTick\(uint32_t now_ms\)\n\{(?P<body>.*?)\n\}", self.source, re.S)
        self.assertIsNotNone(tick_body)
        self.assertIn("dds_app.active_mode == DDS_MODE_OSK", tick_body.group("body"))


if __name__ == "__main__":
    unittest.main()
