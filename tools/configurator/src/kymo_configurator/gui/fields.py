# SPDX-License-Identifier: MIT
"""Generic editors that render catalog fields.

Every editor shows the label, a short technical subtitle, the cost and, when disabled,
why. The plain-language explanation and the affected C macro or binding are in the
tooltip. New field kinds are added in ``_make_input`` only; sections never build
widgets by hand.
"""
from __future__ import annotations

from typing import Any

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (QCheckBox, QComboBox, QFrame, QGridLayout, QHBoxLayout, QLabel,
                               QLineEdit, QPushButton, QSpinBox, QVBoxLayout, QWidget)

from .. import catalog
from ..i18n import tr
from ..presets import (GOAL_FEATURES, PRESETS, apply_goals, goals, set_goal_feature, set_values_goal,
                       values_selected)
from .parts import ToggleSwitch, goal_titles
from .session import ProjectSession


class _Tile(QFrame):
    """Goal tile: a click anywhere on it toggles its checkbox."""

    button: QCheckBox | None = None

    def mouseReleaseEvent(self, event) -> None:  # noqa: N802 - Qt override
        if (self.button is not None and self.button.isEnabled()
                and event.button() == Qt.MouseButton.LeftButton):
            self.button.toggle()
        super().mouseReleaseEvent(event)


def split_label(text: str) -> tuple[str, str]:
    """'Watch memory (stacks and heap)' -> ('Watch memory', 'stacks and heap')."""
    head, sep, tail = text.rpartition(" (")
    split = bool(sep) and text.endswith(")") and bool(head)
    return (head, tail[:-1]) if split else (text, "")


