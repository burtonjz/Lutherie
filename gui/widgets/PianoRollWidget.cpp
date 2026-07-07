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

#include "widgets/PianoRollWidget.hpp"
#include "app/Theme.hpp"
#include "widgets/NoteWidget.hpp"
#include "requests/CollectionRequest.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QCursor>

PianoRollWidget::PianoRollWidget(ComponentModel* model, QWidget* parent):
    CollectionWidget(model, parent),
    pianoScroll_(new QScrollArea(this)),
    piano_(new PianoWidget(this)),
    pianoContainer_(new QWidget(this)),
    pianoScrollSpacer_(new QWidget(pianoContainer_)),
    pianoLayout_(new QVBoxLayout(pianoContainer_)),
    rollScroll_(new QScrollArea(this)),
    roll_(new ContentWidget(this)),
    layout_(new QHBoxLayout(this)),
    totalBeats_(16.0f),
    notes_(),
    selectedNotes_()
{
    layout_->setContentsMargins(0,0,0,0);
    layout_->setSpacing(0);

    // piano and roll scroll need to be in 2 synced scroll areas. 
    // the roll scroll will control both to keep in sync.
    pianoScroll_->setWidget(piano_);
    pianoScroll_->setWidgetResizable(true);
    pianoScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pianoScroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pianoScroll_->setFixedWidth(piano_->sizeHint().width());
    pianoScroll_->setFrameShape(QFrame::NoFrame);
    
    // account for rollScroll horizontal bar size
    int hBarHeight = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    pianoLayout_->setContentsMargins(0,0,0,0);
    pianoLayout_->setSpacing(0);
    pianoLayout_->addWidget(pianoScroll_);

    pianoScrollSpacer_->setFixedHeight(hBarHeight);
    pianoLayout_->addWidget(pianoScrollSpacer_);
    layout_->addWidget(pianoContainer_);

    rollScroll_->setWidget(roll_);
    rollScroll_->setWidgetResizable(true);
    rollScroll_->setFrameShape(QFrame::NoFrame);
    rollScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    rollScroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    rollScroll_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout_->addWidget(rollScroll_, 1);

    connect(
        rollScroll_->verticalScrollBar(), &QScrollBar::valueChanged,
        pianoScroll_->verticalScrollBar(), &QScrollBar::setValue
    );
    connect(
        rollScroll_->verticalScrollBar(), &QScrollBar::valueChanged,
        roll_, [this](int){ roll_->update(); }
    );
    connect(
        rollScroll_->horizontalScrollBar(), &QScrollBar::valueChanged,
        roll_, [this](int){ roll_->update(); }
    );
    
    update();
}

void PianoRollWidget::setTotalBeats(float beats){
    totalBeats_ = beats ;
    roll_->updateGeometry();
}

void PianoRollWidget::contentMousePress(QMouseEvent* e){
    QPointF pos = e->position();
    
    if ( e->button() == Qt::LeftButton ){
        NoteWidget* clickedNote = findNoteAtPos(pos);
        if ( clickedNote ){
            if ( startResize(clickedNote, pos) ){
                e->accept();
                return ;
            } 

            bool multiSelect = e->modifiers() & Qt::ControlModifier ;
            selectNote(clickedNote, multiSelect);
            e->accept();
            return ;
        }

        if ( e->modifiers() & Qt::ControlModifier ){
            deselectNotes();
            startDrag(pos);
        } else {
            startSelectionBox(pos);
        }

        e->accept();
        return ;
    }
}

void PianoRollWidget::contentMouseMove(QMouseEvent* e){
    QPointF pos = e->position();

    if ( isDragging_ ){    
        updateDrag(pos);
        return ;
    }

    if ( isResizing_ ){
        updateResize(pos);
        return ;
    }

    if ( isSelecting_ ){
        updateSelectionBox(pos);
        return ;
    }

    updateCursor(pos);
}

void PianoRollWidget::contentMouseRelease(QMouseEvent* e){
    QPointF pos = e->position();

    if ( e->button() == Qt::LeftButton ){
         if ( isDragging_ ){
            endDrag(pos);
            e->accept();
            return ;
         }

         if ( isResizing_ ){
            endResize(pos);
            e->accept();
            return ;
         }

         if ( isSelecting_ ){
            endSelectionBox(pos);
            e->accept();
            return ;
         }
    }
}

