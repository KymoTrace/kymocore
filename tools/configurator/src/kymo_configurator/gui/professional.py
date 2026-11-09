# SPDX-License-Identifier: MIT
"""Professional mode: every section and every setting, free navigation."""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (QHBoxLayout, QLabel, QLineEdit, QListWidget, QListWidgetItem, QSplitter,
                               QStackedWidget, QVBoxLayout, QWidget)

from .. import catalog
from ..i18n import tr
from .inspector import InspectorPanel
from .parts import StatusListDelegate
from .section_page import SectionPage
from .session import ProjectSession


def status_marker(session: ProjectSession, section: catalog.Section) -> str:
    diagnostics = session.diagnostics_for(section)
    if any(d.severity == "error" for d in diagnostics):
        return "✖"
    if diagnostics:
        return "⚠"
    return "✓"


class ProfessionalEditor(QWidget):
    """Section navigation with live summaries next to the selected page."""

    def __init__(self, session: ProjectSession):
        super().__init__()
        self.session = session
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        splitter = QSplitter(Qt.Orientation.Horizontal)
        layout.addWidget(splitter)
        side = QWidget()
        side.setObjectName("sidebar")
        side_layout = QVBoxLayout(side)
        heading = QLabel(tr("Configuration"))
        heading.setObjectName("sidebarTitle")
        side_layout.addWidget(heading)
        self.search = QLineEdit()
        self.search.setObjectName("navSearch")
        self.search.setPlaceholderText(tr("Search settings …"))
        self.search.setClearButtonEnabled(True)
        self.search.textChanged.connect(self._search)
        side_layout.addWidget(self.search)
        self.nav = QListWidget()
        self.nav.setObjectName("navigation")
        self.nav.setWordWrap(True)
        self.nav.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.nav.setItemDelegate(StatusListDelegate(self.nav))
        self.nav.setMouseTracking(True)
        self.nav.setSpacing(1)
        self.nav.currentRowChanged.connect(self._select)
        side_layout.addWidget(self.nav, 1)
        legend = QLabel(tr("● ready   ◆ warning   ■ needs attention"))
        legend.setObjectName("fieldHelp")
        side_layout.addWidget(legend)
        side.setMinimumWidth(240)
        splitter.addWidget(side)
        self.stack = QStackedWidget()
        splitter.addWidget(self.stack)
        self.inspector = InspectorPanel(session)
        self.inspector.navigate.connect(self.open_section)
        splitter.addWidget(self.inspector)
        splitter.setStretchFactor(1, 1)
        splitter.setCollapsible(2, False)
        splitter.setSizes([272, 828, 340])
        splitter.setHandleWidth(1)
        self.pages: dict[str, SectionPage] = {}
        self.keys: list[str] = []
        self.rebuild()
        session.replaced.connect(self.rebuild)
        session.changed.connect(self.refresh)

    def rebuild(self) -> None:
        current = self.current_key()
        while self.stack.count():
            page = self.stack.widget(0)
            self.stack.removeWidget(page)
            page.deleteLater()
        self.nav.blockSignals(True)
        self.nav.clear()
        self.pages = {}
        self.keys = []
        for section in catalog.sections():
            page = SectionPage(section, self.session, guided=False,
                               kicker=tr("SECTION {n} OF {total}", n=len(self.keys) + 1,
                                         total=len(catalog.sections())))
            page.navigate.connect(self.open_section)
            self.pages[section.key] = page
            self.keys.append(section.key)
            self.stack.addWidget(page)
            item = QListWidgetItem(tr(section.title))
            item.setData(Qt.ItemDataRole.UserRole, section.key)
            self.nav.addItem(item)
        self.nav.blockSignals(False)
        self.refresh()
        self.open_section(current if current in self.pages else self.keys[0])

    def refresh(self) -> None:
        view = self.session.view()
        for row, key in enumerate(self.keys):
            section = catalog.section(key)
            item = self.nav.item(row)
            summary = section.summary(view) if section.summary else ""
            marker = status_marker(self.session, section) if key != "review" else "▶"
            item.setText(f"{marker}  {tr(section.title)}" + (f"\n{summary}" if summary else ""))
            item.setHidden(not section.is_visible(view) or not self._matches(section))
            item.setToolTip(tr(section.description))

    def _matches(self, section: catalog.Section) -> bool:
        """Search over section, group and setting names, explanations and C macros."""
        needle = self.search.text().strip().casefold()
        texts = [section.title, section.description]
        for group in section.groups:
            texts.append(group.title)
            for field in group.fields:
                texts += [field.label, field.help, field.guided_help, field.target]
        return not needle or any(needle in tr(t).casefold() or needle in t.casefold() for t in texts if t)

    def _search(self, _text: str = "") -> None:
        self.refresh()
        visible = [row for row in range(self.nav.count()) if not self.nav.item(row).isHidden()]
        if visible and self.nav.currentRow() not in visible:
            self.nav.setCurrentRow(visible[0])

    def current_key(self) -> str:
        row = self.nav.currentRow()
        return self.keys[row] if 0 <= row < len(self.keys) else ""

    def _select(self, row: int) -> None:
        if 0 <= row < len(self.keys):
            self.stack.setCurrentWidget(self.pages[self.keys[row]])
            self.inspector.setVisible(self.keys[row] != "review")

    def open_section(self, key: str) -> None:
        if key in self.keys:
            self.nav.setCurrentRow(self.keys.index(key))
