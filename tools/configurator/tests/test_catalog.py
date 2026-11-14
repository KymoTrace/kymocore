# SPDX-License-Identifier: MIT
"""Qt-free checks for the setting catalog, presets, migration and extension hooks."""
import copy
import unittest

from kymo_configurator import catalog
from kymo_configurator.generation import _render, register_renderer, unregister_renderer
from kymo_configurator.migration import migrate, needs_migration
from kymo_configurator.model import CORE_CONFIG_API_VERSION, SCHEMA_VERSION, default_project
from kymo_configurator.presets import PRESETS, apply_preset, matching_preset
from kymo_configurator.validation import (Diagnostic, register_validator, unregister_validator,
                                          validate)


def field(key):
    return next(f for s in catalog.sections() for f in s.fields() if f.key == key)


class CatalogTests(unittest.TestCase):
    def test_every_field_writes_a_location_the_validator_knows(self):
        project = default_project()
        apply_preset(project["profiles"]["development"], "trace")
        for section in catalog.sections():
            for item in section.fields():
                with self.subTest(field=item.key):
                    doc = copy.deepcopy(project)
                    value = item.value(catalog.View(doc, "development"))
                    if value is None and item.kind == "int":
                        value = item.minimum
                    item.store(doc, "development", value)
                    unknown = [d for d in validate(doc, "development") if d.code == "schema.unknown_field"]
                    self.assertEqual(unknown, [])

    def test_field_keys_are_unique_and_explained(self):
        keys = [f.key for s in catalog.sections() for f in s.fields()]
        self.assertEqual(len(keys), len(set(keys)))
        for section in catalog.sections():
            self.assertTrue(section.title and section.description, section.key)
            for item in section.fields():
                self.assertTrue(item.label and item.help, item.key)

    def test_preset_sets_features_and_manual_edit_becomes_custom(self):
        project = default_project()
        field("profile.preset").store(project, "development", "xyz")
        profile = project["profiles"]["development"]
        self.assertTrue(profile["features"]["x"] and profile["features"]["timestamp"])
        self.assertEqual(profile["preset"], "xyz")
        field("features.crc").store(project, "development", True)
        self.assertEqual(profile["preset"], "custom")
        field("features.crc").store(project, "development", False)
        self.assertEqual(profile["preset"], "xyz")

    def test_every_preset_is_valid_out_of_the_box(self):
        for key in PRESETS:
            with self.subTest(preset=key):
                project = default_project()
                apply_preset(project["profiles"]["development"], key)
                self.assertEqual(matching_preset(project["profiles"]["development"]), key)
                errors = [d for d in validate(project, "development") if d.severity == "error"]
                self.assertEqual(errors, [])

    def test_identifier_cleared_removes_binding(self):
        project = default_project()
        field("bindings.service").store(project, "development", "")
        self.assertNotIn("service", project["bindings"])

    def test_port_change_resets_capability_verification(self):
        project = default_project()
        project["port"]["capabilities_verified"] = True
        field("port.kind").store(project, "development", "generic")
        self.assertTrue(project["port"]["capabilities_verified"])
        field("port.kind").store(project, "development", "esp32_freertos")
        self.assertFalse(project["port"]["capabilities_verified"])
        self.assertEqual(project["profiles"]["development"]["trace"]["port"]["kind"], "esp32_freertos")

    def test_diagnostics_route_to_their_section(self):
        project = default_project()
        project["bindings"]["clock"] = ""
        project["channels"].append(dict(project["channels"][0]))
        owners = {d.path: catalog.section_for_path(d.path, "development").key
                  for d in validate(project, "development")}
        self.assertEqual(owners["bindings.clock"], "callbacks")
        self.assertEqual(owners["channels[1].id"], "channels")

    def test_trace_section_is_a_guided_step_only_for_trace_goals(self):
        project = default_project()
        view = catalog.View(project, "development")
        self.assertFalse(catalog.section("trace").in_guided(view))
        apply_preset(project["profiles"]["development"], "trace")
        self.assertTrue(catalog.section("trace").in_guided(view))


class MigrationTests(unittest.TestCase):
    def test_schema_one_migrates_to_kymocore_seven(self):
        old = default_project()
        old["schema_version"] = 1
        old["core_config_api_version"] = 1
        for profile in old["profiles"].values():
            profile.pop("preset")
        self.assertTrue(needs_migration(old))
        self.assertEqual(validate(old, "development")[0].code, "schema.version")
        new = migrate(old)
        self.assertEqual(new["schema_version"], SCHEMA_VERSION)
        self.assertEqual(new["core_config_api_version"], CORE_CONFIG_API_VERSION)
        self.assertEqual(new["profiles"]["development"]["preset"], "minimal")
        self.assertEqual([d for d in validate(new, "development") if d.severity == "error"], [])

    def test_generated_header_requires_configuration_api_two(self):
        files = _render(default_project(), "development")
        self.assertIn(b"KYMO_CONFIG_API_VERSION != 2", files["kymo_user_config.h"])
        self.assertIn(b"KYMO_MAX_CHANNELS", files["kymo_user_config.h"])
        self.assertIn(b"#include <kymo_modules.h>", files["kymo_generated_config.h"])


class ExtensionTests(unittest.TestCase):
    """A future feature (e.g. a back channel) plugs in without touching the core modules."""

    def setUp(self):
        self.section = catalog.Section(
            "demo_backchannel", "Back channel (demo)", "Example extension section.",
            groups=(catalog.Group("Demo", (catalog.Field(
                "demo.enabled", ("demo_backchannel", "enabled"), "Receive commands",
                "Demo setting stored in an extension-owned top-level field.",
                target="KYMO_ENABLE_DEMO"),)),),
            diagnostic_prefixes=("demo_backchannel",), order=55)

        def validator(project, profile):
            value = project.get("demo_backchannel", {}).get("enabled", False)
            return [] if isinstance(value, bool) else [
                Diagnostic("demo.type", "error", "demo_backchannel.enabled", "Must be boolean")]

        def renderer(project, profile, resolved):
            enabled = int(bool(project.get("demo_backchannel", {}).get("enabled", False)))
            return {"kymo_demo.h": f"#define KYMO_ENABLE_DEMO {enabled}\n"}

        self.validator = validator
        self.renderer = renderer
        catalog.register_section(self.section)
        register_validator(validator, ("demo_backchannel",))
        register_renderer(renderer)

    def tearDown(self):
        catalog.unregister_section(self.section.key)
        unregister_validator(self.validator, ("demo_backchannel",))
        unregister_renderer(self.renderer)

    def test_extension_section_field_validator_and_renderer(self):
        project = default_project()
        self.assertIn("demo_backchannel", [s.key for s in catalog.sections()])
        field("demo.enabled").store(project, "development", True)
        self.assertEqual(validate(project, "development"), [])
        self.assertEqual(_render(project, "development")["kymo_demo.h"], b"#define KYMO_ENABLE_DEMO 1\n")
        project["demo_backchannel"]["enabled"] = "yes"
        problem = validate(project, "development")[0]
        self.assertEqual(catalog.section_for_path(problem.path, "development").key, "demo_backchannel")


if __name__ == "__main__":
    unittest.main()
