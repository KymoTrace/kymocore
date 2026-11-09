# SPDX-License-Identifier: MIT
"""Each step-1 goal tile switches its own sidebar section; rows, tiles and search behave."""
import os
import unittest

try:
    from PySide6.QtCore import Qt
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication
    from kymo_configurator import catalog
    from kymo_configurator.gui.professional import ProfessionalEditor
    from kymo_configurator.gui.section_page import SectionPage
    from kymo_configurator.gui.session import ProjectSession
    from kymo_configurator.presets import apply_goals, goals
except ImportError:
    QApplication = None


def tiles(page):
    return [w for w in page.editors["profile.preset"].input.children()
            if getattr(w, "button", None) is not None]


def visible_keys(session):
    view = session.view()
    return [s.key for s in catalog.sections() if s.is_visible(view)]


@unittest.skipIf(QApplication is None, "PySide6 GUI extra is not installed")
class GoalSectionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
        cls.app = QApplication.instance() or QApplication([])

    def test_each_tile_shows_its_own_section(self):
        session = ProjectSession()
        apply_goals(session.view().profile, False, False)
        session.touch()
        goal_sections = ("xyz", "trace", "parameters", "watch", "memory")
        self.assertFalse(set(goal_sections) & set(visible_keys(session)))
        for guided in (True, False):
            page = SectionPage(catalog.sections()[0], session, guided=guided)
            self.assertEqual(len(tiles(page)), 6)
        page = SectionPage(catalog.sections()[0], session, guided=True)
        for index, key in enumerate(goal_sections, start=1):
            QTest.mouseClick(tiles(page)[index], Qt.MouseButton.LeftButton)
            self.assertIn(key, visible_keys(session))
        features = session.view().profile["features"]
        self.assertIs(features["decoder"], True)          # parameters and watch need it
        QTest.mouseClick(tiles(page)[2], Qt.MouseButton.LeftButton)
        self.assertEqual(goals(session.view().profile), (True, False))
        self.assertNotIn("trace", visible_keys(session))

    def test_values_tile_can_be_switched_off_and_xyz_needs_it(self):
        session = ProjectSession()
        page = SectionPage(catalog.sections()[0], session, guided=True)
        values, xyz = tiles(page)[0], tiles(page)[1]
        QTest.mouseClick(xyz, Qt.MouseButton.LeftButton)
        QTest.mouseClick(values, Qt.MouseButton.LeftButton)        # off: takes X/Y along
        features = session.view().profile["features"]
        self.assertFalse(features["runtime"] or features["cpp"] or features["x"])
        self.assertFalse({"values", "xyz", "channels"} & set(visible_keys(session)))
        QTest.mouseClick(xyz, Qt.MouseButton.LeftButton)           # X/Y brings values back
        self.assertTrue(values.button.isChecked())
        keys = visible_keys(session)
        self.assertLess(keys.index("xyz"), keys.index("channels"))

    def test_switch_row_click_and_grey_when_off(self):
        session = ProjectSession()
        page = SectionPage(catalog.section("callbacks"), session, guided=False)
        editor = page.editors["features.crc"]
        self.assertTrue(editor.title.property("off"))
        QTest.mouseClick(editor.title, Qt.MouseButton.LeftButton)
        self.assertIs(session.value(editor.field), True)
        self.assertFalse(editor.title.property("off"))

    def test_settings_search_filters_sections_and_examples_fold(self):
        session = ProjectSession()
        editor = ProfessionalEditor(session)
        editor.search.setText("KYMO_ENABLE_CRC")
        shown = [editor.keys[r] for r in range(editor.nav.count()) if not editor.nav.item(r).isHidden()]
        self.assertEqual(shown, ["callbacks"])
        editor.search.clear()
        clock = editor.pages["callbacks"].editors["bindings.clock"]
        self.assertFalse(clock.example.isVisibleTo(clock))
        clock.example_button.click()
        self.assertTrue(clock.example.isVisibleTo(clock))

    def test_trace_tile_preserves_existing_xyz_switches(self):
        session = ProjectSession()
        features = session.view().profile["features"]
        features["x"] = True
        page = SectionPage(catalog.section("project"), session, guided=True)
        QTest.mouseClick(tiles(page)[2], Qt.MouseButton.LeftButton)
        self.assertEqual([features[key] for key in ("x", "z", "timestamp")], [True, False, False])


if __name__ == "__main__":
    unittest.main()
