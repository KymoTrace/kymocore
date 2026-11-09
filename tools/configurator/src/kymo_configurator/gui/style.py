# SPDX-License-Identifier: MIT
"""Application stylesheet and palette with the KymoStudio palette (qml/content/Theme/AppTheme.qml).

The colour names below match AppTheme so both tools stay in step when the
Studio theme changes. Shapes Qt style sheets cannot draw (switch, status dots)
are painted in widgets.py and professional.py with the same constants.
"""

BACKGROUND = "#0B0F14"
PANEL = "#11161D"
CARD = "#151B23"
CONTROL = "#1B222C"
CONTROL_HOVER = "#222A35"
BORDER = "#252D38"
BORDER_SUBTLE = "#1E2630"
BORDER_STRONG = "#313B48"
TEXT = "#E7ECF3"
TEXT_SECONDARY = "#A3AEBF"
TEXT_HINT = "#7D899B"
TEXT_LABEL = "#C9D2DE"
TEXT_DISABLED = "#5E6A7B"
PRIMARY = "#4DB6F0"
PRIMARY_HOVER = "#6BC3F3"
PRIMARY_PRESSED = "#3AA3DD"
PRIMARY_BORDER = "#2B5878"
PRIMARY_SOFT = "#13283A"
PRIMARY_TEXT = "#06121B"
SUCCESS = "#46C489"
WARNING = "#EDB248"
DANGER = "#EF6F6C"
MONO = "Consolas, 'IBM Plex Mono', monospace"
FONT = "'Segoe UI', 'IBM Plex Sans', sans-serif"

# Status chips: (background, border, text) as in AppTheme.status.
CHIPS = {
    "live": ("#10231A", "#1F4A34", "#9BE0BC"),
    "warning": ("#2A2210", "#5A4719", "#F5D9A0"),
    "danger": ("#2A1515", "#5C2626", "#F6C3C1"),
    "neutral": (CONTROL, BORDER_STRONG, TEXT_SECONDARY),
}


def palette():
    """Dark palette for the parts style sheets leave to the palette (viewports, dialogs)."""
    from PySide6.QtGui import QColor, QPalette
    result = QPalette()
    for role, color in ((QPalette.ColorRole.Window, BACKGROUND), (QPalette.ColorRole.Base, BACKGROUND),
                        (QPalette.ColorRole.AlternateBase, CARD), (QPalette.ColorRole.Button, CONTROL),
                        (QPalette.ColorRole.WindowText, TEXT), (QPalette.ColorRole.Text, TEXT),
                        (QPalette.ColorRole.ButtonText, TEXT), (QPalette.ColorRole.ToolTipBase, CARD),
                        (QPalette.ColorRole.ToolTipText, TEXT), (QPalette.ColorRole.Highlight, PRIMARY_SOFT),
                        (QPalette.ColorRole.HighlightedText, TEXT), (QPalette.ColorRole.PlaceholderText, TEXT_HINT),
                        (QPalette.ColorRole.Link, PRIMARY)):
        result.setColor(role, QColor(color))
    result.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, QColor(TEXT_DISABLED))
    result.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, QColor(TEXT_DISABLED))
    result.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.ButtonText, QColor(TEXT_DISABLED))
    return result


def _chips() -> str:
    rules = []
    for name, (bg, border, text) in CHIPS.items():
        rules.append(f"#chip_{name} {{ background: {bg}; border: 1px solid {border}; color: {text}; "
                     f"border-radius: 9px; padding: 1px 8px; font-size: 12px; }}")
        rules.append(f"#card_{name} {{ background: {bg}; border: 1px solid {border}; border-radius: 6px; }}")
        rules.append(f"#card_{name} QLabel#cardTitle {{ color: {text}; font-weight: 600; }}")
    return "\n".join(rules)


