# SPDX-License-Identifier: MIT
"""Stack pointer sampling is an optional trace option: old projects stay unchanged."""
import unittest

from kymo_configurator.generation import _render
from kymo_configurator.model import default_project
from kymo_configurator.validation import validate


def trace_project(**trace):
    project = default_project()
    profile = project["profiles"]["development"]
    profile["trace"]["enabled"] = True
    profile["trace"].update(trace)
    return project


class TraceStackPointerTest(unittest.TestCase):
    def test_option_reaches_the_header(self):
        header = _render(trace_project(stack_pointer=True), "development")["kymo_user_config.h"]
        self.assertIn(b"#define KYMO_ENABLE_TRACE_STACK_POINTER 1", header)

    def test_absent_option_leaves_the_header_unchanged(self):
        header = _render(trace_project(), "development")["kymo_user_config.h"]
        self.assertNotIn(b"KYMO_ENABLE_TRACE_STACK_POINTER", header)

    def test_option_must_be_boolean(self):
        codes = {d.code for d in validate(trace_project(stack_pointer="yes"), "development")}
        self.assertIn("boolean.type", codes)
        codes = {d.code for d in validate(trace_project(stack_pointer=True), "development")}
        self.assertNotIn("schema.unknown_field", codes)


if __name__ == "__main__":
    unittest.main()
