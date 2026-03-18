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
#include <qcolor.h>
#include <qpalette.h>

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
const QColor Theme::BACKGROUND_DARK = QColor(22, 26, 30);  
const QColor Theme::BACKGROUND_MEDIUM = QColor(32, 38, 42);  
const QColor Theme::BACKGROUND_LIGHT = QColor(42, 48, 52);  
const QColor Theme::TEXT_PRIMARY = QColor(245, 237, 240);
const QColor Theme::TEXT_SECONDARY = QColor(160, 165, 168);  
const QColor Theme::ACCENT_COLOR = QColor(213, 137, 54);  
const QColor Theme::GRAPH_GRID_COLOR = QColor(38, 44, 48);

// Component colors
const QColor Theme::COMPONENT_BACKGROUND = QColor(42, 52, 58, 180); 
const QColor Theme::COMPONENT_BORDER = QColor(58, 68, 74);  
const QColor Theme::COMPONENT_BORDER_SELECTED = QColor(245, 237, 240);  
const QColor Theme::COMPONENT_BACKGROUND_HOVER = QColor(52, 62, 68, 200);
const QColor Theme::COMPONENT_TEXT = QColor(245, 237, 240);

// Cable colors - warmer and more saturated
const QColor Theme::CABLE_SHADOW = QColor(0, 0, 0, 60);
const QColor Theme::CABLE_AUDIO = QColor(135, 179, 141);  
const QColor Theme::CABLE_MODULATION = QColor(185, 49, 79); 
const QColor Theme::CABLE_MIDI = QColor(132, 169, 192); 

// Socket colors - match cables
const QColor Theme::SOCKET_AUDIO = QColor(135, 179, 141);
const QColor Theme::SOCKET_MODULATION = QColor(185, 49, 79);
const QColor Theme::SOCKET_MIDI = QColor(132, 169, 192);
const QColor Theme::SOCKET_AUDIO_LIGHT = Theme::SOCKET_AUDIO.lighter(140);
const QColor Theme::SOCKET_MODULATION_LIGHT = Theme::SOCKET_MODULATION.lighter(140);
const QColor Theme::SOCKET_MIDI_LIGHT = Theme::SOCKET_MIDI.lighter(140);

// Piano Roll
const QColor Theme::PIANO_ROLL_KEY_WHITE = QColor(220, 218, 215);  
const QColor Theme::PIANO_ROLL_KEY_BLACK = QColor(32, 38, 42);
const QColor Theme::PIANO_ROLL_KEY_BORDER = QColor(75, 82, 88);
const QColor Theme::PIANO_ROLL_KEY_LABEL = QColor(32, 38, 42);
const QColor Theme::PIANO_ROLL_NOTE_COLOR = QColor(135, 175, 155, 180);  
const QColor Theme::PIANO_ROLL_NOTE_SELECTED_COLOR = QColor(220, 155, 85, 200);  
const QColor Theme::PIANO_ROLL_NOTE_BORDER = QColor(115, 155, 135);
const QColor Theme::PIANO_ROLL_BACKGROUND = QColor(28, 32, 36);
const QColor Theme::PIANO_ROLL_GRID_PRIMARY = QColor(65, 72, 78);
const QColor Theme::PIANO_ROLL_GRID_SECONDARY = QColor(42, 48, 52);

// switch button
const QColor Theme::SWITCH_WIDGET_ON_COLOR = QColor(135, 175, 155);
const QColor Theme::SWITCH_WIDGET_OFF_COLOR = QColor(45, 52, 56);
const QColor Theme::SWITCH_WIDGET_THUMB_COLOR_ON = QColor(245, 237, 240);
const QColor Theme::SWITCH_WIDGET_THUMB_COLOR_OFF = QColor(130, 135, 138);
const QColor Theme::SWITCH_WIDGET_DISABLED_COLOR = QColor(38, 44, 48);

// spectrum analyzer
const QColor Theme::ANALYZER_BACKGROUND_COLOR = QColor(22, 26, 30);
const QColor Theme::ANALYZER_GRID_COLOR = QColor(38, 44, 48);
const QColor Theme::ANALYZER_LINE_COLOR = QColor(135, 175, 155);


const QColor Theme::MODULATION_ACTIVE = QColor(185, 49, 79);  
const QColor Theme::MODULATION_INACTIVE = QColor(15, 15, 15);

const QColor Theme::TOAST_NOTIFICATION_BG = QColor(58, 68, 74);
const QColor Theme::TOAST_NOTIFICATION_TEXT = QColor(245, 237, 240);

