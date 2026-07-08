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

#include "widgets/NoteWidget.hpp"
#include "app/Theme.hpp"
#include <QEvent>
#include <QWidget>
#include <QPainter>

NoteWidget::NoteWidget(uint8_t midiNote, uint8_t velocity, float start, float end, QWidget* parent):
    QWidget(parent),
    midiNote_(midiNote),
    velocity_(velocity),
    startBeat_(start),
    endBeat_(end),
    selected_(false),
    noteName_(midi2str(midiNote)),
    label_(new QLabel(noteName_, this))
{ 
    label_->setMargin(4);
    label_->setStyleSheet(Theme::getLabelCommentStyle());
    label_->setMinimumWidth(label_->fontMetrics().horizontalAdvance("0000"));
    label_->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    setToolTip(noteName_ + QString(", velocity=%1").arg(velocity_));
    setMouseTracking(true);
    updateSize();    
}

NoteWidget::NoteWidget(SequenceNote note, QWidget* parent):
    NoteWidget::NoteWidget(note.pitch, note.velocity, note.startBeat, note.getEndBeat(), parent)
{}

void NoteWidget::paintEvent(QPaintEvent*){
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor fillColor = selected_ ? Theme::PIANO_ROLL_NOTE_SELECTED_COLOR :
        Theme::PIANO_ROLL_NOTE_COLOR ;

    painter.fillRect(0,0,w_, Theme::PIANO_KEY_THICKNESS, fillColor);
    painter.setPen(Theme::PIANO_ROLL_NOTE_BORDER);
    painter.drawRect(0,0,w_, Theme::PIANO_KEY_THICKNESS);
}

uint8_t NoteWidget::getMidiNote() const {
    return midiNote_ ;
}

void NoteWidget::setMidiNote(uint8_t midiNote){
    midiNote_ = std::min(std::max(midiNote, (uint8_t) 0), (uint8_t) 127 );
    noteName_ = midi2str(midiNote_);
    setToolTip(noteName_ + QString(", velocity=%1").arg(velocity_));
    label_->setText(noteName_);
    updateSize();
}

uint8_t NoteWidget::getVelocity() const {
    return velocity_ ;
}

void NoteWidget::setVelocity(uint8_t velocity){
    velocity_ = std::min(std::max(velocity, (uint8_t) 0), (uint8_t) 127 );
    setToolTip(noteName_ + QString(", velocity=%1").arg(velocity_));
    updateSize();
}

float NoteWidget::getStartBeat() const {
    return startBeat_ ;
}

void NoteWidget::setStartBeat(float startBeat, bool round){
    setBeatRange(startBeat, endBeat_, round);
}

float NoteWidget::getEndBeat() const {
    return endBeat_ ;
}

void NoteWidget::setEndBeat(float endBeat, bool round){
    setBeatRange(startBeat_, endBeat, round);
}

void NoteWidget::setBeatRange(float start, float end, bool round){
    if ( start > end ){
        std::swap(start, end);
    }

    if ( start < 0 ) start = 0 ;

    if ( round ){
        startBeat_ = std::round(start * 4) / 4.0f ;
        endBeat_ = std::round(end * 4) / 4.0f ;
    } else {
        startBeat_ = start ;
        endBeat_ = end ;
    }

    updateSize();
}

SequenceNote NoteWidget::getNote() const {
    return {midiNote_,velocity_,startBeat_,endBeat_ - startBeat_};
}

void NoteWidget::setSelected(bool selected){
    selected_ = selected ;
    update();
}

bool NoteWidget::isSelected() const {
    return selected_ ;
}

QString NoteWidget::midi2str(uint8_t midiNote){
    QString note ;
    switch( midiNote % 12 ){
        case 0 : note = "C"  ; break ;
        case 1 : note = "C#" ; break ;
        case 2 : note = "D"  ; break ;
        case 3 : note = "D#" ; break ;
        case 4 : note = "E"  ; break ;
        case 5 : note = "F"  ; break ;
        case 6 : note = "F#" ; break ;
        case 7 : note = "G"  ; break ;
        case 8 : note = "G#" ; break ;
        case 9 : note = "A"  ; break ;
        case 10: note = "A#" ; break ;
        case 11: note = "B"  ; break ;
    }

    return note + QString("%1").arg(midiNote / 12 - 1) ;
}

void NoteWidget::updateSize(){
    x_ = startBeat_ * Theme::PIANO_ROLL_PIXELS_PER_BEAT ;
    y_ = (127 - midiNote_) * Theme::PIANO_KEY_THICKNESS ;
    w_ = static_cast<float>(endBeat_ - startBeat_) * Theme::PIANO_ROLL_PIXELS_PER_BEAT ;
    setGeometry(x_,y_,w_, Theme::PIANO_KEY_THICKNESS);
    show();
    update();
}