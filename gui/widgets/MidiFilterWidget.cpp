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

#include "widgets/MidiFilterWidget.hpp"
#include "app/Theme.hpp"

#include <QStackedLayout>
#include <QMouseEvent>
#include <QPainter>
#include <spdlog/spdlog.h>

FilterOverlay::FilterOverlay(PianoWidget* piano, QWidget* parent):
    QWidget(parent),
    piano_(piano)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

const std::set<uint8_t>& FilterOverlay::activeNotes() const {
    return activeNotes_ ;
}

void FilterOverlay::setActiveNotes(std::set<uint8_t> notes){
    activeNotes_ = std::move(notes);
    update();
}

void FilterOverlay::clearActiveNotes(){
    activeNotes_.clear();
}

void FilterOverlay::insertNote(uint8_t note){
    activeNotes_.insert(note);
    update();
}

void FilterOverlay::removeNote(uint8_t note){
    activeNotes_.erase(note);
    update();
}

void FilterOverlay::applyDrag(uint8_t pitch){
    if ( pitch == dragPitch_ ) return ;
    dragPitch_ = pitch ;

    bool isActive = activeNotes_.count(pitch) > 0 ;
    if ( isActive == dragActivating_ ) return ;

    update();
    emit requestActivateNote(pitch, dragActivating_);
}


void FilterOverlay::mousePressEvent(QMouseEvent* event){
    uint8_t pitch = piano_->pitchAt(event->pos());
    if ( pitch == 128 ) return ;

    dragActivating_ = activeNotes_.count(pitch) == 0 ;
    dragPitch_ = 128 ;
    applyDrag(static_cast<uint8_t>(pitch));
}

void FilterOverlay::mouseMoveEvent(QMouseEvent* event) {
    if ( !(event->buttons() & Qt::LeftButton) ) return ;
    uint8_t pitch = piano_->pitchAt(event->pos());
    if ( pitch == 128 ) return ;
    applyDrag(static_cast<uint8_t>(pitch));
}

void FilterOverlay::mouseReleaseEvent(QMouseEvent*) {
    dragPitch_ = 128 ;
}

void FilterOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor dimmed = Theme::MIDI_FILTER_TINT_COLOR  ;

    for ( uint8_t note = 0; note < 128; ++note ) {
        if ( activeNotes_.count(note) ) continue ;
        p.fillRect(piano_->getNoteDimensions(note), dimmed);
    }
}

MidiFilterWidget::MidiFilterWidget(ComponentModel* model, QWidget* parent):
    CollectionWidget(model, parent),
    piano_(new PianoWidget(this)),
    overlay_(new FilterOverlay(piano_, this))
{
    piano_->setVertical(false);

    auto* layout = new QStackedLayout(this);
    layout->setStackingMode(QStackedLayout::StackAll);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(piano_);
    layout->addWidget(overlay_);
    setLayout(layout);

    overlay_->raise();

    connect(
        overlay_, &FilterOverlay::requestActivateNote,
        this, &MidiFilterWidget::noteActivateRequested
    );
}

void MidiFilterWidget::noteActivateRequested(uint8_t note, bool active){
    CollectionRequest req ;
    req.componentId = model_->getId();

    if ( active ){
        req.action = CollectionAction::ADD ;
        req.value = note ;
    } else {
        req.action = CollectionAction::REMOVE ;
        int idx = getIndexForNote(note);
        if ( idx == -1 ){
            SPDLOG_WARN(
                "received note remove for note "
                "that is not present in map. Please investigate."
            );
            return ;
        }
        req.index = idx ;
    }

    emit collectionEdited(req);
}

void MidiFilterWidget::updateCollection(const CollectionRequest& req){
    switch(req.action){
    case CollectionAction::ADD:
        handleCollectionAdd(req);
        break ;
    case CollectionAction::REMOVE:
        handleCollectionRemove(req);
        break ;
    case CollectionAction::GET_ALL:
        handleCollectionGetAll(req);
        break ;
    default:
        SPDLOG_WARN(
            "received collection request with unexpected action: {}", 
            static_cast<json>(req).dump()
        );
        break ;
    }
}

void MidiFilterWidget::handleCollectionAdd(const CollectionRequest& req){
    uint8_t note = req.value.value();
    int index = req.index.value();

    active_[index] = note ;
    overlay_->insertNote(note);
}

void MidiFilterWidget::handleCollectionRemove(const CollectionRequest& req){
    int index = req.index.value();

    auto it = active_.find(index);
    if ( it == active_.end() ){
        SPDLOG_WARN(
            "received request to delete note with index {}, "
            "but element is not in map", 
            index
        );
        return ;
    }

    uint8_t note = it->second ;
    active_.erase(it);
    overlay_->removeNote(note);
}

void MidiFilterWidget::handleCollectionGetAll(const CollectionRequest& req){
    active_.clear();
    overlay_->clearActiveNotes();

    int idx = 0 ;
    for ( const auto& val : req.value.value() ){
        active_[idx++] = val ;
        overlay_->insertNote(val);
    }
}

int MidiFilterWidget::getIndexForNote(uint8_t note) const {
    for ( const auto& [idx, n] : active_ ){
        if ( note == n ) return idx ;
    }
    return -1 ;
}