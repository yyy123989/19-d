import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class MeasureFeatureTests(unittest.TestCase):
    def test_measure_module_files_exist_and_define_known_resistors(self):
        header = ROOT / "TFT+AD7060B/Core/Inc/measure_app.h"
        source = ROOT / "TFT+AD7060B/Core/Src/measure_app.c"

        self.assertTrue(header.exists())
        self.assertTrue(source.exists())

        header_text = header.read_text(encoding="utf-8")
        source_text = source.read_text(encoding="utf-8")
        self.assertIn("MEASURE_INPUT_SERIES_OHM 10000UL", header_text)
        self.assertIn("MEASURE_OUTPUT_LOAD_OHM 1000UL", header_text)
        self.assertIn("Measure_AppAccumulate", header_text)
        self.assertIn("Measure_AppDraw", header_text)
        self.assertIn("Measure_AppInvalidate", header_text)
        self.assertIn("MEASURE_TRACK_CHANNEL_COUNT 3U", source_text)
        self.assertIn("MEASURE_MIN_VPP_MV 20UL", source_text)
        self.assertIn("measure_draw_cache", source_text)

    def test_measure_page_is_wired_into_menu_and_build_project(self):
        dds_app = (ROOT / "TFT+AD7060B/Core/Src/dds_app.c").read_text(encoding="utf-8")
        main = (ROOT / "TFT+AD7060B/Core/Src/main.c").read_text(encoding="utf-8")
        uvproj = (ROOT / "TFT+AD7060B/MDK-ARM/2019_D.uvprojx").read_text(encoding="utf-8")

        self.assertIn('"MEASURE"', dds_app)
        self.assertIn("DDS_PAGE_MEASURE", dds_app)
        self.assertIn("Measure_AppHandleKey", dds_app)
        self.assertIn("Measure_AppDraw", dds_app)
        self.assertIn("Measure_AppInvalidate", dds_app)
        self.assertIn("Measure_AppAccumulate", main)
        self.assertIn("measure_app.c", uvproj)


if __name__ == "__main__":
    unittest.main()
