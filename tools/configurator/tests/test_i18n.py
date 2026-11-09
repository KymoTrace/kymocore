# SPDX-License-Identifier: MIT
"""Every user-visible English text has a German translation with the same placeholders."""
import ast
import string
import unittest
from pathlib import Path

from kymo_configurator import catalog, i18n
from kymo_configurator.i18n_de import DE
from kymo_configurator.presets import PRESETS

SOURCE = Path(__file__).resolve().parents[1] / "src" / "kymo_configurator"
CHANNEL_TEXTS = ("ID", "Name", "Period (ms)", "Rate", "X", "Z", "Time", "Bytes/frame")


def tr_literals() -> set[str]:
    """String literals passed directly to tr() anywhere in the package."""
    found = set()
    for path in SOURCE.rglob("*.py"):
        for node in ast.walk(ast.parse(path.read_text(encoding="utf-8"))):
            if (isinstance(node, ast.Call) and getattr(node.func, "id", "") == "tr" and node.args
                    and isinstance(node.args[0], ast.Constant) and isinstance(node.args[0].value, str)):
                found.add(node.args[0].value)
    return found


def diagnostic_templates() -> set[str]:
    """Message and suggestion templates of the add() calls in validation.py."""
    found = set()

    def strings(node):
        if isinstance(node, ast.Constant) and isinstance(node.value, str):
            yield node.value
        elif isinstance(node, ast.IfExp):
            yield from strings(node.body)
            yield from strings(node.orelse)
        else:
            raise AssertionError(f"diagnostic text is not a template literal: {ast.unparse(node)}")

    tree = ast.parse((SOURCE / "validation.py").read_text(encoding="utf-8"))
    for node in ast.walk(tree):
        name = getattr(node.func, "id", "") if isinstance(node, ast.Call) else ""
        if name == "add":
            texts = node.args[2:4] + [k.value for k in node.keywords if k.arg == "suggestion"]
            for text in texts:
                found.update(strings(text))
        elif name == "Diagnostic" and isinstance(node.args[3], ast.Constant):
            found.add(node.args[3].value)
    return found


def catalog_texts() -> set[str]:
    found = set()
    for section in catalog.sections():
        found |= {section.title, section.description, section.guided_title, section.guided_intro,
                  section.guided_nav}
        for group in section.groups:
            found |= {group.title, group.description, group.guided_description}
            for field in group.fields:
                found |= {field.label, field.help, field.guided_help, field.requires, field.cost}
                found |= {label for _, label in field.choices}
    for preset in PRESETS.values():
        found |= {preset.title, preset.question}
    return found


def all_texts() -> set[str]:
    texts = tr_literals() | catalog_texts() | diagnostic_templates() | set(CHANNEL_TEXTS)
    try:
        from kymo_configurator.gui.widgets import ChannelEditor
    except ImportError:
        pass
    else:
        texts |= set(ChannelEditor.HEADERS) | set(ChannelEditor.TIPS)
    return {t for t in texts if t and not t.isdigit()}


def placeholders(text: str) -> set[str]:
    return {name for _, name, _, _ in string.Formatter().parse(text) if name}


class GermanTranslationTests(unittest.TestCase):
    def test_every_text_is_translated(self):
        # Texts that are the same in German (names, units, symbols).
        same = {"ID", "X", "Z", "Generic", "CMake", "PlatformIO", "32 bit", "64 bit", "Name",
                "ESP32 + FreeRTOS"}
        missing = sorted(t for t in all_texts() if t not in DE and t not in same)
        self.assertEqual(missing, [])

    def test_no_stale_translations(self):
        self.assertEqual(sorted(set(DE) - all_texts()), [])

    def test_placeholders_match(self):
        for english, german in DE.items():
            self.assertEqual(placeholders(english), placeholders(german), english)

    def test_switching_language(self):
        self.addCleanup(i18n.set_language, "en")
        i18n.set_language("de")
        self.assertEqual(i18n.tr("Step {n} of {total}", n=2, total=6), "Schritt 2 von 6")
        self.assertEqual(i18n.tr("text without translation"), "text without translation")
        i18n.set_language("xx")
        self.assertEqual(i18n.language(), "en")

    def test_diagnostics_translate_only_for_display(self):
        from kymo_configurator.model import default_project
        from kymo_configurator.validation import validate
        self.addCleanup(i18n.set_language, "en")
        project = default_project()
        project["channels"].append(dict(project["channels"][0]))
        duplicate = next(d for d in validate(project) if d.code == "channel.duplicate_id")
        i18n.set_language("de")
        again = next(d for d in validate(project) if d.code == "channel.duplicate_id")
        self.assertEqual(again.to_dict(), duplicate.to_dict())
        self.assertIn("is duplicated", again.message)
        self.assertIn("doppelt vergeben", again.localized()[0])


if __name__ == "__main__":
    unittest.main()