void PianoRollWidget::contentKeyPress(QKeyEvent* e){
    if ( e->key() == Qt::Key_Control ){
        ctrlHeld_ = true ;
        updateCursor(roll_->cursor().pos());
    }

    if ( e->key() == Qt::Key_Delete ){
        requestRemoveSelectedNotes();
        e->accept();
        return ;
    }

    if ( e->key() == Qt::Key_Up ){
        updateSelectedNotePitch(1);
        e->accept();
        return ;
    }
        
    if ( e->key() == Qt::Key_Down ){
        updateSelectedNotePitch(-1);
        e->accept();
        return ;
    }

    if ( e->key() == Qt::Key_Right ){
        if ( e->modifiers() & Qt::ControlModifier ){
            updateSelectedNoteDuration(0.125);
        } else {
            updateSelectedNoteStart(0.125);
        }        
        e->accept();
        return ;
    }

    if ( e->key() == Qt::Key_Left ){
        if ( e->modifiers() & Qt::ControlModifier ){
            updateSelectedNoteDuration(-0.125);
        } else {
            updateSelectedNoteStart(-0.125);
        }
        e->accept();
        return ;
    }
}

void PianoRollWidget::contentKeyRelease(QKeyEvent* e){
    if ( e->key() == Qt::Key_Control  ){
        ctrlHeld_ = false ;
        updateCursor(roll_->cursor().pos());
    }
}

void PianoRollWidget::paintContent(QWidget* target, QPaintEvent* event){
    QPainter p(target);

    // vertical beats
    for ( int beat = 0; beat <= totalBeats_ ; ++beat ){
        int x = beat * Theme::PIANO_ROLL_PIXELS_PER_BEAT ;

        p.setPen(QPen(
            Theme::PIANO_ROLL_BACKGROUND, 
            Theme::PIANO_ROLL_GRID_PEN_WIDTH_PRIMARY
        ));
        p.drawLine(x, 0, x, target->height());

        p.setPen(Theme::TEXT_SECONDARY);

        int vOffset = rollScroll_->verticalScrollBar()->value();
        p.drawText(
            QPoint(
                x + Theme::PIANO_KEY_LABEL_PAD, 
                vOffset + fontMetrics().ascent() + Theme::PIANO_KEY_LABEL_PAD 
            ), 
            QString("%1").arg(beat + 1)
        );
    }

    // vertical beat subdivisions
    for ( float beat = 0 ; beat <= totalBeats_ ; beat += 0.25f ){
        if ( std::fmod(beat, 1.0) < .0001 ){
            p.setPen(QPen(Theme::PIANO_ROLL_GRID_PRIMARY, Theme::PIANO_ROLL_GRID_PEN_WIDTH_SECONDARY));
        } else {
            p.setPen(QPen(Theme::PIANO_ROLL_GRID_SECONDARY, Theme::PIANO_ROLL_GRID_PEN_WIDTH_SECONDARY));
        }
        int x = beat * Theme::PIANO_ROLL_PIXELS_PER_BEAT ;
        p.drawLine(x,0,x,target->height());
    }

    // horizontal notes
    for ( uint8_t note = 0 ; note < 128; ++note ){
        int y = note * Theme::PIANO_KEY_THICKNESS ;
        QColor keyColor ;
        if ( piano_->isWhiteNote(127 - note) ){
            keyColor = Theme::PIANO_ROLL_KEY_WHITE ;
        } else {
            keyColor = Theme::PIANO_ROLL_KEY_BLACK ;
        }
        keyColor.setAlpha(30);
        p.setPen(QPen(keyColor, Theme::PIANO_ROLL_GRID_PEN_WIDTH_PRIMARY));
        p.fillRect(0, y, target->width(), Theme::PIANO_KEY_THICKNESS, keyColor);
    }

    if ( isSelecting_ ){
        QColor fillColor = Theme::ACCENT_COLOR ;
        fillColor.setAlpha(30);
        p.setPen(QPen(Theme::TEXT_SECONDARY, 1, Qt::DashLine));
        p.drawRect(selectionRect_);
        p.fillRect(selectionRect_, fillColor);
    }
}

QSize PianoRollWidget::contentSizeHint() const {
    return {
        static_cast<int>(totalBeats_ * Theme::PIANO_ROLL_PIXELS_PER_BEAT),
        static_cast<int>(128 * Theme::PIANO_KEY_THICKNESS) 
    };
}

float PianoRollWidget::xToBeat(float x) const {
    return x / Theme::PIANO_ROLL_PIXELS_PER_BEAT ;
}

int PianoRollWidget::yToPitch(float y) const {
    float row = ( y - Theme::PIANO_KEY_THICKNESS / 2.0 ) / Theme::PIANO_KEY_THICKNESS ;
    return 127 - static_cast<int>(std::round(row));
}

