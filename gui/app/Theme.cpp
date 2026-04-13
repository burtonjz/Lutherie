/*
 * Copyright (C) 2025 Jared Burton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "app/Theme.hpp"
#include <QColor>

/*
==================== PALETTE ===================
Just trying my best using https://coolors.co

Background: 22, 26, 30 // carbon black
Border: 58, 68, 74 // charcoal blue
Grid: 38, 44, 48 // jet black
Text: 245, 237, 240 // lavender blush
Accent: 242, 129, 35 // vivid tangerine

Audio: 135, 179, 141 // muted teal
Modulation: 185, 49, 79 // rosewood
Midi: 132, 169, 192 // steel blue
*/

// Main Panel Colors
const QColor Theme::BACKGROUND_DARK   = QColor(22, 26, 30);  
const QColor Theme::BACKGROUND_MEDIUM = QColor(32, 38, 42);  
const QColor Theme::BACKGROUND_LIGHT  = QColor(42, 48, 52);  
const QColor Theme::TEXT_PRIMARY      = QColor(245, 237, 240);
const QColor Theme::TEXT_SECONDARY    = QColor(160, 165, 168);  
const QColor Theme::ACCENT_COLOR      = QColor(213, 137, 54);  
const QColor Theme::GRAPH_GRID_COLOR  = QColor(38, 44, 48);

// Component colors
const QColor Theme::COMPONENT_BACKGROUND       = QColor(42, 52, 58, 180); 
const QColor Theme::COMPONENT_BORDER           = QColor(58, 68, 74);  
const QColor Theme::COMPONENT_BORDER_SELECTED  = QColor(245, 237, 240);  
const QColor Theme::COMPONENT_BACKGROUND_HOVER = QColor(52, 62, 68, 200);
const QColor Theme::COMPONENT_TEXT             = QColor(245, 237, 240);

// Cable colors - warmer and more saturated
const QColor Theme::CABLE_SHADOW     = QColor(0, 0, 0, 60);
const QColor Theme::CABLE_AUDIO      = QColor(135, 179, 141);  
const QColor Theme::CABLE_MODULATION = QColor(185, 49, 79); 
const QColor Theme::CABLE_MIDI       = QColor(132, 169, 192); 

// Socket colors - match cables
const QColor Theme::SOCKET_AUDIO            = QColor(135, 179, 141);
const QColor Theme::SOCKET_MODULATION       = QColor(185, 49, 79);
const QColor Theme::SOCKET_MIDI             = QColor(132, 169, 192);
const QColor Theme::SOCKET_AUDIO_LIGHT      = Theme::SOCKET_AUDIO.lighter(140);
const QColor Theme::SOCKET_MODULATION_LIGHT = Theme::SOCKET_MODULATION.lighter(140);
const QColor Theme::SOCKET_MIDI_LIGHT       = Theme::SOCKET_MIDI.lighter(140);

// Piano Roll
const QColor Theme::PIANO_ROLL_KEY_WHITE           = QColor(220, 218, 215);  
const QColor Theme::PIANO_ROLL_KEY_BLACK           = QColor(32, 38, 42);
const QColor Theme::PIANO_ROLL_KEY_BORDER          = QColor(75, 82, 88);
const QColor Theme::PIANO_ROLL_KEY_LABEL           = QColor(32, 38, 42);
const QColor Theme::PIANO_ROLL_NOTE_COLOR          = QColor(135, 175, 155, 180);  
const QColor Theme::PIANO_ROLL_NOTE_SELECTED_COLOR = QColor(220, 155, 85, 200);  
const QColor Theme::PIANO_ROLL_NOTE_BORDER         = QColor(115, 155, 135);
const QColor Theme::PIANO_ROLL_BACKGROUND          = QColor(28, 32, 36);
const QColor Theme::PIANO_ROLL_GRID_PRIMARY        = QColor(65, 72, 78);
const QColor Theme::PIANO_ROLL_GRID_SECONDARY      = QColor(42, 48, 52);

