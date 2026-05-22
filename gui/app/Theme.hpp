/*
 * Copyright (C) 2025 Jared Burton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY ; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

 #ifndef __UI_THEME_HPP_
 #define __UI_THEME_HPP_

#include <QColor>
#include <QString>
#include <QApplication>
#include <QPalette>

class Theme {
public:
    /*
    ================================================
    ==================THEME COLORS==================
    ================================================ 
    */
    // UI Colors
    static const QColor BACKGROUND_DARK ;
    static const QColor BACKGROUND_MEDIUM ;
    static const QColor BACKGROUND_LIGHT ;
    static const QColor GRAPH_GRID_COLOR ;
    static const QColor TEXT_PRIMARY ;
    static const QColor TEXT_SECONDARY ;
    static const QColor ACCENT_COLOR ;

    // Cable/Connection Colors
    static const QColor CABLE_SHADOW ;
    static const QColor CABLE_SIGNAL ;
    static const QColor CABLE_BUFFER ;
    static const QColor CABLE_MODULATION ;
    static const QColor CABLE_MIDI ;
    
    // Socket Colors
    static const QColor SOCKET_SIGNAL ;
    static const QColor SOCKET_SIGNAL_LIGHT ;
    static const QColor SOCKET_BUFFER ;
    static const QColor SOCKET_BUFFER_LIGHT ;
    static const QColor SOCKET_MIDI ;
    static const QColor SOCKET_MIDI_LIGHT ;
    static const QColor SOCKET_MODULATION ;
    static const QColor SOCKET_MODULATION_LIGHT ;

    // Component Colors
    static const QColor COMPONENT_BORDER ;
    static const QColor COMPONENT_BORDER_SELECTED ;
    static const QColor COMPONENT_BACKGROUND ;
    static const QColor COMPONENT_BACKGROUND_HOVER ;
    static const QColor COMPONENT_TEXT ;
    
    static const QColor PIANO_ROLL_KEY_WHITE ;
    static const QColor PIANO_ROLL_KEY_BLACK ;
    static const QColor PIANO_ROLL_KEY_BORDER ;
    static const QColor PIANO_ROLL_KEY_LABEL ;
    static const QColor PIANO_ROLL_NOTE_COLOR ;
    static const QColor PIANO_ROLL_NOTE_SELECTED_COLOR ;
    static const QColor PIANO_ROLL_NOTE_BORDER ;
    static const QColor PIANO_ROLL_BACKGROUND ;
    static const QColor PIANO_ROLL_GRID_PRIMARY ;
    static const QColor PIANO_ROLL_GRID_SECONDARY ;

    static const QColor SWITCH_WIDGET_ON_COLOR ;
    static const QColor SWITCH_WIDGET_OFF_COLOR ;
    static const QColor SWITCH_WIDGET_THUMB_COLOR_ON ;
    static const QColor SWITCH_WIDGET_THUMB_COLOR_OFF ;
    static const QColor SWITCH_WIDGET_DISABLED_COLOR ;

    static const QColor SPECTRUM_BACKGROUND_COLOR ;
    static const std::array<QColor,6> SPECTRUM_LINE_COLORS ;
    static const QColor SPECTRUM_GRID_COLOR ;

    static const QColor OSCILLOSCOPE_BACKGROUND_COLOR ;
    static const std::array<QColor,6> OSCILLOSCOPE_LINE_COLORS ;
    static const QColor OSCILLOSCOPE_GRID_COLOR ;

    static const QColor MODULATION_ACTIVE ;
    static const QColor MODULATION_INACTIVE ;

    static const QColor TOAST_NOTIFICATION_BG ;
    static const QColor TOAST_NOTIFICATION_TEXT ;

    static const QColor KNOB_WIDGET_TRACK_COLOR ;
    static const QColor KNOB_WIDGET_ARC_COLOR ;
    static const QColor KNOB_WIDGET_HANDLE_COLOR ;
    static const QColor KNOB_WIDGET_NOTCH_COLOR ;
    static const QColor KNOB_WIDGET_BODY_COLOR ;
    static const QColor KNOB_WIDGET_BODY_BORDER_COLOR ;
    static const QColor KNOB_WIDGET_HIGHLIGHT_COLOR;  
    
    // Apply theme to application
    static void applyDarkTheme();
    
    // Get stylesheet snippets
    static QString getComponentStyle(bool selected = false, bool hovered = false);

    /*
    =======================================================
    =============== THEME SPACING / CONTROLS ==============
    =======================================================
    */

    static constexpr const char* DEFAULT_WINDOW_TITLE = "Lutherie" ;

    static constexpr int TOOLBAR_HEIGHT = 32 ;

    static constexpr int   GRAPH_DOUBLE_CLICK_MS    = 300 ;
    static constexpr qreal GRAPH_GRID_SIZE          = 20.0 ;
    static constexpr qreal GRAPH_WHEEL_SCALE_FACTOR = 1.15 ;
    

    static constexpr qreal COMPONENT_WIDTH            = 120.0 ;
    static constexpr qreal COMPONENT_HEIGHT           = 80.0 ;
    static constexpr qreal COMPONENT_ROUNDED_RADIUS   = 5.0 ;
    static constexpr qreal COMPONENT_BORDER_WIDTH     = 2.0 ;
    static constexpr qreal COMPONENT_TEXT_PADDING     = 5.0 ;
    static constexpr qreal COMPONENT_HIGHLIGHT_BUFFER = 2.0 ; 
    static constexpr qreal COMPONENT_HIGHLIGHT_WIDTH  = 3.0 ;
    static constexpr qreal SOCKET_WIDGET_SPACING      = 15.0 ;
    static constexpr qreal SOCKET_WIDGET_RADIUS       = 6.0 ;
    static constexpr qreal SOCKET_WIDGET_MARGIN       = 5.0 ;

    static constexpr qreal CABLE_SIDE_BEND_FACTOR   = 0.5 ; 
    static constexpr qreal CABLE_SIDE_BEND_MAX      = 70.0 ; 
    static constexpr qreal CABLE_STEM_LENGTH_MAX    = 80.0 ;
    static constexpr qreal CABLE_STEM_LENGTH_FACTOR = 0.6 ;
    static constexpr qreal CABLE_CYCLE_THRESHOLD    = 0.0 ;
    static constexpr qreal CABLE_ARROW_HEIGHT       = 12.0 ;
    static constexpr qreal CABLE_ARROW_BASE_WIDTH   = 6.0 ; 

    static constexpr qreal PARAMETER_WIDGET_SPACING = 10 ;
    static constexpr qreal PARAMETER_WIDGET_WIDTH   = 120 ;
    static constexpr qreal PARAMETER_WIDGET_MARGINS = 20 ;
    static constexpr int   PARAMETER_GRID_N_COLS    = 2 ;

    static constexpr qreal COMPONENT_DETAIL_COLLECTION_ROW_SPACING     = 2 ;
    static constexpr qreal COMPONENT_DETAIL_COLLECTION_WIDGET_SPACING  = 8 ;
    static constexpr qreal COMPONENT_DETAIL_COLLECTION_DELETE_BTN_SIZE = 24 ;

    static constexpr qreal PIANO_ROLL_NOTE_HEIGHT         = 30 ; 
    static constexpr qreal PIANO_ROLL_NOTE_EDGE_THRESHOLD = 8 ;

    static constexpr qreal PIANO_ROLL_PIXELS_PER_BEAT          = 100 ;
    static constexpr qreal PIANO_ROLL_KEY_WIDTH                = 60 ;
    static constexpr qreal PIANO_ROLL_GRID_PEN_WIDTH_PRIMARY   = 1 ;
    static constexpr qreal PIANO_ROLL_GRID_PEN_WIDTH_SECONDARY = .75 ;
    static constexpr qreal PIANO_ROLL_KEY_LABEL_X_PAD          = 4 ;

    static constexpr int   SPECTRUM_MARGIN_LEFT      = 60 ;
    static constexpr int   SPECTRUM_MARGIN_RIGHT     = 20 ;
    static constexpr int   SPECTRUM_MARGIN_TOP       = 20 ;
    static constexpr int   SPECTRUM_MARGIN_BOTTOM    = 120 ;
    static constexpr int   SPECTRUM_PIXEL_RESOLUTION = 2 ;
    static constexpr float SPECTRUM_MIN_FREQUENCY    = 10.0 ;
    static constexpr float SPECTRUM_MAX_FREQUENCY    = 25000.0 ;
    static constexpr float SPECTRUM_MIN_DECIBEL      = -100.0 ;
    static constexpr float SPECTRUM_MAX_DECIBEL      = 5.0 ;

    static constexpr int   OSCILLOSCOPE_MARGIN_LEFT      = 60 ;
    static constexpr int   OSCILLOSCOPE_MARGIN_RIGHT     = 20 ;
    static constexpr int   OSCILLOSCOPE_MARGIN_TOP       = 20 ;
    static constexpr int   OSCILLOSCOPE_MARGIN_BOTTOM    = 120 ;
    static constexpr float OSCILLOSCOPE_MIN_AMPLITUDE    = -.75 ;
    static constexpr float OSCILLOSCOPE_MAX_AMPLITUDE    = .75 ;
    static constexpr std::initializer_list<float> OSCILLOSCOPE_AMPLITUDE_LABELS = {
        -0.75f, -0.5f, -0.25f, 0.0f, 0.25f, 0.5f, 0.75f 
    };

    static constexpr qreal SWITCH_WIDGET_OPACITY_ON       = 0.8 ;
    static constexpr qreal SWITCH_WIDGET_OPACITY_OFF      = 0.38 ;
    static constexpr qreal SWITCH_WIDGET_OPACITY_DISABLED = 0.12 ;
    static constexpr int SWITCH_WIDGET_HEIGHT             = 16 ;
    static constexpr int SWITCH_WIDGET_MARGIN             = 3 ;
    static constexpr int SWITCH_WIDGET_CORNER_ROUND       = 8 ;
    static constexpr int SWITCH_WIDGET_ANIMATION_DURATION = 120 ; // msec
    
    static constexpr int MODULATION_INDICATOR_SIZE         = 16 ; 
    static constexpr int MODULATION_INDICATOR_GLOW_RADIUS  = 6 ;
    static constexpr qreal MODULATION_INDICATOR_RING_WIDTH = 2.0 ;

    static constexpr int TOAST_NOTIFICATION_DURATION      = 4000 ; // msec
    static constexpr int TOAST_NOTIFICATION_FADE_DURATION = 800 ; // msec
    static constexpr int TOAST_NOTIFICATION_CORNER_RADIUS = 6 ;
    static constexpr int TOAST_NOTIFICATION_PADDING_H     = 16 ;
    static constexpr int TOAST_NOTIFICATION_PADDING_V     = 8 ;
    static constexpr int TOAST_NOTIFICATION_FONT_SIZE     = 13 ;
    static constexpr int TOAST_NOTIFICATION_MARGIN        = 24 ;
    static constexpr int TOAST_NOTIFICATION_BG_MAX_ALPHA  = 220 ;

    static constexpr int COLLAPSIBLE_EDITOR_MARGIN = 4 ;
    static constexpr int COLLAPSIBLE_EDITOR_INDENT = 8 ;

    static constexpr int TOOL_BUTTON_SIZE = 16 ;

    static constexpr int    KNOB_WIDGET_SIZE             = 42 ;
    static constexpr int    KNOB_WIDGET_ARC_WIDTH        = 4 ;     
    static constexpr int    KNOB_WIDGET_DRAG_SENSITIVITY = 200 ;
    static constexpr double KNOB_WIDGET_START_ANGLE      = 225.0 ;
    static constexpr double KNOB_WIDGET_SWEEP_ANGLE      = 270.0 ; 

    /*
    =======================================================
    =============== STYLE SHEET DEFINITIONS ===============
    =======================================================
    */
    static const QString& getLabelTitleStyle();
    static const QString& getLabelHeaderStyle();

};

#endif // __UI_THEME_HPP_

