# SPDX-License-Identifier: MIT
"""Render one catalog section; used by both modes."""
from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import QFrame, QHBoxLayout, QLabel, QScrollArea, QToolButton, QVBoxLayout, QWidget

from .. import catalog
from ..i18n import tr
from ..presets import goal_selected
from .fields import FieldEditor
from .parts import chip, goal_titles
from .session import ProjectSession
from .widgets import create_widget


class GroupCard(QFrame):
    """A field group as a Studio card: header with fold arrow and switch count, then the body."""

    def __init__(self, title: str, goal: str = ""):
        super().__init__()
        self.setObjectName("groupFrame")
        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.setSpacing(0)
        header = QFrame()
        header.setObjectName("groupHeader")
        header.setFixedHeight(40)
        row = QHBoxLayout(header)
        row.setContentsMargins(10, 0, 14, 0)
        self.toggle = QToolButton()
        self.toggle.setObjectName("groupToggle")
        self.toggle.setArrowType(Qt.ArrowType.DownArrow)
        self.toggle.setCheckable(True)
        self.toggle.setChecked(True)
        self.toggle.setToolTip(tr("Show or hide this group"))
        self.toggle.toggled.connect(self._fold)
        heading = QLabel(title)
        heading.setObjectName("groupTitle")
        self.count = QLabel("")
        self.count.setObjectName("groupCount")
        row.addWidget(self.toggle)
        row.addWidget(heading)
        if goal:
            row.addSpacing(6)
            goal_chip = chip(goal_titles()[goal], "neutral")
            goal_chip.setToolTip(tr("Belongs to this goal from step 1"))
            row.addWidget(goal_chip, 0, Qt.AlignmentFlag.AlignVCenter)
        row.addStretch(1)
        row.addWidget(self.count)
        outer.addWidget(header)
        self.body = QWidget()
        self.body.setObjectName("groupBody")
        self.layout_body = QVBoxLayout(self.body)
        self.layout_body.setContentsMargins(0, 0, 0, 4)
        self.layout_body.setSpacing(0)
        outer.addWidget(self.body)

    def _fold(self, open_: bool) -> None:
        self.toggle.setArrowType(Qt.ArrowType.DownArrow if open_ else Qt.ArrowType.RightArrow)
        self.body.setVisible(open_)