void PianoRollWidget::selectNote(NoteWidget* note, bool multiSelect){
    int idx = findNoteIndex(note);

    if ( !note || idx == -1 ){
        deselectNotes();
        return ;
    }

    if ( !multiSelect ){
        deselectNotes();
    }

    if ( note->isSelected() ){    
        note->setSelected(false);
        selectedNotes_.erase(std::remove(selectedNotes_.begin(), selectedNotes_.end(), idx), selectedNotes_.end());
    } else {
        note->setSelected(true);
        selectedNotes_.push_back(idx);
    }
}

void PianoRollWidget::deselectNotes(){
    for ( int idx : selectedNotes_ ){
        auto it = notes_.find(idx);
        if ( it != notes_.end() && it->second ){
            it->second->setSelected(false);
        }
    }
    selectedNotes_.clear();
}

void PianoRollWidget::requestRemoveNote(int idx){
    CollectionRequest req ;
    req.action = CollectionAction::REMOVE ;
    req.index = idx ;
    req.componentId = model_->getId() ;
    
    emit collectionEdited(req);
};

void PianoRollWidget::requestRemoveSelectedNotes(){
    for ( int idx : selectedNotes_ ){
        requestRemoveNote(idx);
    }
}

int PianoRollWidget::findNoteIndex(NoteWidget* note) const {
    for (const auto& [idx, widget] : notes_) {
        if (widget == note) {
            return idx;
        }
    }
    return -1;  // Not found
}

NoteWidget* PianoRollWidget::findNoteAtPos(const QPointF& pos) {
    for ( auto& [idx, note] : notes_ ) {
        if (note && note->geometry().contains(pos.toPoint())) {
            return note;
        }
    }
    return nullptr;
}

void PianoRollWidget::startDrag(const QPointF pos){
    anchorBeat_ = xToBeat(pos.x());
    uint8_t pitch = yToPitch(pos.y());
    dragNote_ = new NoteWidget(pitch,100,anchorBeat_ + 0.25,anchorBeat_,roll_);
    isDragging_ = true ;
}

void PianoRollWidget::updateDrag(const QPointF pos){
    float dragBeat = xToBeat(pos.x());
    dragNote_->setBeatRange(anchorBeat_, dragBeat);
}

void PianoRollWidget::endDrag(const QPointF pos){
    float dragBeat = xToBeat(pos.x());
    dragNote_->setBeatRange(anchorBeat_, dragBeat, true);

    if ( dragNote_->getEndBeat() == dragNote_->getStartBeat() ){
        dragNote_->deleteLater();
        dragNote_ = nullptr ;
        isDragging_ = false ;
        return ;
    } 

    CollectionRequest req ;
    req.action = CollectionAction::ADD ;
    req.componentId = model_->getId() ;
    req.value = dragNote_->getNote() ;
    
    // clean up drag
    dragNote_->deleteLater();
    dragNote_ = nullptr ;
    isDragging_ = false ;
    
    emit collectionEdited(req);
}

bool PianoRollWidget::startResize(NoteWidget* note, const QPointF pos){
    // if it's within edge threshold, start a resize
    QPointF notePos = note->mapFromParent(pos); // this is a local position
    const qreal threshold = Theme::PIANO_ROLL_NOTE_EDGE_THRESHOLD ;
    if ( notePos.x() <= threshold ){ // left side click
        isResizing_ = true ;
        anchorBeat_ = note->getEndBeat() ;
        dragNote_ = note ;
        return true ;
    } else if ( notePos.x() >= note->width() - threshold ){ // right side click
        isResizing_ = true ;
        anchorBeat_ = note->getStartBeat() ;
        dragNote_ = note ;
        return true ;
    }

    return false ;
}

void PianoRollWidget::updateResize(const QPointF pos){
    float dragBeat = xToBeat(pos.x());
    dragNote_->setBeatRange(anchorBeat_, dragBeat);
}

void PianoRollWidget::endResize(const QPointF pos){
    float dragBeat = xToBeat(pos.x());
    dragNote_->setBeatRange(anchorBeat_, dragBeat, true);


    int idx = findNoteIndex(dragNote_);
    
    // validate index
    if ( idx == -1 ){
        SPDLOG_DEBUG("attempted to delete a note that has no index. Please investigate");
        return ;
    }

    // delete note if no length
    if ( dragNote_->getEndBeat() == dragNote_->getStartBeat() ){
        requestRemoveNote(idx);
        dragNote_ = nullptr ;
        isResizing_ = false ;
        return ;
    } 

    // otherwise, update note
    CollectionRequest req ;
    req.action = CollectionAction::SET ;
    req.componentId = model_->getId() ;
    req.index =  idx ;
    req.value = dragNote_->getNote() ;
    
    isResizing_ = false ;
    dragNote_ = nullptr ;

    emit collectionEdited(req);
}