class FieldEditor(QWidget):
    """One setting as a compact row: switch rows, form rows (label left, input right) or a chip.

    The full explanation sits in the tooltip, so the page stays scannable; problems, the
    reason for a disabled field and code examples are shown inline.
    """

    def __init__(self, field: catalog.Field, session: ProjectSession, guided: bool = False,
                 parent: QWidget | None = None, chip: bool = False):
        # Created inside its parent: a parentless widget shown by refresh() would flash as a window.
        super().__init__(parent)
        self.field = field
        self.session = session
        self.guided = guided
        self.chip = chip
        self._updating = False
        self.setObjectName("fieldChip" if chip else "fieldRow")
        self.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        title_text, subtitle_text = split_label(tr(field.label))
        self.title = QLabel(title_text, self)
        self.title.setObjectName("fieldTitle")
        self.title.setWordWrap(True)
        self.subtitle = QLabel(subtitle_text, self)
        self.subtitle.setObjectName("fieldSubtitle")
        self.subtitle.setWordWrap(True)
        self.subtitle.setVisible(bool(subtitle_text))
        self.input = self._make_input()
        self.need = QLabel("", self)
        self.cost = QLabel(tr(field.cost) if field.cost else "", self)
        self.cost.setObjectName("fieldCost")
        self.cost.setVisible(bool(field.cost))
        help_text = tr(field.guided_help if guided and field.guided_help else field.help)
        self.help = QLabel(help_text, self)
        self.help.setObjectName("fieldHelp")
        self.help.setWordWrap(True)
        if field.target and guided:
            tip = help_text + "\n\n" + tr("Technical detail: {target}", target=field.target)
        else:
            tip = f"{field.target}\n\n{help_text}" if field.target else help_text
        self.setToolTip(tip)
        self.title.setToolTip(tip)
        self.example = QLabel("", self)
        self.example.setObjectName("codeExample")
        self.example.setTextFormat(Qt.TextFormat.PlainText)
        self.example.setWordWrap(True)  # long signatures must not widen the page
        self.example.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        self.example.setToolTip(tr("Example of the function in your own code; the name follows your entry."))
        self.example.setVisible(False)
        # Code examples stay folded behind a link so the form stays short.
        self.example_button = QPushButton(tr("Show example"), self)
        self.example_button.setObjectName("linkButton")
        self.example_button.setCheckable(True)
        self.example_button.setVisible(bool(field.example))
        self.example_button.toggled.connect(self._show_example)
        self.reason = QLabel("", self)
        self.reason.setObjectName("fieldReason")
        self.reason.setWordWrap(True)
        self.problem = QLabel("", self)
        self.problem.setObjectName("fieldProblem")
        self.problem.setWordWrap(True)
        outer = QVBoxLayout(self)
        outer.setSpacing(4)
        names = QVBoxLayout()
        names.setSpacing(1)
        names.addWidget(self.title)
        names.addWidget(self.subtitle)
        if chip:
            self._build_chip(outer)
        elif field.kind == "bool":
            self._build_switch_row(outer, names)
        elif field.kind == "preset":
            self._build_stacked(outer, names)
        else:
            self._build_form_row(outer, names)
        self.refresh()

    def _build_chip(self, outer: QVBoxLayout) -> None:
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(self.input)
        for label in (self.title, self.subtitle, self.help, self.cost, self.need):
            label.hide()
        outer.addWidget(self.problem)
        outer.addWidget(self.reason)

    def _build_switch_row(self, outer: QVBoxLayout, names: QVBoxLayout) -> None:
        outer.setContentsMargins(14, 9, 14, 9)
        row = QHBoxLayout()
        row.setSpacing(12)
        row.addWidget(self.input, 0, Qt.AlignmentFlag.AlignVCenter)
        row.addLayout(names, 1)
        row.addWidget(self.need, 0, Qt.AlignmentFlag.AlignVCenter)
        row.addWidget(self.cost, 0, Qt.AlignmentFlag.AlignVCenter)
        outer.addLayout(row)
        self.help.hide()
        for label in (self.example, self.reason, self.problem):
            label.setContentsMargins(42, 0, 0, 0)
            outer.addWidget(label)

    def _build_stacked(self, outer: QVBoxLayout, names: QVBoxLayout) -> None:
        outer.setContentsMargins(14, 10, 14, 10)
        outer.addLayout(names)
        outer.addWidget(self.input)
        for label in (self.help, self.reason, self.problem):
            outer.addWidget(label)
        self.need.hide()

    def _build_form_row(self, outer: QVBoxLayout, names: QVBoxLayout) -> None:
        outer.setContentsMargins(14, 9, 14, 9)
        row = QHBoxLayout()
        row.setSpacing(16)
        left = QWidget()
        left.setFixedWidth(210)
        left.setLayout(names)
        names.setContentsMargins(0, 5, 0, 0)
        row.addWidget(left, 0, Qt.AlignmentFlag.AlignTop)
        right = QVBoxLayout()
        right.setSpacing(4)
        line = QHBoxLayout()
        line.addWidget(self.input, 1)
        line.addWidget(self.need, 0, Qt.AlignmentFlag.AlignVCenter)
        right.addLayout(line)
        # Beginners keep the one-line hint under the input; the professional mode uses the tooltip.
        right.addWidget(self.help)
        self.help.setVisible(self.guided)
        if self.field.example:
            right.addWidget(self.example_button, 0, Qt.AlignmentFlag.AlignLeft)
        for label in (self.example, self.reason, self.problem):
            right.addWidget(label)
        row.addLayout(right, 1)
        outer.addLayout(row)

    def _show_example(self, on: bool) -> None:
        self.example_button.setText(tr("Hide example") if on else tr("Show example"))
        self.example.setVisible(on and self.input.isEnabled())

    def mouseReleaseEvent(self, event) -> None:  # noqa: N802 - Qt override
        """A click anywhere on a switch row toggles it, not only the small switch."""
        if (self.field.kind == "bool" and not self.chip and self.input.isEnabled()
                and event.button() == Qt.MouseButton.LeftButton):
            self.input.toggle()
        super().mouseReleaseEvent(event)

    # --- construction -------------------------------------------------------
    def _make_input(self) -> QWidget:
        kind = self.field.kind
        if kind == "bool" and self.chip:
            widget = QPushButton(split_label(tr(self.field.label))[0])
            widget.setObjectName("chipToggle")
            widget.setCheckable(True)
            widget.toggled.connect(self._commit)
        elif kind == "bool":
            widget = ToggleSwitch()
            widget.toggled.connect(self._commit)
        elif kind == "int" and (self.field.maximum is None or self.field.maximum <= 2147483647):
            widget = QSpinBox()
            widget.setRange(self.field.minimum if self.field.minimum is not None else -2147483647,
                            self.field.maximum if self.field.maximum is not None else 2147483647)
            widget.valueChanged.connect(self._commit)
        elif kind == "choice":
            widget = QComboBox()
            for value, label in self.field.choices:
                widget.addItem(tr(label), value)
            widget.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToMinimumContentsLengthWithIcon)
            widget.setMinimumContentsLength(16)
            widget.currentIndexChanged.connect(self._commit)
        elif kind == "preset":
            widget = self._make_cards()
        else:
            widget = QLineEdit()
            widget.setPlaceholderText(self.field.placeholder or
                                      (tr("optional") if kind == "optional_int" else ""))
            widget.editingFinished.connect(self._commit)
            widget.textEdited.connect(self._commit_text)
        return widget

    def _make_cards(self) -> QWidget:
        """Guided goals as tiles: live values always, every other goal freely combinable."""
        frame = QWidget()
        layout = QGridLayout(frame)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(10)
        self._goals: dict[str, QCheckBox] = {}
        titles = goal_titles()
        cards = (
            ("values", titles["values"],
             tr("Measurements such as temperature or current as curves over time."),
             tr("Each sample carries one number, the Y value. KymoStudio draws it over time.\n"
                "Example: temperature, motor current, battery voltage.\n"
                "Available in every project, also together with the options below.")),
            ("xyz", titles["xyz"],
             tr("A value over X (and Z) instead of over time, e.g. a plotter head position."),
             tr("A sample may carry its own X (and Z) next to the Y value, plus the time it was "
                "measured.\nKymoStudio then draws Y over X, or a point in 3D, instead of Y over time.\n"
                "Example: pen position of a plotter, robot arm position, torque over angle.\n"
                "Compiles in X, Z and timestamps (KYMO_ENABLE_X/_Z/_TIMESTAMP).")),
            ("trace", titles["trace"],
             tr("When functions and interrupts run, as a timeline (timing analysis)."),
             tr("Records when your functions, interrupts and data flows run and shows them on a "
                "timeline in KymoStudio.\nExample: find out why a control loop is sometimes late.\n"
                "Compiles in the trace recorder (KYMO_ENABLE_TRACE).")),
            ("parameters", titles["parameters"],
             tr("Change controller gains or a setpoint from KymoStudio while measuring."),
             tr("You list the adjustable values in your code with KYMO_PARAM; the firmware checks "
                "every value it receives.\nExample: tune a PID controller without flashing again.\n"
                "Compiles in KYMO_ENABLE_PARAMETERS and the frame decoder.")),
            ("watch", titles["watch"],
             tr("Global variables from the ELF file as curves, without code for each one."),
             tr("KymoStudio reads the variable addresses from your firmware's ELF file. Your "
                "firmware decides which memory may be read; nothing is ever written.\n"
                "Compiles in KYMO_ENABLE_WATCH and the frame decoder.")),
            ("memory", titles["memory"],
             tr("See how full your task stacks and your heap get."),
             tr("You list the tasks and heaps in your code and read their values in one callback; "
                "KymoStudio names the curves after them.\nExample: find out whether a task is close "
                "to a stack overflow.\nCompiles in KYMO_ENABLE_MEMORY.")),
        )
        for index, (key, title, text, tip) in enumerate(cards):
            card = _Tile()
            card.setObjectName("presetCard")
            card.setToolTip(tip)
            inner = QVBoxLayout(card)
            button = QCheckBox(title)
            button.setObjectName("presetTitle")
            label = QLabel(text)
            label.setWordWrap(True)
            label.setObjectName("fieldHelp")
            card.button = button
            button.toggled.connect(lambda on, c=card: self._mark_card(c, on))
            inner.addWidget(button)
            inner.addWidget(label)
            inner.addStretch(1)
            layout.addWidget(card, index // 3, index % 3)
            button.toggled.connect(self._commit_goals)
            self._goals[key] = button
        for column in range(3):
            layout.setColumnStretch(column, 1)
        return frame

    @staticmethod
    def _mark_card(card: QFrame, on: bool) -> None:
        card.setProperty("checked", on)
        card.style().unpolish(card)
        card.style().polish(card)

    def _commit_goals(self, *_: Any) -> None:
        if self._updating:
            return
        profile = self.session.view().profile
        before = self._goal_state(profile)
        wanted = {key: box.isChecked() for key, box in self._goals.items()}
        # X/Y points are values with an extra coordinate: they need 'Values over time'.
        if wanted["xyz"] and not before["xyz"]:
            wanted["values"] = True
        if not wanted["values"] and before["values"]:
            wanted["xyz"] = False
        if wanted != before:
            if wanted["values"] != before["values"]:
                set_values_goal(profile, wanted["values"])
            if (wanted["xyz"], wanted["trace"]) != (goals(profile)[0], before["trace"]):
                apply_goals(profile, wanted["xyz"], wanted["trace"])
            for key in GOAL_FEATURES:
                if wanted[key] != before[key]:
                    set_goal_feature(profile, key, wanted[key])
            self.session.touch()

    @staticmethod
    def _goal_state(profile: dict[str, Any]) -> dict[str, bool]:
        xyz, trace = goals(profile)
        features = profile.get("features", {})
        state = {"values": values_selected(profile), "xyz": xyz, "trace": trace}
        state.update({key: bool(features.get(key)) for key in GOAL_FEATURES})
        return state

    # --- data flow ----------------------------------------------------------
    def current_input(self) -> Any:
        kind = self.field.kind
        widget = self.input
        if kind == "bool":
            return widget.isChecked()
        if isinstance(widget, QSpinBox):
            return widget.value()
        if kind == "preset":
            return self.session.value(self.field)
        if kind == "choice":
            return widget.currentData()
        text = widget.text().strip()
        if kind in ("int", "optional_int"):
            if not text and kind == "optional_int":
                return None
            try:
                return int(text)
            except ValueError:
                return text
        return text

    def set_input(self, value: Any) -> None:
        kind = self.field.kind
        widget = self.input
        if kind == "bool":
            widget.setChecked(bool(value))
        elif isinstance(widget, QSpinBox):
            number = value if type(value) is int else self.field.default or 0
            widget.setValue(max(widget.minimum(), min(widget.maximum(), number)))
        elif kind == "preset":
            state = self._goal_state(self.session.view().profile)
            for key, box in self._goals.items():
                box.setChecked(state[key])
        elif kind == "choice":
            index = widget.findData(value)
            widget.setCurrentIndex(index)
        else:
            text = "" if value is None else str(value)
            if widget.text() != text:
                widget.setText(text)

    def _commit_text(self, *_: Any) -> None:
        """Store text immediately so dependent editors update while typing."""
        self._commit()

    def _commit(self, *_: Any) -> None:
        if self._updating:
            return
        value = self.current_input()
        if value != self.session.value(self.field):
            self.session.set_value(self.field, value)

    def refresh(self) -> None:
        """Pull value, enabled state and diagnostics from the session."""
        view = self.session.view()
        self._updating = True
        try:
            if not (isinstance(self.input, QLineEdit) and self.input.hasFocus()):
                self.set_input(self.field.value(view))
        finally:
            self._updating = False
        enabled = self.field.is_enabled(view)
        self.input.setEnabled(enabled)
        self.title.setEnabled(enabled)
        if self.field.kind == "bool":
            # Switched-off features read greyed out, so the active ones stand out.
            off = not self.input.isChecked()
            for label in (self.title, self.subtitle, self.help):
                if label.property("off") != off:
                    label.setProperty("off", off)
                    label.style().unpolish(label)
                    label.style().polish(label)
        self.reason.setText("" if enabled else tr(self.field.requires))
        self.reason.setVisible(not enabled and bool(self.field.requires))
        required = bool(self.field.need(view)) if self.field.need is not None else None
        self.need.setText(tr("Required") if required else tr("Optional"))
        self.need.setObjectName("needRequired" if required else "needOptional")
        self.need.setStyleSheet("")  # re-apply the window style for the new object name
        self.need.setVisible(enabled and required is not None)
        if self.field.example:
            name = str(self.field.value(view) or self.field.placeholder or "my_function")
            self.example.setText(tr("In your code, for example:") + "\n" +
                                 self.field.example.replace("{name}", name))
            self.example_button.setVisible(enabled)
            self.example.setVisible(enabled and self.example_button.isChecked())
        self.setVisible(self.field.is_visible(view))
        path = self.session.concrete_path(self.field)
        problems = [d for d in self.session.diagnostics() if d.path == path]
        lines = []
        for d in problems:
            message, suggestion = d.localized()
            lines.append(("⚠ " if d.severity != "error" else "✖ ") + message +
                         (f" — {suggestion}" if suggestion else ""))
        text = "\n".join(lines)
        invalid = any(d.severity == "error" for d in problems)
        if isinstance(self.input, QLineEdit) and self.input.property("invalid") != invalid:
            self.input.setProperty("invalid", invalid)
            self.input.style().unpolish(self.input)
            self.input.style().polish(self.input)
        self.problem.setText(text)
        self.problem.setVisible(bool(text))
