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


if __name__ == "__main__":
    unittest.main()
