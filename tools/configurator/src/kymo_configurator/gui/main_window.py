# SPDX-License-Identifier: MIT
"""Main window in the KymoStudio layout: header, start page or mode, status bar."""
from __future__ import annotations

import json
from pathlib import Path

from PySide6.QtCore import QLocale, QSettings, Qt, QTimer
from PySide6.QtGui import QAction, QActionGroup, QKeySequence
from PySide6.QtWidgets import (QComboBox, QFileDialog, QFrame, QHBoxLayout, QInputDialog,
                               QLabel, QMainWindow, QMenu, QMessageBox, QPushButton, QSizePolicy,
                               QStackedWidget, QToolButton, QVBoxLayout, QWidget)

from ..i18n import LANGUAGES, language, set_language, tr
from ..model import CORE_CONFIG_API_VERSION
from ..presets import PRESETS, apply_preset
from .guided import GuidedWizard
from .parts import chip, rule, set_chip
from .professional import ProfessionalEditor
from .session import ProjectSession
from .style import STYLE, palette

GUIDED = "guided"
PROFESSIONAL = "professional"
RECENT_LIMIT = 5


def settings() -> QSettings:
    return QSettings("KymoCore", "Configurator")


def initial_language(requested: str | None = None) -> str:
    """Command line choice, else the saved choice, else German for a German system."""
    saved = settings().value("language", "")
    system = "de" if QLocale.system().name().startswith("de") else "en"
    return next(code for code in (requested, saved, system) if code in LANGUAGES)


def recent_projects() -> list[Path]:
    """Recently opened or saved projects that still exist, newest first."""
    stored = settings().value("recent", []) or []
    stored = [stored] if isinstance(stored, str) else list(stored)
    return [Path(p) for p in stored if Path(p).is_file()][:RECENT_LIMIT]


def remember_project(path: Path) -> None:
    entries = [str(path.resolve())] + [str(p) for p in recent_projects() if p.resolve() != path.resolve()]
    settings().setValue("recent", entries[:RECENT_LIMIT])


def _profile_count(path: Path) -> int:
    try:
        profiles = json.loads(path.read_text(encoding="utf-8")).get("profiles", {})
    except (OSError, ValueError, AttributeError):
        return 0
    return len(profiles) if isinstance(profiles, dict) else 0


