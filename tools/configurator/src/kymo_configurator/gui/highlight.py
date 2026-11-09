# SPDX-License-Identifier: MIT
"""Syntax colours for the generated files in the Studio palette.

One rule set covers every generated language (C/C++, CMake, Make, Python, INI,
JSON). A line starting with '#' is a preprocessor directive when a C directive
follows, otherwise a comment. With ``diff`` set, '+'/'-' lines get the Studio
live/danger tint and the code rules apply after the prefix. Lines of the form
'--- name (state) ---' are file headers.
"""
from __future__ import annotations

from PySide6.QtCore import QRegularExpression
from PySide6.QtGui import QColor, QFont, QSyntaxHighlighter, QTextCharFormat

from . import style

KEYWORDS = ("if else for while do switch case break continue return static const extern struct "
            "typedef enum class public private void sizeof inline volatile unsigned signed "
            "true false nullptr NULL def import from as in not and or None True False "
            "function endfunction endif set include").split()
TYPES = ("int char float double bool size_t uint8_t uint16_t uint32_t uint64_t int8_t int16_t "
         "int32_t int64_t uintptr_t KymoStatus KymoConfig KymoChannel KymoSample KymoContext").split()
DIRECTIVES = {"include", "if", "ifdef", "ifndef", "elif", "else", "endif", "define", "undef",
              "pragma", "error", "warning"}
IN_BLOCK_COMMENT = 1


def _format(color: str, bold: bool = False, italic: bool = False) -> QTextCharFormat:
    result = QTextCharFormat()
    result.setForeground(QColor(color))
    if bold:
        result.setFontWeight(QFont.Weight.Bold)
    result.setFontItalic(italic)
    return result


class CodeHighlighter(QSyntaxHighlighter):
    def __init__(self, document):
        super().__init__(document)
        self.diff = False
        self._background = ""
        self.comment = _format(style.TEXT_HINT, italic=True)
        self.directive = _format(style.WARNING)
        self.string = _format(style.CHIPS["live"][2])
        self.header = _format(style.TEXT, bold=True)
        self.header.setBackground(QColor(style.CONTROL))
        self.rules = [
            (QRegularExpression(rf"\b({'|'.join(KEYWORDS)})\b"), _format(style.PRIMARY)),
            (QRegularExpression(rf"\b({'|'.join(TYPES)})\b"), _format(style.PRIMARY_HOVER, italic=True)),
            (QRegularExpression(r"\bKYMO_[A-Z0-9_]+\b"), _format("#C9A7F5")),
            (QRegularExpression(r"\b(0x[0-9A-Fa-f]+|\d+(\.\d+)?)[uUlLfF]*\b"), _format(style.CHIPS["warning"][2])),
            (QRegularExpression(r'"(\\.|[^"\\])*"'), self.string),
            (QRegularExpression(r"'(\\.|[^'\\])*'"), self.string),
        ]
        self.line_comment = QRegularExpression(r"//.*$")
        self.block_start = QRegularExpression(r"/\*")
        self.block_end = QRegularExpression(r"\*/")
        self.include_target = QRegularExpression(r'[<"][^>"]*[>"]')

    def set_diff(self, diff: bool) -> None:
        """Call before replacing the text: whether +/- prefixes are diff markers."""
        self.diff = diff

    def _mark(self, start: int, length: int, fmt: QTextCharFormat) -> None:
        """Apply fmt and keep the diff tint of the line."""
        if self._background:
            fmt = QTextCharFormat(fmt)
            fmt.setBackground(QColor(self._background))
        self.setFormat(start, length, fmt)

    def highlightBlock(self, text: str) -> None:  # noqa: N802 - Qt override
        self.setCurrentBlockState(0)
        self._background = ""
        if text.startswith("--- ") and text.endswith(" ---"):
            self.setFormat(0, len(text), self.header)
        elif self.diff and text.startswith("@@"):
            self.setFormat(0, len(text), self.comment)
        else:
            offset = 0
            if self.diff and text[:1] in ("+", "-") and not text.startswith(("+++", "---")):
                self._background = style.CHIPS["live" if text[0] == "+" else "danger"][0]
                tint = QTextCharFormat()
                tint.setBackground(QColor(self._background))
                self.setFormat(0, len(text), tint)
                offset = 1
            self._code(text, offset)

    def _code(self, text: str, offset: int) -> None:
        code = text[offset:]
        stripped = code.lstrip()
        start = offset + len(code) - len(stripped)
        if self.previousBlockState() == IN_BLOCK_COMMENT:
            self._block_comments(text, offset, True)
        elif stripped.startswith("#"):
            # "#include" is a directive, "# include(...)" a CMake/Python comment.
            word = stripped[1:].split(" ", 1)[0].split("(", 1)[0]
            if word in DIRECTIVES:
                self._mark(start, len(text) - start, self.directive)
                match = self.include_target.match(text, start)
                if word == "include" and match.hasMatch():
                    self._mark(match.capturedStart(), match.capturedLength(), self.string)
                self._block_comments(text, start, False)
            else:
                self._mark(start, len(text) - start, self.comment)
        elif stripped.startswith(";"):
            self._mark(start, len(text) - start, self.comment)
        else:
            for pattern, fmt in self.rules:
                matches = pattern.globalMatch(text, offset)
                while matches.hasNext():
                    match = matches.next()
                    self._mark(match.capturedStart(), match.capturedLength(), fmt)
            match = self.line_comment.match(text, offset)
            if match.hasMatch():
                self._mark(match.capturedStart(), len(text) - match.capturedStart(), self.comment)
            self._block_comments(text, offset, False)

    def _block_comments(self, text: str, offset: int, open_: bool) -> None:
        """/* ... */ comments, also across lines."""
        if open_:
            begin = offset
            search_from = offset
        else:
            match = self.block_start.match(text, offset)
            begin = match.capturedStart() if match.hasMatch() else -1
            search_from = begin + 2
        while begin >= 0:
            end = self.block_end.match(text, search_from)
            if not end.hasMatch():
                self._mark(begin, len(text) - begin, self.comment)
                self.setCurrentBlockState(IN_BLOCK_COMMENT)
                begin = -1
            else:
                self._mark(begin, end.capturedEnd() - begin, self.comment)
                match = self.block_start.match(text, end.capturedEnd())
                begin = match.capturedStart() if match.hasMatch() else -1
                search_from = begin + 2
