# SPDX-License-Identifier: MIT
"""Guided mode: one question per step, advanced settings keep defaults.

The steps are the catalog sections that apply to the current goal; for
example the tracing step only appears when tracing is part of the goal. A step
with open errors cannot be left forward, so the final review is reachable only
with a consistent configuration.
"""
from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (QFrame, QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QProgressBar,
                               QPushButton, QStackedWidget, QVBoxLayout, QWidget)

from .. import catalog
from ..i18n import tr
from ..resources import estimate_ram
from .parts import StatusListDelegate
from .section_page import SectionPage
from .session import ProjectSession


class GuidedWizard(QWidget):
    """Step list on the left, the current question on the right."""

    switch_to_professional = Signal(str)

    def __init__(self, session: ProjectSession):
        super().__init__()
        self.session = session
        self.index = 0
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        side = QWidget()
        side.setObjectName("sidebar")
        side.setFixedWidth(272)
        side_layout = QVBoxLayout(side)
        side_layout.setContentsMargins(12, 16, 12, 12)
        heading = QLabel(tr("Guided setup"))
        heading.setObjectName("sidebarTitle")
        side_layout.addWidget(heading)
        self.steps = QListWidget()
        self.steps.setObjectName("navigation")
        self.steps.setItemDelegate(StatusListDelegate(self.steps, numbered=True))
        self.steps.setMouseTracking(True)
        self.steps.setSpacing(1)
        self.steps.itemClicked.connect(self._step_clicked)
        side_layout.addWidget(self.steps, 1)
        hint = QLabel(tr("You can switch to the professional mode at any time; nothing is lost."))
        hint.setObjectName("fieldHelp")
        hint.setWordWrap(True)
        side_layout.addWidget(hint)
        layout.addWidget(side)
        right = QVBoxLayout()
        right.setContentsMargins(0, 0, 0, 0)
        right.setSpacing(0)
        progress_row = QHBoxLayout()
        progress_row.setContentsMargins(28, 18, 28, 0)
        self.progress = QLabel("")
        self.progress.setObjectName("progress")
        self.progress_bar = QProgressBar()
        self.progress_bar.setTextVisible(False)
        progress_row.addWidget(self.progress)
        progress_row.addWidget(self.progress_bar, 1)
        right.addLayout(progress_row)
        self.stack = QStackedWidget()
        right.addWidget(self.stack, 1)
        self.message = QLabel("")
        self.message.setObjectName("fieldProblem")
        self.message.setWordWrap(True)
        self.message.setContentsMargins(28, 0, 28, 6)
        right.addWidget(self.message)
        footer = QFrame()
        footer.setObjectName("footerBar")
        buttons = QHBoxLayout(footer)
        buttons.setContentsMargins(28, 12, 28, 14)
        self.professional_button = QPushButton(tr("Open in professional mode"))
        self.professional_button.setObjectName("ghostButton")
        self.professional_button.clicked.connect(
            lambda: self.switch_to_professional.emit(self.current_section().key))
        self.back_button = QPushButton(tr("← Back"))
        self.next_button = QPushButton(tr("Next →"))
        self.next_button.setObjectName("primaryButton")
        self.back_button.clicked.connect(self.back)
        self.next_button.clicked.connect(self.next)
        buttons.addWidget(self.professional_button)
        buttons.addSpacing(16)
        self.ram = QLabel("")
        self.ram.setObjectName("footerInfo")
        buttons.addWidget(self.ram)
        buttons.addStretch(1)
        buttons.addWidget(self.back_button)
        buttons.addWidget(self.next_button)
        for button in (self.back_button, self.next_button):
            button.setMinimumHeight(34)
        right.addWidget(footer)
        holder = QWidget()
        holder.setLayout(right)
        layout.addWidget(holder, 1)
        self.pages: dict[str, SectionPage] = {}
        session.replaced.connect(self.rebuild)
        session.changed.connect(self.refresh)
        self.rebuild()

    # --- steps --------------------------------------------------------------
    def step_sections(self) -> list[catalog.Section]:
        view = self.session.view()
        return [s for s in catalog.sections() if s.in_guided(view)]

    def current_section(self) -> catalog.Section:
        sections = self.step_sections()
        self.index = max(0, min(self.index, len(sections) - 1))
        return sections[self.index]

    def rebuild(self) -> None:
        while self.stack.count():
            page = self.stack.widget(0)
            self.stack.removeWidget(page)
            page.deleteLater()
        self.pages = {}
        for section in catalog.sections():
            page = SectionPage(section, self.session, guided=True)
            page.navigate.connect(self.open_section)
            self.pages[section.key] = page
            self.stack.addWidget(page)
        self.index = 0
        self.refresh()

    def refresh(self) -> None:
        sections = self.step_sections()
        current = self.current_section()
        view = self.session.view()
        self.stack.setCurrentWidget(self.pages[current.key])
        self.steps.blockSignals(True)
        self.steps.clear()
        for number, section in enumerate(sections):
            if number < self.index:
                marker = "✓"
            elif number == self.index:
                marker = "▶"
            else:
                marker = "○"
            summary = section.summary(view) if section.summary and number < self.index else ""
            name = tr(section.guided_nav or section.title)
            item = QListWidgetItem(f"{marker}  {number + 1}. {name}" + (f"\n{summary}" if summary else ""))
            item.setData(Qt.ItemDataRole.UserRole, number)
            if number == self.index:
                font = item.font()
                font.setBold(True)
                item.setFont(font)
            self.steps.addItem(item)
        self.steps.blockSignals(False)
        self.progress.setText(tr("Step {n} of {total}", n=self.index + 1, total=len(sections)).upper())
        self.progress_bar.setRange(0, len(sections))
        self.progress_bar.setValue(self.index + 1)
        self.back_button.setEnabled(self.index > 0)
        last = self.index == len(sections) - 1
        self.next_button.setVisible(not last)
        self.message.setText("")
        total = sum(estimate_ram(self.session.doc, self.session.profile).values())
        self.ram.setText(tr("RAM estimate: {bytes} B", bytes=f"{total:,}".replace(",", " ")))

    def blocking_errors(self) -> list:
        return [d for d in self.session.diagnostics_for(self.current_section()) if d.severity == "error"]

    def next(self) -> bool:
        errors = self.blocking_errors()
        if errors:
            self.message.setText(tr("Please fix this first: ") + "; ".join(d.localized()[0] for d in errors[:3]))
            return False
        self.index += 1
        self.refresh()
        return True

    def back(self) -> None:
        self.index = max(0, self.index - 1)
        self.refresh()

    def _step_clicked(self, item: QListWidgetItem) -> None:
        target = item.data(Qt.ItemDataRole.UserRole)
        if target <= self.index:
            self.index = target
            self.refresh()
        else:
            while self.index < target and self.next():
                pass

    def open_section(self, key: str) -> None:
        keys = [s.key for s in self.step_sections()]
        if key in keys:
            self.index = keys.index(key)
            self.refresh()
        else:
            self.switch_to_professional.emit(key)