class SectionPage(QScrollArea):
    """Header, explained field groups and custom widgets of a section."""

    navigate = Signal(str)

    def __init__(self, section: catalog.Section, session: ProjectSession, guided: bool = False,
                 kicker: str = ""):
        super().__init__()
        self.section = section
        self.session = session
        self.guided = guided
        self.setWidgetResizable(True)
        self.setFrameShape(QFrame.Shape.NoFrame)
        body = QWidget()
        body.setObjectName("sectionBody")
        layout = QVBoxLayout(body)
        layout.setContentsMargins(28, 20, 28, 20)
        layout.setSpacing(14)
        header = QVBoxLayout()
        header.setSpacing(4)
        if kicker:
            label = QLabel(kicker)
            label.setObjectName("sectionKicker")
            header.addWidget(label)
        title = QLabel(tr(section.guided_title if guided and section.guided_title else section.title))
        title.setObjectName("sectionTitle")
        title.setWordWrap(True)
        header.addWidget(title)
        intro = QLabel(tr(section.guided_intro if guided and section.guided_intro else section.description))
        intro.setObjectName("sectionIntro")
        intro.setWordWrap(True)
        header.addWidget(intro)
        layout.addLayout(header)
        self.editors: dict[str, FieldEditor] = {}
        self.groups: list[tuple[catalog.Group, GroupCard]] = []
        for group in section.groups:
            if guided and group.advanced:
                continue
            fields = [f for f in group.fields if not (guided and f.advanced)]
            if not fields:
                continue
            frame = GroupCard(tr(group.title), group.goal)
            description = group.guided_description if guided and group.guided_description else group.description
            if description:
                note = QLabel(tr(description))
                note.setObjectName("groupNote")
                note.setWordWrap(True)
                note.setContentsMargins(14, 8, 14, 6)
                frame.layout_body.addWidget(note)
            self._add_fields(frame, group, fields)
            layout.addWidget(frame)
            self.groups.append((group, frame))
            frame.locked = None
        self.widgets: list[QWidget] = []
        self._layout = layout
        for name in section.widgets:
            widget = create_widget(name, session, guided)
            if hasattr(widget, "navigate"):
                widget.navigate.connect(self.navigate)
            self.widgets.append(widget)
            layout.addWidget(widget, 1 if name == "review" else 0)
        if guided:
            hidden = sum(1 for g in section.groups for f in g.fields if f.advanced or g.advanced)
            if hidden:
                more = QLabel(tr("{count} rarely needed setting(s) are hidden here and keep safe "
                                 "defaults. The professional mode shows them.", count=hidden))
                more.setObjectName("hiddenHint")
                more.setWordWrap(True)
                layout.addWidget(more)
        if "review" not in section.widgets:
            layout.addStretch(1)
        if guided:
            # Guided pages read like a form: centred column of limited width.
            body.setMaximumWidth(980)
            holder = QWidget()
            holder.setObjectName("sectionBody")
            centre = QHBoxLayout(holder)
            centre.setContentsMargins(0, 0, 0, 0)
            centre.addStretch(1)
            centre.addWidget(body, 100)
            centre.addStretch(1)
            body = holder
        self.setWidget(body)
        session.changed.connect(self.refresh)
        self.refresh()

    def _add_fields(self, frame: GroupCard, group: catalog.Group, fields: list) -> None:
        """Rows by default; 'chips' groups as one toggle row; two switches side by side."""
        if group.display == "chips":
            line = QWidget()
            row = QHBoxLayout(line)
            row.setContentsMargins(14, 10, 14, 10)
            row.setSpacing(8)
            caption = QLabel(tr("Send along:"))
            caption.setObjectName("fieldSubtitle")
            row.addWidget(caption)
            for field in fields:
                editor = FieldEditor(field, self.session, self.guided, line, chip=True)
                self.editors[field.key] = editor
                row.addWidget(editor, 0, Qt.AlignmentFlag.AlignTop)
            row.addStretch(1)
            frame.layout_body.addWidget(line)
        elif len(fields) == 2 and all(f.kind == "bool" for f in fields):
            pair = QWidget()
            row = QHBoxLayout(pair)
            row.setContentsMargins(0, 0, 0, 0)
            row.setSpacing(0)
            for field in fields:
                editor = FieldEditor(field, self.session, self.guided, pair)
                self.editors[field.key] = editor
                row.addWidget(editor, 1)
            frame.layout_body.addWidget(pair)
        else:
            for field in fields:
                editor = FieldEditor(field, self.session, self.guided, frame.body)
                self.editors[field.key] = editor
                frame.layout_body.addWidget(editor)

    def refresh(self) -> None:
        view = self.session.view()
        for editor in self.editors.values():
            editor.refresh()
        for group, frame in self.groups:
            frame.setVisible(group.visible_when is None or bool(group.visible_when(view)))
            # Guided mode: a group whose step-1 goal is not chosen is switched off and closed,
            # so nothing can be added past the goals.
            locked = self.guided and bool(group.goal) and not goal_selected(view.profile, group.goal)
            if locked != frame.locked:
                frame.locked = locked
                frame.toggle.setChecked(not locked)
                frame.toggle.setEnabled(not locked)
            if locked:
                frame.count.setText(tr("Off: choose '{goal}' in step 1", goal=goal_titles()[group.goal]))
                continue
            switches = [self.editors[f.key] for f in group.fields
                        if f.key in self.editors and f.kind == "bool" and f.is_visible(view)]
            on = sum(1 for e in switches if e.input.isChecked())
            frame.count.setText(tr("{on} of {total} on", on=on, total=len(switches)) if len(switches) > 1 else "")
        for widget in self.widgets:
            refresh = getattr(widget, "refresh", None)
            if refresh:
                refresh()
