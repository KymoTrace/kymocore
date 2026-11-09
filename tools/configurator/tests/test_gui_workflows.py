# SPDX-License-Identifier: MIT
"""Real Qt Widgets workflows for the guided and the professional mode."""
import json
import os
import tempfile
import unittest
from pathlib import Path

try:
    from PySide6.QtWidgets import QApplication
    from kymo_configurator import catalog
    from kymo_configurator.gui.main_window import GUIDED, PROFESSIONAL, MainWindow
except ImportError:
    QApplication = None
    MainWindow = None


def field(key):
    return next(f for s in catalog.sections() for f in s.fields() if f.key == key)


@unittest.skipIf(QApplication is None, "PySide6 GUI extra is not installed")
class GuiWorkflowTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
        cls.app = QApplication.instance() or QApplication([])

    def window(self, mode=None, path=None):
        window = MainWindow(path, mode)
        self.addCleanup(self.close, window)
        return window

    def close(self, window):
        window.session.dirty = False
        window.close()

    def editor(self, window, key):
        section = next(s for s in catalog.sections() if any(f.key == key for f in s.fields()))
        return window.professional.pages[section.key].editors[key]

    def test_start_page_offers_both_modes(self):
        window = self.window()
        self.assertEqual(window.mode(), "")
        window.set_mode(GUIDED)
        self.assertEqual(window.mode(), GUIDED)
        window.set_mode(PROFESSIONAL)
        self.assertEqual(window.mode(), PROFESSIONAL)

    def test_guided_goal_selects_features_and_adds_trace_step(self):
        window = self.window(GUIDED)
        wizard = window.guided
        self.assertNotIn("trace", [s.key for s in wizard.step_sections()])
        page = wizard.pages["project"]
        cards = page.editors["profile.preset"]
        xyz, trace = cards._goals["xyz"], cards._goals["trace"]
        profile = window.session.doc["profiles"]["development"]
        trace.click()
        self.assertEqual(profile["preset"], "custom")
        self.assertTrue(profile["trace"]["enabled"])
        self.assertFalse(profile["features"]["x"])
        xyz.click()
        self.assertEqual(profile["preset"], "trace")
        self.assertIn("trace", [s.key for s in wizard.step_sections()])
        self.assertIn("xyz", [s.key for s in wizard.step_sections()])
        trace.click()
        self.assertEqual(profile["preset"], "xyz")
        self.assertFalse(profile["trace"]["enabled"])
        trace.click()
        # Advanced settings stay hidden in guided mode.
        self.assertNotIn("features.decoder", wizard.pages["callbacks"].editors)

    def test_guided_next_is_blocked_by_errors_of_the_current_step(self):
        window = self.window(GUIDED)
        wizard = window.guided
        keys = [s.key for s in wizard.step_sections()]
        window.session.set_value(field("bindings.clock"), "")
        wizard.index = keys.index("callbacks")
        wizard.refresh()
        self.assertFalse(wizard.next())
        self.assertIn("clock", wizard.message.text())
        editor = wizard.pages["callbacks"].editors["bindings.clock"]
        editor.input.setText("my_clock")
        editor.input.editingFinished.emit()
        self.assertTrue(wizard.next())
        self.assertEqual(wizard.current_section().key, "build")

    def test_values_survive_off_switch_and_profile_change(self):
        window = self.window(PROFESSIONAL)
        session = window.session
        session.set_profile("production")
        session.set_value(field("profile.enabled"), True)
        x_editor = self.editor(window, "features.x")
        x_editor.input.setChecked(True)
        session.set_value(field("profile.enabled"), False)
        x_editor = self.editor(window, "features.x")
        self.assertFalse(x_editor.input.isEnabled())
        self.assertTrue(x_editor.reason.isVisible() or x_editor.reason.text())
        self.assertTrue(session.doc["profiles"]["production"]["features"]["x"])
        window.profile.setCurrentText("development")
        window.profile.setCurrentText("production")
        self.assertTrue(session.doc["profiles"]["production"]["features"]["x"])
        self.assertFalse(session.doc["profiles"]["production"]["enabled"])

    def test_professional_navigation_status_and_diagnostic_jump(self):
        window = self.window(PROFESSIONAL)
        window.session.set_value(field("bindings.clock"), "")
        editor = window.professional
        row = editor.keys.index("callbacks")
        self.assertTrue(editor.nav.item(row).text().startswith("✖"))
        review = editor.pages["review"].widgets[0]
        self.assertFalse(review.generate_button.isEnabled())
        review.navigate.emit("callbacks")
        self.assertEqual(editor.current_key(), "callbacks")
        problem = editor.pages["callbacks"].editors["bindings.clock"].problem
        self.assertIn("clock", problem.text())

    def test_channel_table_edits_reach_the_document(self):
        window = self.window(PROFESSIONAL)
        channels = window.professional.pages["channels"].widgets[0]
        channels.table.item(0, 1).setText("Motor current")
        self.assertEqual(window.session.doc["channels"][0]["name"], "Motor current")
        channels.add_channel()
        self.assertEqual(len(window.session.doc["channels"]), 2)
        self.assertEqual(channels.table.rowCount(), 2)

    def test_schema_one_project_is_upgraded_on_open_and_generates(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "old.json"
            window = self.window()
            old = json.loads(json.dumps(window.session.doc))
            old["schema_version"] = 1
            old["core_config_api_version"] = 1
            for profile in old["profiles"].values():
                profile.pop("preset", None)
            path.write_text(json.dumps(old), encoding="utf-8")
            opened = self.window(PROFESSIONAL, path)
            self.assertEqual(opened.session.doc["schema_version"], 2)
            self.assertTrue(opened.session.notes)
            self.assertTrue(opened.session.dirty)
            snapshot = opened.session.generate()
            self.assertTrue((snapshot / "kymo_user_config.h").exists())

    def test_extension_section_appears_in_both_modes(self):
        section = catalog.Section(
            "demo_ext", "Demo extension", "Shows that new features plug in.",
            groups=(catalog.Group("Demo", (catalog.Field(
                "demo_ext.flag", ("profiles", "development", "features", "crc"), "Demo flag",
                "Writes an existing field for the test."),)),),
            order=45)
        catalog.register_section(section)
        self.addCleanup(catalog.unregister_section, "demo_ext")
        window = self.window(PROFESSIONAL)
        self.assertIn("demo_ext", window.professional.keys)
        self.assertIn("demo_ext", [s.key for s in window.guided.step_sections()])
        editor = window.professional.pages["demo_ext"].editors["demo_ext.flag"]
        editor.input.setChecked(True)
        self.assertTrue(window.session.doc["profiles"]["development"]["features"]["crc"])

    def test_guided_mode_uses_beginner_texts_and_named_profile_copy(self):
        window = self.window(GUIDED)
        crc = field("features.crc")
        guided_editor = window.guided.pages["callbacks"].editors["features.crc"]
        professional_editor = self.editor(window, "features.crc")
        self.assertEqual(guided_editor.help.text(), crc.guided_help)
        self.assertEqual(professional_editor.help.text(), crc.help)
        self.assertIn(crc.target, guided_editor.title.toolTip())
        window.session.duplicate_profile("bench")
        self.assertEqual(window.profile.currentText(), "bench")
        with self.assertRaises(ValueError):
            window.session.duplicate_profile("bench")

    def test_startup_shows_no_stray_windows(self):
        from PySide6.QtCore import QEvent, QObject
        from PySide6.QtWidgets import QWidget
        from kymo_configurator.presets import apply_preset
        from kymo_configurator.gui.widgets import RegistryEditor

        shown = []

        class Spy(QObject):
            def eventFilter(self, obj, event):
                if event.type() == QEvent.Type.Show and isinstance(obj, QWidget) and obj.isWindow():
                    shown.append(type(obj).__name__)
                return False

        spy = Spy()
        self.app.installEventFilter(spy)
        self.addCleanup(self.app.removeEventFilter, spy)
        window = self.window(GUIDED)
        apply_preset(window.session.doc["profiles"]["development"], "trace")
        window.session.touch()
        window._build_ui(GUIDED)
        self.assertEqual(shown, [])
        page = window.professional.pages["trace"]
        tables = [w for w in page.widgets if isinstance(w, RegistryEditor)]
        self.assertTrue(tables and all(t.isVisibleTo(page) for t in tables))

    def test_rebuild_in_german_keeps_the_project(self):
        from kymo_configurator import i18n
        self.addCleanup(i18n.set_language, "en")
        window = self.window(GUIDED)
        window.session.set_value(field("project_name"), "Pflanzen")
        i18n.set_language("de")
        window._build_ui(GUIDED)
        self.assertEqual(window.guided.next_button.text(), "Weiter →")
        self.assertEqual(window.session.doc["project_name"], "Pflanzen")
        self.assertEqual(window.mode(), GUIDED)

    def test_legacy_trace_binding_is_editable_and_watch_range_can_be_cleared(self):
        window = self.window(PROFESSIONAL)
        session = window.session
        trace = session.view().profile["trace"]
        trace["clock"]["callback"] = "legacy_clock"
        session.touch()
        clock = field("trace.bindings.clock")
        self.assertEqual(session.value(clock), "legacy_clock")
        session.set_value(clock, "")
        self.assertNotIn("callback", trace["clock"])
        self.assertEqual(session.value(clock), "")
        start = field("watch.range_start")
        session.set_value(start, "")
        self.assertEqual(session.value(start), "")

    def test_failed_open_leaves_unsaved_project_intact(self):
        with tempfile.TemporaryDirectory() as directory:
            invalid = Path(directory) / "invalid.json"
            invalid.write_text('{"profiles": null}', encoding="utf-8")
            session = self.window(PROFESSIONAL).session
            session.set_value(field("project_name"), "Unsaved")
            with self.assertRaises(ValueError):
                session.load(invalid)
            self.assertTrue(session.dirty)
            self.assertEqual(session.doc["project_name"], "Unsaved")


if __name__ == "__main__":
    unittest.main()