STYLE = f"""
* {{ font-family: {FONT}; font-size: 13px; color: {TEXT}; }}
QMainWindow, QStackedWidget, #sectionBody, #page {{ background: {BACKGROUND}; }}
QToolTip {{ background: {CARD}; color: {TEXT}; border: 1px solid {BORDER_STRONG}; padding: 4px 6px; }}

/* header, side panels, status bar */
#header {{ background: {PANEL}; border-bottom: 1px solid {BORDER}; }}
#appTitle {{ font-size: 14px; font-weight: 600; }}
#projectName {{ color: {TEXT_HINT}; }}
#dirtyDot {{ color: {WARNING}; font-size: 10px; }}
#headerDivider {{ background: {BORDER}; }}
#sidebar, #inspector {{ background: {PANEL}; }}
#sidebar {{ border-right: 1px solid {BORDER}; }}
#inspector {{ border-left: 1px solid {BORDER}; }}
#sidebarTitle {{ font-weight: 600; padding: 2px 2px 4px 2px; }}
QStatusBar {{ background: {PANEL}; border-top: 1px solid {BORDER}; min-height: 28px; padding: 0 8px; }}
QStatusBar QLabel {{ margin: 0 4px; }}
QStatusBar::item {{ border: none; }}
QStatusBar QLabel {{ font-size: 12px; color: {TEXT_SECONDARY}; }}
QStatusBar QLabel#statusVersion {{ color: {TEXT_HINT}; }}
{_chips()}

/* navigation lists are painted by delegates; these rules give the frame */
#navigation {{ background: transparent; border: none; outline: none; }}
QSplitter::handle {{ background: {BORDER}; }}

/* section page */
#sectionKicker {{ font-size: 11px; color: {TEXT_HINT}; letter-spacing: 1px; }}
#sectionTitle {{ font-size: 20px; font-weight: 600; }}
#sectionIntro {{ color: {TEXT_SECONDARY}; padding-bottom: 4px; }}
#groupFrame {{ background: {PANEL}; border: 1px solid {BORDER}; border-radius: 8px; }}
#groupTitle {{ font-weight: 600; }}
#groupToggle {{ border: none; background: transparent; padding: 0; }}
#groupCount {{ font-size: 12px; color: {TEXT_HINT}; }}
#groupBody {{ background: transparent; }}
#fieldTitle {{ color: {TEXT}; }}
#fieldTitle:disabled {{ color: {TEXT_DISABLED}; }}
#fieldHelp {{ font-size: 12px; color: {TEXT_HINT}; }}
#fieldTitle[off="true"] {{ color: {TEXT_HINT}; }}
#fieldHelp[off="true"] {{ color: {TEXT_DISABLED}; }}
#fieldReason {{ font-size: 12px; color: {TEXT_HINT}; font-style: italic; }}
#fieldProblem {{ font-size: 12px; color: #F6C3C1; }}
#fieldRow {{ background: transparent; border-top: 1px solid {BORDER_SUBTLE}; }}
#fieldChip {{ background: transparent; }}
#fieldSubtitle {{ font-size: 12px; color: {TEXT_HINT}; }}
#fieldSubtitle[off="true"] {{ color: {TEXT_DISABLED}; }}
#fieldCost {{ font-family: {MONO}; font-size: 11px; color: {TEXT_HINT}; }}
#groupNote {{ font-size: 12px; color: {TEXT_HINT}; border-top: 1px solid {BORDER_SUBTLE}; }}
#chipToggle {{ background: {CONTROL}; border: 1px solid {BORDER_STRONG}; border-radius: 13px;
              padding: 2px 10px; color: {TEXT_SECONDARY}; }}
#chipToggle:checked {{ background: {PRIMARY_SOFT}; border-color: {PRIMARY_BORDER}; color: {PRIMARY}; }}
#chipToggle:disabled {{ color: {TEXT_DISABLED}; border-color: {BORDER}; }}
#footerInfo {{ font-size: 12px; color: {TEXT_HINT}; }}
#navSearch {{ margin: 2px 0 8px 0; }}
#needRequired {{ font-size: 11px; color: #F6C3C1; background: #2A1515; border: 1px solid #5C2626;
                border-radius: 9px; padding: 1px 7px; }}
#needOptional {{ font-size: 11px; color: {TEXT_SECONDARY}; background: {CONTROL};
                border: 1px solid {BORDER_STRONG}; border-radius: 9px; padding: 1px 7px; }}
#codeExample {{ font-family: {MONO}; font-size: 12px; color: {TEXT_LABEL}; background: {BACKGROUND};
               border: 1px solid {BORDER}; border-radius: 6px; padding: 6px 10px; }}
#notice {{ background: #2A2210; border: 1px solid #5A4719; border-radius: 6px; padding: 8px; color: #F5D9A0; }}
#hiddenHint {{ font-size: 12px; color: {TEXT_HINT}; }}

/* start page and guided cards */
#startKicker {{ font-size: 11px; color: {TEXT_HINT}; letter-spacing: 1px; }}
#startTitle {{ font-size: 26px; font-weight: 600; }}
#modeCard {{ background: {CARD}; border: 1px solid {BORDER}; border-radius: 8px; }}
#modeCard[recommended="true"] {{ border-color: {PRIMARY_BORDER}; }}
#modeTitle {{ font-size: 16px; font-weight: 600; }}
#listCard {{ background: {PANEL}; border: 1px solid {BORDER}; border-radius: 8px; }}
#listCardTitle {{ font-weight: 600; padding: 10px 14px; border-bottom: 1px solid {BORDER}; }}
#presetCard {{ background: {CARD}; border: 1px solid {BORDER}; border-radius: 8px; }}
#presetCard[checked="true"] {{ background: {PRIMARY_SOFT}; border-color: {PRIMARY_BORDER}; }}
#presetTitle {{ font-weight: 600; }}
#progress {{ font-size: 11px; color: {TEXT_HINT}; letter-spacing: 1px; }}
QProgressBar {{ background: {CONTROL}; border: none; border-radius: 2px; max-height: 4px; }}
QProgressBar::chunk {{ background: {PRIMARY}; border-radius: 2px; }}
#footerBar {{ border-top: 1px solid {BORDER}; }}

/* inspector */
#inspectorHeading {{ font-weight: 600; }}
#inspectorValue {{ font-family: {MONO}; }}
#inspectorSmall {{ font-size: 12px; color: {TEXT_SECONDARY}; }}
#inspectorNote {{ font-size: 11px; color: {TEXT_HINT}; }}
#inspectorRule {{ background: {BORDER}; max-height: 1px; min-height: 1px; }}
#cardText {{ font-size: 12px; color: {TEXT_LABEL}; }}
#fileState_new {{ font-family: {MONO}; font-size: 12px; color: {SUCCESS}; }}
#fileState_changed {{ font-family: {MONO}; font-size: 12px; color: {WARNING}; }}
#fileState_unchanged {{ font-family: {MONO}; font-size: 12px; color: {TEXT_HINT}; }}
#fileName {{ font-family: {MONO}; font-size: 12px; }}
#preview {{ font-family: {MONO}; font-size: 12px; background: {PANEL}; border: 1px solid {BORDER};
           border-radius: 8px; padding: 6px; }}

/* buttons */
QPushButton, QToolButton {{ background: {CONTROL}; border: 1px solid {BORDER_STRONG}; border-radius: 6px;
                           min-height: 28px; padding: 0 12px; }}
QPushButton:hover, QToolButton:hover {{ background: {CONTROL_HOVER}; }}
QPushButton:pressed, QToolButton:pressed {{ background: {BORDER}; }}
QPushButton:disabled, QToolButton:disabled {{ color: {TEXT_DISABLED}; background: {CONTROL}; }}
QToolButton::menu-indicator {{ image: none; width: 0; }}
#primaryButton {{ background: {PRIMARY}; color: {PRIMARY_TEXT}; border: none; font-weight: 600; }}
#primaryButton:hover {{ background: {PRIMARY_HOVER}; }}
#primaryButton:pressed {{ background: {PRIMARY_PRESSED}; }}
#primaryButton:disabled {{ background: {PRIMARY_BORDER}; color: {TEXT_SECONDARY}; }}
#ghostButton {{ background: transparent; color: {TEXT_LABEL}; }}
#linkButton {{ background: transparent; border: none; color: {PRIMARY}; padding: 0; min-height: 0;
              font-size: 12px; text-align: left; }}
#linkButton:hover {{ color: {PRIMARY_HOVER}; text-decoration: underline; }}
#segmentLeft, #segmentRight, #tabButton {{ background: {CONTROL}; color: {TEXT_SECONDARY}; border: 1px solid {BORDER_STRONG};
                                          padding: 0 12px; min-height: 26px; }}
#segmentLeft {{ border-top-right-radius: 0; border-bottom-right-radius: 0; }}
#segmentRight {{ border-left: none; border-top-left-radius: 0; border-bottom-left-radius: 0; }}
#segmentLeft:checked, #segmentRight:checked {{ background: {PRIMARY_SOFT}; color: {PRIMARY}; font-weight: 600; }}
#tabButton {{ border: none; border-radius: 6px; }}
#tabButton:checked {{ background: {BORDER}; color: {TEXT}; font-weight: 600; }}

/* inputs */
QLineEdit, QSpinBox, QComboBox, QPlainTextEdit {{ background: {BACKGROUND}; border: 1px solid {BORDER};
    border-radius: 6px; padding: 0 8px; min-height: 26px; selection-background-color: {PRIMARY_BORDER}; }}
QPlainTextEdit {{ padding: 4px; }}
QLineEdit:focus, QSpinBox:focus, QComboBox:focus {{ border-color: {PRIMARY}; }}
QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled {{ color: {TEXT_DISABLED}; background: {PANEL}; }}
QLineEdit[invalid="true"] {{ border-color: {DANGER}; }}
QComboBox {{ background: {CONTROL}; border-color: {BORDER_STRONG}; }}
QComboBox::drop-down {{ border: none; width: 20px; }}
QComboBox QAbstractItemView {{ background: {CARD}; border: 1px solid {BORDER_STRONG};
    selection-background-color: {PRIMARY_SOFT}; selection-color: {TEXT}; outline: none; }}
QSpinBox::up-button, QSpinBox::down-button {{ width: 16px; border: none; background: transparent; }}
QCheckBox {{ spacing: 8px; }}
QCheckBox::indicator {{ width: 14px; height: 14px; border-radius: 3px; border: 1px solid {BORDER_STRONG};
    background: {BACKGROUND}; }}
QCheckBox::indicator:checked {{ background: {PRIMARY}; border-color: {PRIMARY}; }}
QCheckBox::indicator:disabled {{ background: {PANEL}; border-color: {BORDER}; }}
QCheckBox::indicator:checked:disabled {{ background: {PRIMARY_BORDER}; border-color: {PRIMARY_BORDER}; }}

/* tables and lists */
QTableWidget, QListWidget {{ background: {PANEL}; border: 1px solid {BORDER}; border-radius: 6px;
    gridline-color: {BORDER_SUBTLE}; alternate-background-color: {CARD};
    selection-background-color: {PRIMARY_SOFT}; selection-color: {TEXT}; outline: none; }}
QHeaderView::section {{ background: {CONTROL}; color: {TEXT_SECONDARY}; border: none;
    border-right: 1px solid {BORDER}; border-bottom: 1px solid {BORDER}; padding: 4px 6px; }}
QTableCornerButton::section {{ background: {CONTROL}; border: none; }}

/* menus, scroll bars, dialogs */
QMenu {{ background: {CARD}; border: 1px solid {BORDER_STRONG}; padding: 4px; }}
QMenu::item {{ padding: 6px 18px; border-radius: 4px; }}
QMenu::item:selected {{ background: {PRIMARY_SOFT}; }}
QMenu::separator {{ height: 1px; background: {BORDER}; margin: 4px 6px; }}
QMenu::section {{ color: {TEXT_HINT}; font-size: 11px; padding: 6px 10px 2px 10px; }}
QScrollArea {{ background: transparent; border: none; }}
QScrollBar:vertical {{ background: transparent; width: 10px; margin: 0; }}
QScrollBar:horizontal {{ background: transparent; height: 10px; margin: 0; }}
QScrollBar::handle {{ background: {BORDER_STRONG}; border-radius: 4px; min-height: 24px; min-width: 24px; }}
QScrollBar::handle:hover {{ background: #4A5566; }}
QScrollBar::add-line, QScrollBar::sub-line, QScrollBar::add-page, QScrollBar::sub-page {{ background: none; border: none; }}
QMessageBox, QInputDialog, QFileDialog {{ background: {CARD}; }}
"""
