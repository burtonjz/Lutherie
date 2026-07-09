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
    if ( e->button() != Qt::LeftButton ) return ;

    NoteWidget* clickedNote = findNoteAtPos(pos);
    if ( clickedNote ){
        if ( cursor() == Qt::SizeHorCursor ){
            startResize(clickedNote, pos);
            e->accept();
            return ;
        }

        // wait to determine if it is a simple selection or a move
        actionedNote_ = clickedNote ;
        multiSelect_ = e->modifiers() & Qt::ControlModifier ;
        clickAction_ = ClickAction::Pending ;
        e->accept();
        return ;
    }

    if ( e->modifiers() & Qt::ControlModifier ){
        deselectNotes();
        startDrag(pos);
    } else {
        // wait to determine if it is a simple selection or a move
        actionedNote_ = clickedNote ;
        multiSelect_ = e->modifiers() & Qt::ControlModifier ;
        clickAction_ = ClickAction::Pending ;
        e->accept();
    }

    e->accept();
}

void PianoRollWidget::contentMouseMove(QMouseEvent* e){
    QPointF pos = e->position();

    switch ( clickAction_){
    case ClickAction::Dragging:  updateDrag(pos);         return ;
    case ClickAction::Resizing:  updateResize(pos);       return ;
    case ClickAction::Selecting: updateSelectionBox(pos); return ;
    case ClickAction::Moving:    updateMove(pos);         return ;
    case ClickAction::Pending: {
        // if we pass the drag threshold, it's either a move or a selection box
        if ( (pos - actionPos_).manhattanLength() < QApplication::startDragDistance() ) return ;
        
        if ( !selectedNotes_.contains(findNoteIndex(actionedNote_)) ){
            selectNote(actionedNote_, multiSelect_);
        } 

        if ( selectedNotes_.size() > 0 ){
            clickAction_ = ClickAction::Moving ;
            actionPos_ = pos ;
        } else {
            startSelectionBox(pos);
        }
    }
    default:
        updateCursor(pos);
        return ;
    }
}