void Theme::applyDarkTheme() {

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, BACKGROUND_MEDIUM);
    darkPalette.setColor(QPalette::WindowText, TEXT_PRIMARY);
    darkPalette.setColor(QPalette::Base, BACKGROUND_DARK);
    darkPalette.setColor(QPalette::AlternateBase, BACKGROUND_MEDIUM);
    darkPalette.setColor(QPalette::Text, TEXT_PRIMARY);
    darkPalette.setColor(QPalette::Button, BACKGROUND_MEDIUM);
    darkPalette.setColor(QPalette::ButtonText, TEXT_PRIMARY);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, ACCENT_COLOR);
    darkPalette.setColor(QPalette::Highlight, ACCENT_COLOR);
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    qApp->setStyle("Fusion");
    
    qApp->setPalette(darkPalette);
    
    // Global stylesheet
    QString styleSheet = QString(R"(
    QWidget {
        font-size: 11pt;
    }
    
    QLineEdit {
        background-color: %2 ;
        color: %5;
        border: 1px solid ;
        padding: 2px ;
    }
    
    QMessageBox {
        background-color: %2;
        color: %5;
    }
    QMessageBox QLabel {
        color: %5;
    }
    QMessageBox QPushButton {
        background-color: %2;
        color: %5;
        border: 1px solid %1;
        padding: 5px 15px;
        border-radius: 3px;
    }
    QMessageBox QPushButton:hover {
        background-color: %3;
    }
    QFileDialog {
        background-color: %2;
        color: %5;
    }
    QFileDialog QLabel {
        color: %5;
    }
    QFileDialog QPushButton {
        background-color: %2;
        color: %5;
        border: 1px solid %1;
        padding: 5px 15px;
        border-radius: 3px;
    }
    QFileDialog QPushButton:hover {
        background-color: %3;
    }
    QFileDialog QLineEdit {
        background-color: %4;
        color: %5;
        border: 1px solid %1;
        padding: 3px;
        border-radius: 3px;
    }
    QFileDialog QListView, QFileDialog QTreeView {
        background-color: %4;
        color: %5;
        border: 1px solid %1;
    }
    QFileDialog QListView::item:hover, QFileDialog QTreeView::item:hover {
        background-color: %3;
    }
    QFileDialog QListView::item:selected, QFileDialog QTreeView::item:selected {
        background-color: %6;
        color: #000;
    }
    QToolTip {
        background-color: %2;
        color: %5;
        border: 1px solid %1;
        padding: 5px;
        border-radius: 3px;
    }
    QToolBar {
        background-color: %2;
        spacing: 4px;
        border: none;
    }
    QToolButton {
        color: %5 ;
        background-color: %2;
        border: 1px solid %1;
        padding: 4px 8px ;
    }
    QToolButton:hover {
        background-color: %6;
    }
    QAbstractItemView {
        background-color: %4;
        color: %5;
        border: 1px solid %1;
        selection-background-color: %3;
        selection-color: %5;
    }
    QMenuBar {
        background-color: %2;
        color: %5;
    }
    QMenuBar::item {
        background: transparent;
    }
    QMenuBar::item:selected {
        background: %3 ;
        color: %5 ;
    }
    QMenu {
        background-color: %2;
        color: %5;
        border: 1px solid %1;
        padding: 5px;
        border-radius: 3px;
    }
    QMenu::item {
        color: %5;
        padding: 4px 20px;
        border-radius: 3px;
    }
    QMenu::item:selected {
        background-color: %3;
        color: %5;                
    }
    QGroupBox {
        font-weight: bold;
        border: 1px solid %1;
        border-radius: 5px;
        margin-top: 10px;
        padding-top: 10px;
        background-color: %2;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 10px;
        padding: 0 5px;
    }
    QPushButton {
        padding: 5px 15px;
        border-radius: 3px;
        background-color: %2;
        border: 1px solid %1;
        color: %5;
    }
    QPushButton:hover {
        background-color: %3;
    }
    QDoubleSpinBox, QSpinBox, QComboBox {
        padding: 3px;
        background-color: %4;
        border: 1px solid %1;
        border-radius: 3px;
        color: %5;
    }
    QComboBox QAbstractItemView {
        background-color: %4;
        border: 1px solid %1;
        outline: none;
        selection-background-color: %3;
        selection-color: %5;
    }
    QComboBox QAbstractItemView::item {
        border-bottom: 1px solid %7;
        padding: 4px;
    }
    QComboBox QAbstractItemView::item:hover {
        background-color: %3;
        color: %5;
    }
    QComboBox QAbstractItemView::item:selected {
        background-color: %6;        
        color: #000;                   
    }
    QLabel {
        color: %5;
    }
)").arg(COMPONENT_BORDER.name())               // %1
   .arg(BACKGROUND_MEDIUM.name())              // %2
   .arg(COMPONENT_BACKGROUND_HOVER.name())     // %3
   .arg(BACKGROUND_DARK.name())                // %4
   .arg(TEXT_PRIMARY.name())                   // %5
   .arg(ACCENT_COLOR.name())                   // %6
   .arg(BACKGROUND_LIGHT.name());              // %7
    
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