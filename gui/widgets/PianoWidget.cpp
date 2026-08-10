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

#include "widgets/PianoWidget.hpp"
#include "app/Theme.hpp"

#include <QPainter>

PianoWidget::PianoWidget(QWidget* parent):
    QWidget(parent),
    keyWidth_(Theme::PIANO_KEY_LENGTH),
    keyHeight_(Theme::PIANO_KEY_THICKNESS),
    textPadW_(Theme::PIANO_KEY_LABEL_PAD),
    textPadH_(0),
    pianoWidth_(keyWidth_),
    pianoHeight_(keyHeight_ * 128) 
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    update();
}

bool PianoWidget::isWhiteNote(uint8_t pitch) const {
    uint8_t note = pitch % 12 ;
    return note == 0 || note == 2 || 
           note == 4 || note == 5 || 
           note == 7 || note == 9 || 
           note == 11 ;
}

QRect PianoWidget::getNoteDimensions(uint8_t pitch) const {
    if ( isVertical_ ){
        int pos = static_cast<int>((127 - pitch) * Theme::PIANO_KEY_THICKNESS);
        return {
            0, pos,
            keyWidth_, keyHeight_ 
        };
    } else {
        int pos = static_cast<int>((pitch) * Theme::PIANO_KEY_THICKNESS);
        return {
            pos, 0,
            keyWidth_, keyHeight_
        };
    }
}

uint8_t PianoWidget::pitchAt(const QPoint& pos) const {
    int pitchPos = isVertical() ? pos.y() : pos.x();
    int step = static_cast<int>(Theme::PIANO_KEY_THICKNESS);
    if ( step <= 0 ) return 128 ;

    int pitch = isVertical_ 
        ? 127 - pitchPos / step
        : pitchPos / step ;
    
    if ( pitch < 0 || pitch > 127 ) return 128 ;
    return pitch ;
}

void PianoWidget::paintEvent(QPaintEvent*){
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    for ( uint8_t note = 0; note < 128; ++note ){
        QRect keyRect = getNoteDimensions(note);
        
        QColor keyColor = isWhiteNote(note) 
            ? Theme::PIANO_ROLL_KEY_WHITE 
            : Theme::PIANO_ROLL_KEY_BLACK ;

        p.fillRect(keyRect, keyColor);
        p.setPen(Theme::PIANO_ROLL_KEY_BORDER);
        p.drawRect(keyRect);

        // draw some note names
        if ( note % 12 == 0 ){
            int xPad = isVertical_ ? Theme::PIANO_KEY_LABEL_PAD / 2 : 0 ;
            int yPad = isVertical_ ? 0 : Theme::PIANO_KEY_LABEL_PAD / 2 ;

            QRect labelRect(
                keyRect.x() + xPad,
                keyRect.y() + yPad,
                keyRect.width() - textPadW_,
                keyRect.height() - textPadH_
            );

            p.setPen(Theme::PIANO_ROLL_KEY_LABEL);
            p.drawText(
                labelRect, Qt::AlignCenter,
                QString("C%1").arg(note / 12 - 1)
            );
        }
    }   
}

QSize PianoWidget::sizeHint() const {
    return { pianoWidth_, pianoHeight_ };
}

bool PianoWidget::isVertical() const {
    return isVertical_ ;
}

void PianoWidget::setVertical(bool vertical){
    isVertical_ = vertical ;
    if ( isVertical_ ){
        keyWidth_ = Theme::PIANO_KEY_LENGTH ;
        keyHeight_ = Theme::PIANO_KEY_THICKNESS ;
        textPadW_ = Theme::PIANO_KEY_LABEL_PAD ;
        textPadH_ = 0 ;
        pianoWidth_ = keyWidth_ ;
        pianoHeight_ = keyHeight_ * 128 ;
    } else {
        keyWidth_ = Theme::PIANO_KEY_THICKNESS ;
        keyHeight_ = Theme::PIANO_KEY_LENGTH ;
        textPadW_ = 0 ;
        textPadH_ = Theme::PIANO_KEY_LABEL_PAD ;
        pianoWidth_ = keyWidth_ * 128 ;
        pianoHeight_ = keyHeight_ ;
    }

    update();
}