void PianoRollWidget::contentMouseRelease(QMouseEvent* e){
    QPointF pos = e->position();
    if ( e->button() != Qt::LeftButton ) return ;

    switch ( clickAction_ ){
    case ClickAction::Dragging:  endDrag(pos);         break ;
    case ClickAction::Resizing:  endResize(pos);       break ;
    case ClickAction::Selecting: endSelectionBox(pos); break ;
    case ClickAction::Moving:    endMove(pos);         break ;
    case ClickAction::Pending:   
        selectNote(actionedNote_, multiSelect_); 
        break ;
    default:
        break ;
    }

    clickAction_ = ClickAction::None ;
    actionedNote_ = nullptr ;
    e->accept();
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

void PianoRollWidget::contentLeave(QEvent* e){
    unsetCursor();
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

    if ( clickAction_ == ClickAction::Selecting ){
        QColor fillColor = Theme::ACCENT_COLOR ;
        fillColor.setAlpha(30);
        p.setPen(QPen(Theme::TEXT_SECONDARY, 1));
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
        selectedNotes_.erase(idx);
    } else {
        note->setSelected(true);
        selectedNotes_.insert(idx);
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
    const qreal threshold = Theme::PIANO_ROLL_NOTE_EDGE_THRESHOLD ;
    NoteWidget* inThreshold = nullptr ;
    qreal thresholdDistance = threshold + 1 ;
    for ( auto& [idx, note] : notes_ ) {
        if ( !note ) continue ;
        
        auto geo = note->geometry();
        auto point = pos.toPoint();

        // clear fit, return immediately
        if ( geo.contains(point) ){
            return note ;
        }

        // within threshold, search for closest note
        auto threshGeo = geo.adjusted(-threshold, 0, threshold, 0);
        if ( threshGeo.contains(point) ){
            int dist = std::min(
                abs(point.x() - geo.left()),
                abs(point.x() - geo.right())
            );
            if ( dist < thresholdDistance ){
                inThreshold = note ;
                thresholdDistance = dist ;
            }
        }
    }

    return inThreshold ;
}

void PianoRollWidget::startDrag(const QPointF pos){
    anchorBeat_ = xToBeat(pos.x());
    uint8_t pitch = yToPitch(pos.y());
    actionedNote_ = new NoteWidget(pitch,100,anchorBeat_ + 0.25,anchorBeat_,roll_);
    clickAction_ = ClickAction::Dragging ;
}

void PianoRollWidget::updateDrag(const QPointF pos){
    float dragBeat = xToBeat(pos.x());
    actionedNote_->setBeatRange(anchorBeat_, dragBeat);
}

void PianoRollWidget::endDrag(const QPointF pos){
    float dragBeat = xToBeat(pos.x());
    actionedNote_->setBeatRange(anchorBeat_, dragBeat, true);

    if ( actionedNote_->getEndBeat() == actionedNote_->getStartBeat() ){
        actionedNote_->deleteLater();
        actionedNote_ = nullptr ;
        return ;
    } 

    CollectionRequest req ;
    req.action = CollectionAction::ADD ;
    req.componentId = model_->getId() ;
    req.value = actionedNote_->getNote() ;
    
    // clean up drag
    actionedNote_->deleteLater();
    
    emit collectionEdited(req);
}

void PianoRollWidget::startResize(NoteWidget* note, const QPointF pos){    
    clickAction_ = ClickAction::Resizing ;
    actionedNote_ = note ;

    // determine if pos is closer to left or right side
    QPointF notePos = note->mapFromParent(pos);
    if ( std::abs(notePos.x()) < std::abs(notePos.x() - note->width()) ){
        anchorBeat_ = note->getEndBeat();
    } else {
        anchorBeat_ = note->getStartBeat();
    }
}

void PianoRollWidget::updateResize(const QPointF pos){
    float dragBeat = xToBeat(pos.x());
    actionedNote_->setBeatRange(anchorBeat_, dragBeat);
}

void PianoRollWidget::endResize(const QPointF pos){
    float dragBeat = xToBeat(pos.x());
    if ( dragBeat > totalBeats_ ) dragBeat = totalBeats_ ;
    if ( dragBeat < 0.0f ) dragBeat = 0.0f ;
    actionedNote_->setBeatRange(anchorBeat_, dragBeat, true);

    int idx = findNoteIndex(actionedNote_);
    if ( idx == -1 ){
        SPDLOG_DEBUG("attempted to delete a note that has no index. Please investigate");
        return ;
    }

    // delete note if no length
    if ( actionedNote_->getEndBeat() == actionedNote_->getStartBeat() ){
        requestRemoveNote(idx);
        return ;
    } 

    // otherwise, update note
    CollectionRequest req ;
    req.action = CollectionAction::SET ;
    req.componentId = model_->getId() ;
    req.index =  idx ;
    req.value = actionedNote_->getNote() ;

    emit collectionEdited(req);
}

void PianoRollWidget::updateMove(const QPointF pos){
    if ( selectedNotes_.size() == 0 ){
        clickAction_ = ClickAction::None ;
        return ;
    }

    // y flipped because y coordinate increases in opposite direction of piano roll
    int numXSteps = ( pos.x() - actionPos_.x() ) / Theme::PIANO_ROLL_PIXELS_PER_BEAT * 8 ; // 8th note precision
    int numYSteps = ( actionPos_.y() - pos.y() ) / Theme::PIANO_KEY_THICKNESS ; 
        
    // x axis 8th note precision
    if ( std::abs(numXSteps) >= 1 ){
        updateSelectedNoteStart(numXSteps / 8.0f );
        actionPos_.setX(pos.x());
    }
    if ( std::abs(numYSteps) >= 1 ){
        updateSelectedNotePitch(numYSteps);
        actionPos_.setY(pos.y());
    }
}

void PianoRollWidget::endMove(const QPointF pos){
}

void PianoRollWidget::startSelectionBox(const QPointF pos){
    clickAction_ = ClickAction::Selecting ;
    selectionStart_ = pos ;
    selectionRect_ = QRect(pos.toPoint(), QSize(0,0));
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
            selectedNotes_.insert(idx);
        } else {
            note->setSelected(false);
            selectedNotes_.erase(idx);
        }
    }

    selectionRect_ = QRect();
    roll_->update();
}


void PianoRollWidget::updateSelectedNotePitch(int p){
    for ( int idx : selectedNotes_ ){
        NoteWidget* n = notes_[idx];
        if ( !n ) continue ;

        if ( n->getMidiNote() == 0 && p < 0 ){
            return ;
        } else if ( n->getMidiNote() == 127 && p > 0 ){
            return ;
        }

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
    
        float newStart = n->getStartBeat() + t ;
        float newEnd = n->getEndBeat() + t ;
        if ( newStart < 0 ) newStart = 0 ;
        if ( newEnd > totalBeats_ ) newEnd = totalBeats_ ;

        n->setBeatRange(newStart, newEnd);
        if ( n->getEndBeat() - n->getStartBeat() <= 0 ){
            requestRemoveNote(idx);
            return ;
        }

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
    const qreal threshold = Theme::PIANO_ROLL_NOTE_EDGE_THRESHOLD ;
    if ( n ){
        // if its within threshold of an edge, present resize cursor
        int dist = std::min(
            abs(pos.x() - n->geometry().left()),
            abs(pos.x() - n->geometry().right())
        );
        if ( dist < threshold ){
            setCursor(Qt::SizeHorCursor);
        } else {
            unsetCursor();
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
    selectedNotes_.erase(it->first);
    update();
}

void PianoRollWidget::onParameterChanged(ParameterType p){
    if ( p == ParameterType::DURATION ){
        setTotalBeats(std::get<float>(model_->getParameterValue(p)));
        return ;
    }
}