void PianoRollWidget::startSelectionBox(const QPointF pos){
    deselectNotes();
    selectionStart_ = pos ;
    selectionRect_ = QRect(pos.toPoint(), QSize(0,0));
    isSelecting_ = true ;
}

void PianoRollWidget::updateSelectionBox(const QPointF pos){
    selectionRect_ = QRect(selectionStart_.toPoint(), pos.toPoint()).normalized();
    roll_->update();
}

void PianoRollWidget::endSelectionBox(const QPointF pos){
    selectionRect_ = QRect(selectionStart_.toPoint(), pos.toPoint()).normalized();
    
    for ( auto& [idx, note] : notes_ ){
        if ( !note ) continue ;
        if ( selectionRect_.intersects(note->geometry()) ){
            note->setSelected(true);
            selectedNotes_.push_back(idx);
        }
    }

    isSelecting_ = false ;
    selectionRect_ = QRect();
    roll_->update();
}


void PianoRollWidget::updateSelectedNotePitch(int p){
    for ( int idx : selectedNotes_ ){
        NoteWidget* n = notes_[idx];
        if ( !n ) continue ;

        n->setMidiNote(n->getMidiNote() + p);
        
        CollectionRequest req ;
        req.action = CollectionAction::SET ;
        req.index = idx ;
        req.componentId = model_->getId() ;
        req.value = n->getNote() ;

        emit collectionEdited(req);
    }
}

void PianoRollWidget::updateSelectedNoteStart(float t){
    for ( int idx : selectedNotes_ ){
        NoteWidget* n = notes_[idx];
        if ( !n ) continue ;
    
        n->setBeatRange(n->getStartBeat() + t, n->getEndBeat() + t);
        CollectionRequest req ;
        req.action = CollectionAction::SET ;
        req.index = idx ;
        req.componentId = model_->getId() ;
        req.value = n->getNote() ;

        emit collectionEdited(req);
    }
}

void PianoRollWidget::updateSelectedNoteDuration(float d){
    for ( int idx : selectedNotes_ ){
        NoteWidget* n = notes_[idx];
        if ( !n ) continue ;
        
        n->setEndBeat(n->getEndBeat() + d);
        CollectionRequest req ;
        req.action = CollectionAction::SET ;
        req.index = idx ;
        req.componentId = model_->getId() ;
        req.value = n->getNote() ;

        emit collectionEdited(req);
    }
}

void PianoRollWidget::updateCursor(const QPointF pos){
    NoteWidget* n = findNoteAtPos(pos);
    if ( n ){
        QPointF notePos = n->mapFromParent(pos); // note local position
        const qreal threshold = Theme::PIANO_ROLL_NOTE_EDGE_THRESHOLD ;

        if ( notePos.x() <= threshold || notePos.x() >= n->width() - threshold ){
            setCursor(Qt::SizeHorCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        return ;
    } 

    setCursor(ctrlHeld_ ? Qt::CrossCursor : Qt::ArrowCursor);
}

void PianoRollWidget::updateCollection(const CollectionRequest& req){    
    switch(req.action){
    case CollectionAction::ADD:
        handleCollectionAdd(req);
        break ;
    case CollectionAction::REMOVE:
        handleCollectionRemove(req);
        break ;
    default:
        break ;
    }
}

void PianoRollWidget::handleCollectionAdd(const CollectionRequest& req){
    SequenceNote note = req.value.value();
    int index = req.index.value();
    notes_[index] = new NoteWidget(note, roll_);
    
    update();
}

void PianoRollWidget::handleCollectionRemove(const CollectionRequest& req){
    int index = req.index.value();
        
    auto it = notes_.find(index);
    if ( it == notes_.end() ){
        SPDLOG_DEBUG("received request to delete note with index {}, but element is not in map", 
            index);
        return ;
    }

    delete it->second ;
    notes_.erase(it);
    update();
}

void PianoRollWidget::onParameterChanged(ParameterType p){
    if ( p == ParameterType::DURATION ){
        setTotalBeats(std::get<float>(model_->getParameterValue(p)));
        return ;
    }
}