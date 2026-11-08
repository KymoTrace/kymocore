# SPDX-License-Identifier: MIT
"""Live variables are an optional feature: old projects stay valid."""
import unittest

from kymo_configurator.generation import _render
from kymo_configurator.model import default_project
from kymo_configurator.validation import validate


def codes(project):
    return {d.code for d in validate(project, "development")}


class WatchFeatureTest(unittest.TestCase):
    def test_project_without_the_key_stays_valid(self):
        project = default_project()
        del project["profiles"]["development"]["features"]["watch"]
        self.assertEqual(codes(project), codes(default_project()))

    def test_watch_needs_the_decoder_and_reaches_the_header(self):
        project = default_project()
        features = project["profiles"]["development"]["features"]
        features["watch"] = True
        self.assertIn("feature.watch_decoder", codes(project))
        features["decoder"] = True
        self.assertNotIn("feature.watch_decoder", codes(project))
        header = b"".join(content for name, content in _render(project, "development").items()
                          if name.endswith(".h"))
        self.assertIn(b"KYMO_ENABLE_WATCH", header)


def watch_project(**watch):
    project = default_project()
    features = project["profiles"]["development"]["features"]
    features["watch"] = features["decoder"] = True
    if watch:
        project["watch"] = watch
    return project


class WatchGenerationTest(unittest.TestCase):
    def test_defaults_generate_the_esp32_init_file_and_macros(self):
        files = _render(watch_project(), "development")
        source = files["kymo_generated_watch.cpp"].decode()
        self.assertIn('extern "C" uint8_t _data_start;', source)
        self.assertIn("(uintptr_t)&_bss_end - (uintptr_t)&_data_start", source)
        self.assertIn("{ ranges, 1u, id, 0u", source.replace("id_length", "0u"))
        self.assertIn(b"kymo_generated_watch_init", files["kymo_generated_config.h"])
        self.assertIn(b"#define KYMO_MAX_WATCHES 8u", files["kymo_user_config.h"])

    def test_two_ranges_and_build_id(self):
        project = watch_project(symbols="custom", range_start="__data_start__", range_end="__bss_end__",
                                range2_start="_ext_ram_start", range2_end="_ext_ram_end",
                                build_id="kymo_port_build_id", slots=4, min_period_ms=10)
        self.assertEqual(codes(project) & {"watch.no_range", "watch.identifier", "watch.range"}, set())
        files = _render(project, "development")
        source = files["kymo_generated_watch.cpp"].decode()
        self.assertIn("ranges[1].start = (uint32_t)(uintptr_t)&_ext_ram_start;", source)
        self.assertIn("id_length = kymo_port_build_id(&id);", source)
        self.assertIn(b"#define KYMO_WATCH_MIN_PERIOD_MS 10u", files["kymo_user_config.h"])

    def test_without_watch_nothing_is_generated(self):
        files = _render(default_project(), "development")
        self.assertNotIn("kymo_generated_watch.cpp", files)
        self.assertNotIn(b"kymo_generated_watch", files["kymo_generated_config.h"])

    def test_invalid_settings_are_reported(self):
        self.assertIn("watch.no_range", codes(watch_project(range_start="", range_end="")))
        self.assertIn("watch.identifier", codes(watch_project(range_start="1bad")))
        self.assertIn("watch.range", codes(watch_project(slots=65)))
        self.assertIn("watch.range2_incomplete", codes(watch_project(range2_start="_x")))
        self.assertIn("schema.unknown_field", codes(watch_project(colour="red")))

    def test_symbol_preset_fills_the_first_range(self):
        from kymo_configurator.catalog import sections
        field = next(f for section in sections() for f in section.fields() if f.key == "watch.symbols")
        project = watch_project()
        field.store(project, "development", "stm32")
        self.assertEqual((project["watch"]["range_start"], project["watch"]["range_end"]), ("_sdata", "_ebss"))


if __name__ == "__main__":
    unittest.main()