// switch button
const QColor Theme::SWITCH_WIDGET_ON_COLOR        = QColor(135, 175, 155);
const QColor Theme::SWITCH_WIDGET_OFF_COLOR       = QColor(45, 52, 56);
const QColor Theme::SWITCH_WIDGET_THUMB_COLOR_ON  = QColor(245, 237, 240);
const QColor Theme::SWITCH_WIDGET_THUMB_COLOR_OFF = QColor(130, 135, 138);
const QColor Theme::SWITCH_WIDGET_DISABLED_COLOR  = QColor(38, 44, 48);

// spectrum analyzer
const QColor Theme::ANALYZER_BACKGROUND_COLOR = QColor(22, 26, 30);
const QColor Theme::ANALYZER_GRID_COLOR       = QColor(38, 44, 48);
const QColor Theme::ANALYZER_LINE_COLOR       = QColor(135, 175, 155);

const QColor Theme::MODULATION_ACTIVE   = QColor(185, 49, 79);  
const QColor Theme::MODULATION_INACTIVE = QColor(15, 15, 15);

const QColor Theme::TOAST_NOTIFICATION_BG   = QColor(58, 68, 74);
const QColor Theme::TOAST_NOTIFICATION_TEXT = QColor(245, 237, 240);

const QColor Theme::KNOB_WIDGET_TRACK_COLOR       = Theme::COMPONENT_BORDER ;
const QColor Theme::KNOB_WIDGET_ARC_COLOR         = Theme::ACCENT_COLOR ;
const QColor Theme::KNOB_WIDGET_HANDLE_COLOR      = Theme::ACCENT_COLOR.lighter(130);
const QColor Theme::KNOB_WIDGET_NOTCH_COLOR       = Theme::ACCENT_COLOR ;
const QColor Theme::KNOB_WIDGET_BODY_COLOR        = Theme::BACKGROUND_LIGHT ;
const QColor Theme::KNOB_WIDGET_BODY_BORDER_COLOR = Theme::COMPONENT_BORDER ;
const QColor Theme::KNOB_WIDGET_HIGHLIGHT_COLOR   = QColor(235, 162, 85) ;

