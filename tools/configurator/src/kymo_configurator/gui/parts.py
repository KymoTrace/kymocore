# SPDX-License-Identifier: MIT
"""Small painted parts in the KymoStudio look that style sheets cannot draw."""
from __future__ import annotations

from PySide6.QtCore import QPointF, QRectF, QSize, Qt
from PySide6.QtGui import QColor, QFont, QPainter, QPainterPath, QPen, QPolygonF
from PySide6.QtWidgets import (QCheckBox, QFrame, QLabel, QStyle, QStyledItemDelegate,
                               QStyleOptionViewItem, QWidget)

from ..i18n import tr
from . import style

STATUS_COLORS = {"✓": style.SUCCESS, "⚠": style.WARNING, "✖": style.DANGER, "▶": style.PRIMARY}


class ToggleSwitch(QCheckBox):
    """A QCheckBox drawn as the Studio switch; the API stays QCheckBox."""

    def sizeHint(self) -> QSize:  # noqa: N802 - Qt override
        return QSize(30, 18)

    def hitButton(self, pos) -> bool:  # noqa: N802 - Qt override
        return self.rect().contains(pos)

    def paintEvent(self, _event) -> None:  # noqa: N802 - Qt override
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        on = self.isChecked()
        track = QColor(style.PRIMARY if on else style.BORDER_STRONG)
        knob = QColor(style.PRIMARY_TEXT if on else style.TEXT_HINT)
        if not self.isEnabled():
            track.setAlpha(90)
            knob.setAlpha(140)
        rect = QRectF(0, 1, 30, 16)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(track)
        painter.drawRoundedRect(rect, 8, 8)
        painter.setBrush(knob)
        painter.drawEllipse(QRectF(16 if on else 2, 3, 12, 12))
        if self.hasFocus():
            painter.setBrush(Qt.BrushStyle.NoBrush)
            painter.setPen(QPen(QColor(style.PRIMARY_HOVER), 1))
            painter.drawRoundedRect(rect.adjusted(-1, -1, 1, 1), 9, 9)


def draw_marker(painter: QPainter, marker: str, center: QPointF, size: float = 8.0) -> None:
    """Status shapes that differ in form as well as colour: dot, diamond, square, triangle."""
    color = QColor(STATUS_COLORS.get(marker, style.TEXT_HINT))
    painter.setPen(Qt.PenStyle.NoPen)
    painter.setBrush(color)
    half = size / 2
    if marker == "⚠":
        painter.drawPolygon(QPolygonF([center + QPointF(0, -half - 1), center + QPointF(half + 1, 0),
                                       center + QPointF(0, half + 1), center + QPointF(-half - 1, 0)]))
    elif marker == "✖":
        painter.drawRect(QRectF(center.x() - half, center.y() - half, size, size))
    elif marker == "▶":
        painter.drawPolygon(QPolygonF([center + QPointF(-half + 1, -half), center + QPointF(half, 0),
                                       center + QPointF(-half + 1, half)]))
    else:
        painter.drawEllipse(center, half, half)


