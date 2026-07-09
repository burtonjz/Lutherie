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

#ifndef PIANOROLL_WIDGET_HPP_
#define PIANOROLL_WIDGET_HPP_

#include "widgets/CollectionWidget.hpp"
#include "models/ComponentModel.hpp"
#include "types/SequenceData.hpp"
#include "widgets/NoteWidget.hpp"
#include "widgets/PianoWidget.hpp"

#include <QWidget>
#include <QScrollArea>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QBoxLayout>
#include <set> 
#include <map>

class PianoRollWidget : public CollectionWidget {
    Q_OBJECT

private:
    // lightweight widget to support partial QScrollArea wrap
    class ContentWidget : public QWidget {
    private:
        PianoRollWidget* owner_ ;

    public:
        explicit ContentWidget(PianoRollWidget* owner, QWidget* parent = nullptr):
            QWidget(parent),
            owner_(owner)
        {
            setMouseTracking(true);
            setFocusPolicy(Qt::StrongFocus);
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
    
    protected:
        void paintEvent(QPaintEvent* event) override {
            owner_->paintContent(this, event);
        }
        QSize sizeHint() const override { return owner_->contentSizeHint(); }
        QSize minimumSizeHint() const override { return sizeHint(); }
        void mousePressEvent(QMouseEvent* e) override   { owner_->contentMousePress(e); }
        void mouseMoveEvent(QMouseEvent* e) override    { owner_->contentMouseMove(e); }
        void mouseReleaseEvent(QMouseEvent* e) override { owner_->contentMouseRelease(e); }
        void keyPressEvent(QKeyEvent* e) override       { owner_->contentKeyPress(e); }
        void keyReleaseEvent(QKeyEvent* e) override     { owner_->contentKeyRelease(e); }
        void leaveEvent(QEvent* e) override             { owner_->contentLeave(e); }
    };

    QHBoxLayout* layout_ ;
    QScrollArea* pianoScroll_ ;
    PianoWidget* piano_ ;
    QWidget* pianoContainer_ ;
    QWidget* pianoScrollSpacer_ ;
    QVBoxLayout* pianoLayout_ ;
    QScrollArea* rollScroll_ ;
    ContentWidget* roll_ ;

    std::map<int, NoteWidget*> notes_ ;
    std::set<int> selectedNotes_ ;

    float totalBeats_ ;

    enum class ClickAction {
        None,
        Pending, // click or drag?
        Selecting,
        Moving,
        Resizing,
        Dragging,
        Velocity
    };
    ClickAction clickAction_ ;

    NoteWidget* actionedNote_ ;
    bool multiSelect_ = false ;
    QPointF actionPos_ ;

    float anchorBeat_ ;
    
    QPointF selectionStart_ ;
    QRect selectionRect_ ;

    bool ctrlHeld_ = false ;

public:
    explicit PianoRollWidget(ComponentModel* model, QWidget* parent = nullptr);

    void updateCollection(const CollectionRequest& req) override ;
    
    void setTotalBeats(float beats);
    // void removeNote(int idx);

protected:
    void contentMousePress(QMouseEvent* e);
    void contentMouseMove(QMouseEvent* e);
    void contentMouseRelease(QMouseEvent* e);
    void contentKeyPress(QKeyEvent* e);
    void contentKeyRelease(QKeyEvent* e);
    void contentLeave(QEvent* e);

private:
    // content wrappers
    void paintContent(QWidget* target, QPaintEvent* event);
    QSize contentSizeHint() const ;

    float xToBeat(float x) const ;
    int yToPitch(float y) const ;

    // note handling
    int findNoteIndex(NoteWidget* note) const ;
    NoteWidget* findNoteAtPos(const QPointF& pos);
    
    // functions for Qt event handling
    void selectNote(NoteWidget* note, bool multiSelect);
    void deselectNotes();

    // dragging new notes
    void startDrag(const QPointF pos);
    void updateDrag(const QPointF pos);
    void endDrag(const QPointF pos);

    // resize existing note
    void startResize(NoteWidget* note, const QPointF pos);
    void updateResize(const QPointF pos);
    void endResize(const QPointF pos);

    // move existing note / velocity
    void updateMove(const QPointF pos);
    void updateVelocity(const QPointF pos);
    
    // selection
    void startSelectionBox(const QPointF pos);
    void updateSelectionBox(const QPointF pos);
    void endSelectionBox(const QPointF pos);

    void updateSelectedNotePitch(int p);
    void updateSelectedNoteStart(float t);
    void updateSelectedNoteDuration(float d);
    void updateSelectedNoteVelocity(int v);

    void updateCursor(const QPointF pos);

    // functions for api responses
    void handleCollectionAdd(const CollectionRequest& req);
    void handleCollectionRemove(const CollectionRequest& req);
    
    void onNoteAdded(SequenceNote note);
    void onNoteRemoved(SequenceNote note);

    void requestRemoveNote(int idx);
    void requestRemoveSelectedNotes();
    
public slots:
    // respond to parameter changes from the component model
    void onParameterChanged(ParameterType p);
};

#endif // PIANOROLL_WIDGET_HPP_