void Theme::applyDarkTheme() {

    QPalette darkPalette ;
    // Window / panels
    darkPalette.setColor(QPalette::Window,        Theme::BACKGROUND_MEDIUM);
    darkPalette.setColor(QPalette::WindowText,    Theme::TEXT_PRIMARY);

    // Input fields, list views, combo dropdowns
    darkPalette.setColor(QPalette::Base,          Theme::BACKGROUND_DARK);
    darkPalette.setColor(QPalette::AlternateBase, Theme::BACKGROUND_LIGHT);
    darkPalette.setColor(QPalette::Text,          Theme::TEXT_PRIMARY);

    // Buttons
    darkPalette.setColor(QPalette::Button,        Theme::BACKGROUND_LIGHT);
    darkPalette.setColor(QPalette::ButtonText,    Theme::TEXT_PRIMARY);

    // Highlight — this drives selection everywhere: combos, lists, tables
    darkPalette.setColor(QPalette::Highlight,        Theme::ACCENT_COLOR);
    darkPalette.setColor(QPalette::HighlightedText,  Theme::BACKGROUND_DARK);

    // Tooltips
    darkPalette.setColor(QPalette::ToolTipBase, Theme::BACKGROUND_MEDIUM);
    darkPalette.setColor(QPalette::ToolTipText, Theme::TEXT_PRIMARY);

    // Placeholder text in line edits
    darkPalette.setColor(QPalette::PlaceholderText, Theme::TEXT_SECONDARY);

    // Disabled states
    darkPalette.setColor(QPalette::Disabled, QPalette::Text,       Theme::TEXT_SECONDARY);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Theme::TEXT_SECONDARY);
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, Theme::TEXT_SECONDARY);
    darkPalette.setColor(QPalette::Disabled, QPalette::Base,       Theme::BACKGROUND_DARK);
    darkPalette.setColor(QPalette::Disabled, QPalette::Button,     Theme::BACKGROUND_MEDIUM);
    darkPalette.setColor(QPalette::Active,   QPalette::ButtonText, Theme::TEXT_PRIMARY);
    darkPalette.setColor(QPalette::Inactive, QPalette::ButtonText, Theme::TEXT_PRIMARY);
    darkPalette.setColor(QPalette::Active,   QPalette::WindowText, Theme::TEXT_PRIMARY);
    darkPalette.setColor(QPalette::Inactive, QPalette::WindowText, Theme::TEXT_PRIMARY);

    // Borders / mid tones (used by Fusion for dividers, frames, spin box arrows)
    darkPalette.setColor(QPalette::Mid,    Theme::COMPONENT_BORDER);
    darkPalette.setColor(QPalette::Dark,   Theme::BACKGROUND_DARK);
    darkPalette.setColor(QPalette::Shadow, Theme::BACKGROUND_DARK);
    darkPalette.setColor(QPalette::Light,  Theme::BACKGROUND_LIGHT);

    qApp->setStyle("Fusion");
    qApp->setPalette(darkPalette);    
    
    // stylesheet for layout/sizing definitions
    // also a couple things that apparently don't respect the pallette...
    QString styleSheet = QString(R"(
        QWidget {
            font-size: 11pt;
        }
        QPushButton {
            padding: 5px 15px;
            border-radius: 3px;
        }
        QLineEdit, QDoubleSpinBox, QSpinBox, QComboBox {
            padding: 3px;
            border-radius: 3px;
        }
        QComboBox QAbstractItemView::item {
            min-height: 24px;
            padding: 0px 8px;
        }
        QMenu {
            padding: 5px;
            border-radius: 3px;
        }
        QMenu::item {
            padding: 4px 20px;
            border-radius: 3px;
        }
        QGroupBox {
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QToolTip {
            background-color: %3;
            color: %4;
            border: 1px solid %5;
            padding: 5px;
            border-radius: 3px;
            opacity: 255;
        }
        QMenu::item:selected {
            background-color: %1;
            color: %2;           
        }
        QMenuBar::item:selected {
            background-color: %1;
            color: %2;
        }
        QToolBar QToolButton {
            color: %4;   
            background-color: transparent;
            border: none;
        }
        QToolBar QToolButton[popupMode="1"],
        QToolBar QToolButton[popupMode="2"] {
            padding-right: 10px;
        }
        QToolBar QToolButton:hover {
            background-color: %1;
            color: %2;
        }
        QToolBar QToolButton:pressed, 
        QToolBar QToolButton:open {
            background-color: %1;
            color: %2;
        }
        QToolButton#unitToggle,
        QToolButton#unitToggle:checked {
            color: %1;
            border: 1px solid %5;
            border-radius: 3px;
            padding: 1px 4px;
            font-size: 9pt;
            background-color: transparent;
        }
        QToolButton#unitToggle:hover,
        QToolButton#unitToggle:checked:hover {
            color: %4;
            border: 1px solid %5;
            border-radius: 3px;
            padding: 1px 4px;
            font-size: 9pt;
            background-color: %1
        }
    )").arg(ACCENT_COLOR.name())
    .arg(BACKGROUND_DARK.name())
    .arg(BACKGROUND_MEDIUM.name())
    .arg(TEXT_PRIMARY.name())
    .arg(COMPONENT_BORDER.name());
    
    qApp->setStyleSheet(styleSheet);
}

const QString& Theme::getLabelTitleStyle(){
    static const QString title = QString(
        "font-size: 18px; font-weight: bold; color: %1;"
    ).arg(TEXT_PRIMARY.name()) ;
    
    return title ;
}

const QString& Theme::getLabelHeaderStyle(){
    static const QString title = QString(
        "font-size: 14px; font-weight: bold; color: %1;"
    ).arg(TEXT_PRIMARY.name()) ;
    
    return title ;
}