class StatusListDelegate(QStyledItemDelegate):
    """Paints navigation rows as in the mock-up.

    Item text keeps the plain form "<marker>  <title>\\n<summary>" so it stays
    readable for tests and screen readers; the delegate turns the marker into a
    shape (or a numbered circle for guided steps) and draws the summary smaller.
    """

    def __init__(self, parent: QWidget | None = None, numbered: bool = False):
        super().__init__(parent)
        self.numbered = numbered

    @staticmethod
    def _split(text: str) -> tuple[str, str, str]:
        first, _, summary = text.partition("\n")
        marker, _, title = first.partition("  ")
        return marker, title, summary

    def sizeHint(self, option: QStyleOptionViewItem, index) -> QSize:  # noqa: N802 - Qt override
        _, _, summary = self._split(index.data() or "")
        return QSize(option.rect.width(), 52 if summary else 36)

    def paint(self, painter: QPainter, option: QStyleOptionViewItem, index) -> None:
        marker, title, summary = self._split(index.data() or "")
        selected = bool(option.state & QStyle.StateFlag.State_Selected)
        hovered = bool(option.state & QStyle.StateFlag.State_MouseOver)
        current = selected or (self.numbered and marker == "▶")
        rect = QRectF(option.rect).adjusted(0, 1, -2, -1)
        painter.save()
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        if current:
            painter.setPen(QPen(QColor(style.PRIMARY_BORDER), 1))
            painter.setBrush(QColor(style.PRIMARY_SOFT))
            painter.drawRoundedRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), 6, 6)
        elif hovered:
            painter.setPen(Qt.PenStyle.NoPen)
            painter.setBrush(QColor(style.CONTROL))
            painter.drawRoundedRect(rect, 6, 6)
        top = rect.top() + (10 if summary else rect.height() / 2 - 8)
        if self.numbered:
            self._paint_step(painter, marker, title, QPointF(rect.left() + 19, rect.center().y()))
            title = title.split(". ", 1)[-1]
            left = rect.left() + 38
        else:
            draw_marker(painter, marker, QPointF(rect.left() + 14, top + 8))
            left = rect.left() + 28
        font = QFont(option.font)
        font.setBold(current)
        painter.setFont(font)
        painter.setPen(QColor(style.TEXT if marker != "○" else style.TEXT_HINT))
        width = rect.right() - left - 6
        metrics = painter.fontMetrics()
        painter.drawText(QRectF(left, top, width, 18), Qt.AlignmentFlag.AlignVCenter,
                         metrics.elidedText(title, Qt.TextElideMode.ElideRight, int(width)))
        if summary:
            small = QFont(option.font)
            small.setPixelSize(12)
            painter.setFont(small)
            color = style.CHIPS["danger"][2] if marker == "✖" else (
                style.TEXT_SECONDARY if current else style.TEXT_HINT)
            painter.setPen(QColor(color))
            painter.drawText(QRectF(left, top + 18, width, 16), Qt.AlignmentFlag.AlignVCenter,
                             painter.fontMetrics().elidedText(summary, Qt.TextElideMode.ElideRight, int(width)))
        painter.restore()

    @staticmethod
    def _paint_step(painter: QPainter, marker: str, title: str, center: QPointF) -> None:
        number = title.split(".", 1)[0]
        circle = QRectF(center.x() - 11, center.y() - 11, 22, 22)
        small = QFont(painter.font())
        small.setPixelSize(11)
        small.setBold(True)
        painter.setFont(small)
        if marker == "✓":
            bg, border, text, label = (*style.CHIPS["live"], "✓")
        elif marker == "▶":
            bg, border, text, label = style.PRIMARY, style.PRIMARY, style.PRIMARY_TEXT, number
        else:
            bg, border, text, label = style.PANEL, style.BORDER_STRONG, style.TEXT_HINT, number
        painter.setPen(QPen(QColor(border), 1))
        painter.setBrush(QColor(bg))
        painter.drawEllipse(circle)
        painter.setPen(QColor(text))
        painter.drawText(circle, Qt.AlignmentFlag.AlignCenter, label)


def goal_titles() -> dict[str, str]:
    """Names of the step-1 goals; every group that belongs to a goal shows the same name."""
    return {
        "values": tr("Values over time"),
        "xyz": tr("X/Y or X/Y/Z points"),
        "trace": tr("Execution tracing"),
        "parameters": tr("Adjust values live"),
        "watch": tr("Watch variables"),
        "memory": tr("Watch memory (stacks and heap)"),
        "all": tr("All goals"),
    }


def chip(text: str, kind: str = "neutral") -> QLabel:
    """Rounded status label in one of style.CHIPS."""
    label = QLabel(text)
    label.setObjectName(f"chip_{kind}")
    return label


def set_chip(label: QLabel, text: str, kind: str) -> None:
    label.setText(text)
    if label.objectName() != f"chip_{kind}":
        label.setObjectName(f"chip_{kind}")
        label.style().unpolish(label)
        label.style().polish(label)


def rule(vertical: bool = False) -> QFrame:
    """One-pixel divider in the border colour."""
    line = QFrame()
    line.setObjectName("headerDivider" if vertical else "inspectorRule")
    if vertical:
        line.setFixedSize(1, 22)
    return line


class StackedBar(QWidget):
    """Horizontal bar of (fraction, colour) parts on the control background."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        self.parts: list[tuple[float, str]] = []
        self.setFixedHeight(8)

    def set_parts(self, parts: list[tuple[float, str]]) -> None:
        self.parts = parts
        self.update()

    def paintEvent(self, _event) -> None:  # noqa: N802 - Qt override
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        rect = QRectF(self.rect())
        clip = QPainterPath()
        clip.addRoundedRect(rect, 4, 4)
        painter.setClipPath(clip)
        painter.fillRect(rect, QColor(style.CONTROL))
        x = 0.0
        for fraction, color in self.parts:
            width = max(0.0, min(fraction, 1.0 - x / rect.width())) * rect.width()
            painter.fillRect(QRectF(x, 0, width, rect.height()), QColor(color))
            x += width
