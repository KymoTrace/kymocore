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
        ]
        self.tokens = QRegularExpression(
            r"""(?<string>"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|(?<line>//.*$)|/\*""")
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
        length = len(text.encode("utf-16-le")) // 2
        if text.startswith("--- ") and text.endswith(" ---"):
            self.setFormat(0, length, self.header)
        elif self.diff and text.startswith("@@"):
            self.setFormat(0, length, self.comment)
        else:
            offset = 0
            if self.diff and text[:1] in ("+", "-") and not text.startswith(("+++ ", "--- ")):
                self._background = style.CHIPS["live" if text[0] == "+" else "danger"][0]
                tint = QTextCharFormat()
                tint.setBackground(QColor(self._background))
                self.setFormat(0, length, tint)
                offset = 1
            self._code(text, offset, length)

    def _code(self, text: str, offset: int, length: int) -> None:
        code = text[offset:]
        stripped = code.lstrip()
        start = offset + len(code) - len(stripped)
        open_comment = self.previousBlockState() == IN_BLOCK_COMMENT
        if not open_comment and stripped.startswith("#"):
            # "#include" is a directive, "# include(...)" a CMake/Python comment.
            word = stripped[1:].split(" ", 1)[0].split("(", 1)[0]
            if word in DIRECTIVES:
                self._mark(start, length - start, self.directive)
                match = self.include_target.match(text, start)
                if word == "include" and match.hasMatch():
                    self._mark(match.capturedStart(), match.capturedLength(), self.string)
            else:
                self._mark(start, length - start, self.comment)
                return
        elif not open_comment and stripped.startswith(";"):
            self._mark(start, length - start, self.comment)
            return
        else:
            for pattern, fmt in self.rules:
                matches = pattern.globalMatch(text, offset)
                while matches.hasNext():
                    match = matches.next()
                    self._mark(match.capturedStart(), match.capturedLength(), fmt)
        self._strings_and_comments(text, offset, length, open_comment)

    def _strings_and_comments(self, text: str, offset: int, length: int, open_comment: bool) -> None:
        while offset <= length:
            if open_comment:
                start, search_from = offset, offset
            else:
                match = self.tokens.match(text, offset)
                if not match.hasMatch():
                    break
                start, search_from = match.capturedStart(), match.capturedEnd()
                if match.capturedLength("string") > 0:
                    self._mark(start, match.capturedLength(), self.string)
                    offset = search_from
                    continue
                if match.capturedLength("line") > 0:
                    self._mark(start, match.capturedLength(), self.comment)
                    break
            end = self.block_end.match(text, search_from)
            if not end.hasMatch():
                self._mark(start, length - start, self.comment)
                self.setCurrentBlockState(IN_BLOCK_COMMENT)
                break
            offset = end.capturedEnd()
            self._mark(start, offset - start, self.comment)
            open_comment = False
