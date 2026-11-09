# SPDX-License-Identifier: MIT
"""RAM estimate per profile and the budget warning; CRC tables as an optional feature."""
import unittest
import re

from kymo_configurator.generation import _render
from kymo_configurator.model import default_project
from kymo_configurator.resources import estimate_ram
from kymo_configurator.validation import validate


def project_with(**features):
    project = default_project()
    profile = project["profiles"]["development"]
    profile["features"].update(features)
    return project


class ResourceEstimateTest(unittest.TestCase):
    def test_parts_follow_the_features_and_measured_sizes(self):
        project = project_with(runtime=True, decoder=True, watch=True, memory=True)
        project["channels"] = [{"id": i, "name": f"c{i}", "period_ms": 20, "x": False, "z": False,
                                "timestamp": False} for i in range(16)]
        parts = estimate_ram(project, "development")
        self.assertEqual(parts, {"runtime": 136, "control": 32, "watch": 224, "memory": 44})

    def test_trace_defaults_match_the_measured_recorder(self):
        project = default_project()
        trace = project["profiles"]["development"]["trace"]
        trace["enabled"] = True
        trace["buffer"]["events_per_core"] = 64
        parts = estimate_ram(project, "development")
        header = _render(project, "development")["kymo_user_config.h"]
        admin_bytes = int(re.search(rb"KYMO_MUX_ADMIN_RECORD_MAX (\d+)", header)[1])
        self.assertEqual(parts["trace"], 3400)
        self.assertEqual(parts["mux"], (332 + admin_bytes + 7) // 8 * 8 + 24)
        trace["isr"] = True
        trace["port"]["core_count"] = 2
        trace["buffer"].update(events_per_core=16, max_depth=5, max_isr_nesting=4)
        self.assertEqual(estimate_ram(project, "development")["trace"], 1640)

    def test_budget_warning_only_when_exceeded(self):
        project = project_with(runtime=True)
        project["resources"]["ram_budget_bytes"] = 50
        warnings = [d for d in validate(project, "development") if d.code == "resources.ram_budget_exceeded"]
        self.assertEqual([d.severity for d in warnings], ["warning"])
        project["resources"]["ram_budget_bytes"] = 100000
        self.assertFalse([d for d in validate(project, "development") if d.code == "resources.ram_budget_exceeded"])

    def test_crc_table_feature_reaches_the_header(self):
        header = _render(project_with(crc=True, crc_table=True), "development")["kymo_user_config.h"]
        self.assertIn(b"#define KYMO_ENABLE_CRC_TABLE 1", header)


if __name__ == "__main__":
    unittest.main()