class StartPage(QWidget):
    """Choose how to work, reopen a recent project or start from a template."""

    def __init__(self, window: "MainWindow"):
        super().__init__()
        self.setObjectName("page")
        outer = QHBoxLayout(self)
        outer.setContentsMargins(40, 48, 40, 32)
        column = QVBoxLayout()
        column.setSpacing(22)
        holder = QWidget()
        holder.setMaximumWidth(1040)
        holder.setLayout(column)
        outer.addStretch(1)
        outer.addWidget(holder, 100)
        outer.addStretch(1)
        kicker = QLabel(tr("KYMOCORE · CONFIGURATION API {version}", version=CORE_CONFIG_API_VERSION))
        kicker.setObjectName("startKicker")
        title = QLabel(tr("What should KymoCore do in your firmware?"))
        title.setObjectName("startTitle")
        intro = QLabel(tr("Creates the build configuration for KymoCore 7 (configuration API "
                          "{version}): which features are compiled in, which channels are "
                          "sent and which of your functions connect KymoCore to the hardware.",
                          version=CORE_CONFIG_API_VERSION))
        intro.setWordWrap(True)
        intro.setObjectName("sectionIntro")
        for widget in (kicker, title, intro):
            column.addWidget(widget)
        cards = QHBoxLayout()
        cards.setSpacing(16)
        for mode, heading, text, note, button_text in (
            (GUIDED, tr("Guided setup"),
             tr("Step by step: KymoCore asks what you want to achieve and only shows what matters for it. "
                "Advanced options keep safe defaults. Recommended for the first project."),
             tr("5 to 7 steps, depending on the goal"), tr("Start guided setup")),
            (PROFESSIONAL, tr("Professional mode"),
             tr("All settings at once, grouped by topic, with live status, diagnostics and a preview "
                "of the generated files. For users who know KymoCore."),
             tr("You can switch at any time; nothing is lost."), tr("Open professional mode")),
        ):
            card = QFrame()
            card.setObjectName("modeCard")
            card.setProperty("recommended", mode == GUIDED)
            card_layout = QVBoxLayout(card)
            card_layout.setContentsMargins(20, 18, 20, 18)
            card_layout.setSpacing(10)
            head = QHBoxLayout()
            label = QLabel(heading)
            label.setObjectName("modeTitle")
            head.addWidget(label)
            if mode == GUIDED:
                head.addWidget(chip(tr("recommended"), "neutral"))
            head.addStretch(1)
            body = QLabel(text)
            body.setWordWrap(True)
            body.setObjectName("sectionIntro")
            hint = QLabel(note)
            hint.setObjectName("fieldHelp")
            button = QPushButton(button_text)
            button.setObjectName("primaryButton" if mode == GUIDED else "")
            button.setMinimumHeight(34)
            button.clicked.connect(lambda _=False, m=mode: window.set_mode(m))
            card_layout.addLayout(head)
            card_layout.addWidget(body, 1)
            card_layout.addWidget(hint)
            card_layout.addWidget(button)
            cards.addWidget(card)
        column.addLayout(cards)
        lists = QHBoxLayout()
        lists.setSpacing(16)
        lists.addWidget(self._recent_card(window), 3)
        lists.addWidget(self._template_card(window), 2)
        column.addLayout(lists)
        column.addStretch(1)

    @staticmethod
    def _card(title: str) -> tuple[QFrame, QVBoxLayout]:
        card = QFrame()
        card.setObjectName("listCard")
        layout = QVBoxLayout(card)
        layout.setContentsMargins(0, 0, 0, 10)
        layout.setSpacing(0)
        heading = QLabel(title)
        heading.setObjectName("listCardTitle")
        layout.addWidget(heading)
        return card, layout

    def _recent_card(self, window: "MainWindow") -> QFrame:
        card, layout = self._card(tr("Recently opened"))
        recent = recent_projects()
        for path in recent:
            button = QPushButton(f"{path.name}\n{path.parent}")
            button.setObjectName("ghostButton")
            button.setStyleSheet("text-align: left; border: none; padding: 8px 14px;")
            button.setToolTip(tr("{count} profile(s)", count=_profile_count(path)))
            button.clicked.connect(lambda _=False, p=path: window.open_path(p))
            layout.addWidget(button)
        if not recent:
            empty = QLabel(tr("No project opened yet. Projects you open or save appear here."))
            empty.setObjectName("fieldHelp")
            empty.setWordWrap(True)
            empty.setContentsMargins(14, 10, 14, 0)
            layout.addWidget(empty)
        layout.addStretch(1)
        return card

    def _template_card(self, window: "MainWindow") -> QFrame:
        card, layout = self._card(tr("New from template"))
        layout.setContentsMargins(14, 0, 14, 12)
        for key, preset in PRESETS.items():
            button = QPushButton(tr(preset.title))
            button.setStyleSheet("text-align: left; padding: 0 12px;")
            button.setToolTip(tr(preset.description))
            button.clicked.connect(lambda _=False, k=key: window.new_from_template(k))
            hint = QLabel(tr(preset.question))
            hint.setObjectName("fieldHelp")
            hint.setWordWrap(True)
            layout.addSpacing(10)
            layout.addWidget(button)
            layout.addWidget(hint)
        layout.addStretch(1)
        return card


