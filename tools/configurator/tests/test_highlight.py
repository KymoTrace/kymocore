# SPDX-License-Identifier: MIT
"""Syntax colours of the generated-file preview."""
import os
import unittest

try:
    from PySide6.QtGui import QColor, QTextDocument
    from PySide6.QtWidgets import QApplication
    from kymo_configurator.gui import style
    from kymo_configurator.gui.highlight import CodeHighlighter
except ImportError:
    QApplication = None

SOURCE = """--- kymo_port.cpp (new) ---
#include <Arduino.h>
/* first
   second */
static uint32_t clock_ms(void) // note
# include("a CMake comment")
+#define KYMO_ENABLE_WATCH 1"""


@unittest.skipIf(QApplication is None, "PySide6 GUI extra is not installed")
class HighlightTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
        cls.app = QApplication.instance() or QApplication([])

    def colour_at(self, document, line, column, background=False):
        block = document.findBlockByNumber(line)
        for fmt_range in block.layout().formats():
            if fmt_range.start <= column < fmt_range.start + fmt_range.length:
                brush = fmt_range.format.background() if background else fmt_range.format.foreground()
                return brush.color().name().upper()
        return None

    def test_rules(self):
        document = QTextDocument()
        highlighter = CodeHighlighter(document)
        highlighter.set_diff(True)
        document.setPlainText(SOURCE)
        highlighter.rehighlight()
        self.assertEqual(self.colour_at(document, 1, 0), style.WARNING.upper())           # directive
        self.assertEqual(self.colour_at(document, 1, 10), style.CHIPS["live"][2].upper())  # <Arduino.h>
        self.assertEqual(self.colour_at(document, 3, 2), style.TEXT_HINT.upper())         # comment, 2nd line
        self.assertEqual(self.colour_at(document, 4, 0), style.PRIMARY.upper())           # static
        self.assertEqual(self.colour_at(document, 4, 33), style.TEXT_HINT.upper())        # // note
        self.assertEqual(self.colour_at(document, 5, 3), style.TEXT_HINT.upper())         # # comment
        self.assertEqual(self.colour_at(document, 6, 0, background=True),
                         style.CHIPS["live"][0].upper())                                  # + line tint


if __name__ == "__main__":
    unittest.main()
