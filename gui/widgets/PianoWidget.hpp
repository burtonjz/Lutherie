/*
 * Copyright (C) 2026 Jared Burton
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

#ifndef PIANO_WIDGET_HPP_
#define PIANO_WIDGET_HPP_

#include <QWidget>

class PianoWidget : public QWidget {
    Q_OBJECT

private:
    bool isVertical_ = true ;

    int keyWidth_ ;
    int keyHeight_ ;

    int textPadW_ ;
    int textPadH_ ;

    int pianoWidth_ ;
    int pianoHeight_ ;
    
public:
    explicit PianoWidget(QWidget* parent = nullptr);

    bool isWhiteNote(uint8_t pitch) const ;
    QRect getNoteDimensions(uint8_t pitch) const ;

    bool isVertical() const ;
    void setVertical(bool vertical);

    QSize sizeHint() const override ;
    
protected:
    void paintEvent(QPaintEvent*) override ;
    

};

#endif // PIANO_WIDGET_HPP_