class MainWindow(QMainWindow):
    def __init__(self, path: Path | None = None, mode: str | None = None):
        super().__init__()
        self.resize(1440, 900)
        self.setPalette(palette())
        self.setStyleSheet(STYLE)
        self.session = ProjectSession()
        if path is not None and Path(path).exists():
            self.session.load(path)
        elif path is not None:
            self.session.path = Path(path)
        self._build_ui(mode)
        self.session.changed.connect(self._update_chrome)
        self.session.replaced.connect(self._load_profiles)

    def _build_ui(self, mode: str | None) -> None:
        """(Re)create header, pages and status bar in the current language; the session is kept."""
        for action in self.actions():
            self.removeAction(action)
        self._build_actions()
        central = QWidget()
        outer = QVBoxLayout(central)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.setSpacing(0)
        outer.addWidget(self._build_header())
        self.notice = QLabel("")
        self.notice.setObjectName("notice")
        self.notice.setWordWrap(True)
        outer.addWidget(self.notice)
        self.stack = QStackedWidget()
        outer.addWidget(self.stack, 1)
        self.setCentralWidget(central)
        self._build_status_bar()
        self.start_page = StartPage(self)
        self.guided = GuidedWizard(self.session)
        self.professional = ProfessionalEditor(self.session)
        self.guided.switch_to_professional.connect(self._open_in_professional)
        for page in (self.start_page, self.guided, self.professional):
            self.stack.addWidget(page)
        self._load_profiles()
        if mode in (GUIDED, PROFESSIONAL):
            self.set_mode(mode)
        else:
            self.stack.setCurrentWidget(self.start_page)
            self._show_project_controls(False)
        self._update_chrome()

    # --- chrome -------------------------------------------------------------
    def _action(self, text: str, slot, shortcut=None) -> QAction:
        action = QAction(text, self)
        if shortcut is not None:
            action.setShortcut(shortcut)
        action.triggered.connect(slot)
        self.addAction(action)  # shortcuts work without a menu bar
        return action

    def _build_actions(self) -> None:
        self.new_action = self._action(tr("&New"), self.new_project, QKeySequence.StandardKey.New)
        self.open_action = self._action(tr("&Open…"), self.open_project, QKeySequence.StandardKey.Open)
        self.save_action = self._action(tr("&Save"), self.save, QKeySequence.StandardKey.Save)
        self.save_as_action = self._action(tr("Save &as…"), self.save_as, QKeySequence.StandardKey.SaveAs)
        self.quit_action = self._action(tr("&Quit"), self.close, QKeySequence.StandardKey.Quit)
        modes = QActionGroup(self)
        self.guided_action = QAction(tr("Guided setup"), self)
        self.professional_action = QAction(tr("Professional mode"), self)
        for action, target in ((self.guided_action, GUIDED), (self.professional_action, PROFESSIONAL)):
            action.setCheckable(True)
            modes.addAction(action)
            action.triggered.connect(lambda _=False, m=target: self.set_mode(m))

    def _header_button(self, text: str, slot, name: str = "", tip: str = "") -> QPushButton:
        button = QPushButton(text)
        if name:
            button.setObjectName(name)
        if tip:
            button.setToolTip(tip)
        button.clicked.connect(slot)
        return button

    def _build_header(self) -> QWidget:
        header = QWidget()
        header.setObjectName("header")
        header.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        header.setFixedHeight(48)
        bar = QHBoxLayout(header)
        bar.setContentsMargins(12, 0, 12, 0)
        bar.setSpacing(8)
        title = QLabel(tr("KymoCore Configurator"))
        title.setObjectName("appTitle")
        self.project_label = QLabel("")
        self.project_label.setObjectName("projectName")
        self.dirty_dot = QLabel("●")
        self.dirty_dot.setObjectName("dirtyDot")
        self.dirty_dot.setToolTip(tr("Unsaved changes"))
        for widget in (title, self.project_label, self.dirty_dot):
            bar.addWidget(widget)
        profile_tip = tr("A profile is a named set of settings for one firmware build, e.g. 'development' "
                         "with all measurements and 'production' with KymoCore switched off. "
                         "All steps edit the profile selected here.")
        profile_label = QLabel(tr("Build profile:"))
        profile_label.setObjectName("projectName")
        profile_label.setToolTip(profile_tip)
        self.profile = QComboBox()
        self.profile.setToolTip(profile_tip)
        self.profile.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToContents)
        self.profile.setMinimumContentsLength(12)
        self.profile.currentTextChanged.connect(self.session.set_profile)
        new_profile = self._header_button(tr("New profile…"), self.new_profile, "ghostButton",
                                          tr("Adds a profile that starts as a copy of the selected one."))
        switch = QWidget()
        switch_layout = QHBoxLayout(switch)
        switch_layout.setContentsMargins(0, 0, 0, 0)
        switch_layout.setSpacing(0)
        for action, tip, name in (
                (self.guided_action, tr("Step by step, only what matters for your goal."), "segmentLeft"),
                (self.professional_action, tr("All settings at once."), "segmentRight")):
            button = QToolButton()
            button.setDefaultAction(action)
            button.setToolTip(tip)
            button.setObjectName(name)
            switch_layout.addWidget(button)
        spacer = QWidget()
        spacer.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        open_button = self._header_button(tr("Open"), self.open_project)
        save_button = self._header_button(tr("Save"), self.save)
        generate_button = self._header_button(tr("Generate"), self.show_review, "primaryButton",
                                              tr("Check the configuration and write the generated files."))
        more = QToolButton()
        more.setText("⋯")
        more.setToolTip(tr("More"))
        more.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        more.setMenu(self._more_menu(more))
        self.project_controls = [self.project_label, self.dirty_dot, profile_label, self.profile, new_profile,
                                 switch, save_button, generate_button]
        self._dividers = [rule(vertical=True), rule(vertical=True), rule(vertical=True)]
        for widget in (self._dividers[0], profile_label, self.profile, new_profile, spacer, switch,
                       self._dividers[1], open_button, save_button, generate_button, more):
            bar.addWidget(widget)
        self.project_controls += [self._dividers[0], self._dividers[1]]
        return header

    def _more_menu(self, parent: QWidget) -> QMenu:
        menu = QMenu(parent)
        # Open and Save sit in the header; the menu holds only what the header does not show.
        for action in (self.new_action, self.save_as_action):
            menu.addAction(action)
        menu.addSeparator()
        language_menu = menu.addMenu("Language / Sprache")
        languages = QActionGroup(language_menu)
        for code, name in LANGUAGES.items():
            action = language_menu.addAction(name)
            action.setCheckable(True)
            action.setChecked(code == language())
            languages.addAction(action)
            # Deferred: rebuilding the header inside its own signal would delete the sender.
            action.triggered.connect(lambda _=False, c=code: QTimer.singleShot(0, lambda: self.change_language(c)))
        menu.addSeparator()
        menu.addAction(self.quit_action)
        return menu

    def _build_status_bar(self) -> None:
        bar = self.statusBar()
        bar.setSizeGripEnabled(False)
        for child in bar.findChildren(QWidget, options=Qt.FindChildOption.FindDirectChildrenOnly):
            if isinstance(child, QLabel):
                bar.removeWidget(child)
                child.deleteLater()
        self.status_profile = QLabel("")
        self.status_errors = chip("", "danger")
        self.status_warnings = chip("", "warning")
        version = QLabel(tr("Configuration API {version}", version=CORE_CONFIG_API_VERSION))
        version.setObjectName("statusVersion")
        for widget in (self.status_profile, self.status_errors, self.status_warnings):
            bar.addWidget(widget)
        bar.addPermanentWidget(version)

    def _show_project_controls(self, visible: bool) -> None:
        for widget in self.project_controls:
            widget.setVisible(visible)

    def change_language(self, code: str) -> None:
        if code == language():
            return
        set_language(code)
        settings().setValue("language", code)
        guided_step = self.guided.index
        mode = self.mode()
        self._build_ui(mode)
        self.guided.index = guided_step
        self.guided.refresh()

    def _load_profiles(self) -> None:
        self.profile.blockSignals(True)
        self.profile.clear()
        self.profile.addItems(self.session.profiles)
        self.profile.setCurrentText(self.session.profile)
        self.profile.blockSignals(False)

    def _update_chrome(self) -> None:
        name = self.session.path.name if self.session.path else tr("unsaved project")
        self.setWindowTitle(f"{tr('KymoCore Configurator')} — {name}{' *' if self.session.dirty else ''}")
        self.project_label.setText(name)
        self.dirty_dot.setVisible(self.session.dirty and bool(self.mode()))
        errors = len(self.session.errors())
        warnings = len(self.session.diagnostics()) - errors
        self.status_profile.setText(tr("Profile '{name}'", name=self.session.profile) if self.mode()
                                    else tr("No project opened"))
        if errors:
            set_chip(self.status_errors, tr("{count} problem(s) to fix", count=errors), "danger")
        else:
            set_chip(self.status_errors, tr("ready to generate"), "live")
        self.status_errors.setVisible(bool(self.mode()))
        self.status_warnings.setText(tr("{count} warning(s)", count=warnings))
        self.status_warnings.setVisible(bool(warnings) and bool(self.mode()))
        self.notice.setText("\n".join(self.session.notes))
        self.notice.setVisible(bool(self.session.notes))

    def mode(self) -> str:
        current = self.stack.currentWidget()
        return GUIDED if current is self.guided else PROFESSIONAL if current is self.professional else ""

    def set_mode(self, mode: str) -> None:
        self.stack.setCurrentWidget(self.guided if mode == GUIDED else self.professional)
        self.guided_action.setChecked(mode == GUIDED)
        self.professional_action.setChecked(mode == PROFESSIONAL)
        self._show_project_controls(True)
        self._update_chrome()

    def show_review(self) -> None:
        """Header 'Generate': the review section shows the files and writes them."""
        self._open_in_professional("review")

    def new_profile(self) -> None:
        name, ok = QInputDialog.getText(
            self, tr("New profile"),
            tr("The new profile starts as a copy of '{name}'.\n"
               "Afterwards you can change it without affecting the original.\n\n"
               "Name (e.g. production):", name=self.session.profile))
        name = name.strip()
        if not ok or not name:
            return
        try:
            self.session.duplicate_profile(name)
        except ValueError:
            QMessageBox.warning(self, tr("New profile"),
                                tr("A profile named '{name}' already exists.", name=name))

    def _open_in_professional(self, key: str) -> None:
        self.set_mode(PROFESSIONAL)
        self.professional.open_section(key)

    # --- files --------------------------------------------------------------
    def _ask_save_dirty(self) -> bool:
        if not self.session.dirty:
            return True
        answer = QMessageBox.question(
            self, tr("Unsaved changes"), tr("Save changes before continuing?"),
            QMessageBox.StandardButton.Save | QMessageBox.StandardButton.Discard |
            QMessageBox.StandardButton.Cancel)
        if answer == QMessageBox.StandardButton.Cancel:
            return False
        return self.save() if answer == QMessageBox.StandardButton.Save else True

    def new_project(self) -> None:
        if self._ask_save_dirty():
            self.session.new()

    def new_from_template(self, key: str) -> None:
        if not self._ask_save_dirty():
            return
        self.session.new()
        apply_preset(self.session.doc["profiles"][self.session.profile], key)
        self.session.touch()
        self.set_mode(PROFESSIONAL)

    def open_project(self) -> None:
        if not self._ask_save_dirty():
            return
        name, _ = QFileDialog.getOpenFileName(self, tr("Open Kymo project"), "", tr("Kymo project (*.json)"))
        if name:
            self.open_path(Path(name), ask=False)

    def open_path(self, path: Path, ask: bool = True) -> None:
        if ask and not self._ask_save_dirty():
            return
        try:
            self.session.load(path)
        except ValueError as exc:
            QMessageBox.critical(self, tr("Open failed"), str(exc))
            return
        remember_project(path)
        if not self.mode():
            self.set_mode(PROFESSIONAL)

    def save(self) -> bool:
        if self.session.path is None:
            return self.save_as()
        try:
            self.session.save()
        except OSError as exc:
            QMessageBox.critical(self, tr("Save failed"), str(exc))
            return False
        remember_project(self.session.path)
        return True

    def save_as(self) -> bool:
        name, _ = QFileDialog.getSaveFileName(self, tr("Save Kymo project"), "kymo.project.json",
                                              tr("Kymo project (*.json)"))
        if not name:
            return False
        try:
            self.session.save(name)
        except OSError as exc:
            QMessageBox.critical(self, tr("Save failed"), str(exc))
            return False
        remember_project(Path(name))
        return True

    def closeEvent(self, event) -> None:  # noqa: N802 - Qt override
        if self._ask_save_dirty():
            event.accept()
        else:
            event.ignore()
