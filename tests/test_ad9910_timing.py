import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class AD9910TimingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / "TFT+AD7060B/Core/Src/ad9910.c").read_text(encoding="utf-8")

    def _function_body(self, name):
        pattern = rf"static void {name}\([^)]*\)\n\{{(?P<body>.*?)\n\}}"
        match = re.search(pattern, self.source, re.S)
        self.assertIsNotNone(match, f"{name} not found")
        return match.group("body")

    def test_bitbang_write_byte_has_data_setup_and_clock_hold_delays(self):
        body = self._function_body("AD9910_WriteByte")

        self.assertIn("#define AD9910_DELAY_NOP_COUNT", self.source)
        self.assertIn("__DSB()", self.source)
        self.assertGreaterEqual(body.count("AD9910_ShortDelay();"), 3)
        self.assertRegex(
            body,
            r"(?s)AD9910_Pin(?:Set|Clr)\(DDS_SDIO_GPIO_Port, DDS_SDIO_Pin\);"
            r".*?AD9910_ShortDelay\(\);"
            r".*?AD9910_PinSet\(DDS_SCLK_GPIO_Port, DDS_SCLK_Pin\);"
            r".*?AD9910_ShortDelay\(\);"
            r".*?AD9910_PinClr\(DDS_SCLK_GPIO_Port, DDS_SCLK_Pin\);"
            r".*?AD9910_ShortDelay\(\);",
        )

    def test_register_write_has_cs_timing_and_short_critical_section(self):
        body = self._function_body("AD9910_WriteRegister")

        self.assertIn("__get_PRIMASK", body)
        self.assertIn("__disable_irq", body)
        self.assertIn("__enable_irq", body)
        self.assertIn("AD9910_SelectChip();", body)
        self.assertIn("AD9910_DeselectChip();", body)


if __name__ == "__main__":
    unittest.main()
