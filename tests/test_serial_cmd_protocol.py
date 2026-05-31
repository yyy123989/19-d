import unittest
import pathlib

from tools.serial_cmd_protocol import parse_command

ROOT = pathlib.Path(__file__).resolve().parents[1]


class SerialCommandProtocolTest(unittest.TestCase):
    def test_full_dds_command_sets_frequency_amplitude_and_phase(self):
        result = parse_command("DDS F=1000 A=8192 P=90")

        self.assertEqual(result["type"], "set")
        self.assertEqual(result["freq_hz"], 1000)
        self.assertEqual(result["amplitude"], 8192)
        self.assertEqual(result["phase_deg"], 90)

    def test_frequency_can_be_customized_alone(self):
        result = parse_command("FREQ=2500")

        self.assertEqual(result["type"], "set")
        self.assertEqual(result["freq_hz"], 2500)
        self.assertIsNone(result["amplitude"])
        self.assertIsNone(result["phase_deg"])

    def test_amplitude_and_phase_can_be_updated_without_frequency(self):
        result = parse_command("AMP=12000 PHASE=180")

        self.assertEqual(result["type"], "set")
        self.assertIsNone(result["freq_hz"])
        self.assertEqual(result["amplitude"], 12000)
        self.assertEqual(result["phase_deg"], 180)

    def test_query_command_is_supported(self):
        result = parse_command("DDS?")

        self.assertEqual(result["type"], "query")

    def test_frequency_and_amplitude_ranges_are_checked(self):
        with self.assertRaises(ValueError):
            parse_command("DDS F=420000001")

        with self.assertRaises(ValueError):
            parse_command("DDS A=16384")

        with self.assertRaises(ValueError):
            parse_command("DDS P=361")

    def test_firmware_command_responses_check_tx_queue_result(self):
        source = (ROOT / "TFT+AD7060B/Core/Src/serial_cmd.c").read_text(encoding="utf-8")

        self.assertIn("SerialCommand_WriteText", source)
        self.assertNotIn("(void)USART1_Write", source)


if __name__ == "__main__":
    unittest.main()
