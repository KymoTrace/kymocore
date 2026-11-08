# SPDX-License-Identifier: MIT
"""Memory telemetry is an optional feature: it needs no decoder and old projects stay valid."""
import unittest

from kymo_configurator.generation import _render
from kymo_configurator.model import default_project
from kymo_configurator.validation import validate


class MemoryFeatureTest(unittest.TestCase):
    def test_feature_reaches_the_header_without_the_decoder(self):
        project = default_project()
        project["profiles"]["development"]["features"]["memory"] = True
        self.assertEqual({d.code for d in validate(project, "development") if d.severity == "error"}, set())
        self.assertIn(b"#define KYMO_ENABLE_MEMORY 1", _render(project, "development")["kymo_user_config.h"])

    def test_project_without_the_key_stays_valid(self):
        project = default_project()
        del project["profiles"]["development"]["features"]["memory"]
        self.assertEqual({d.code for d in validate(project, "development")},
                         {d.code for d in validate(default_project(), "development")})


if __name__ == "__main__":
    unittest.main()
