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

 #ifndef MIDI_FILTER_WIDGET_HPP_
 #define MIDI_FILTER_WIDGET_HPP_

#include "widgets/PianoWidget.hpp"
#include "widgets/CollectionWidget.hpp"

class FilterOverlay : public QWidget {
    Q_OBJECT

private:
    PianoWidget* piano_ ;
    std::set<uint8_t> activeNotes_ ;
    bool dragActivating_ = false ;
    uint8_t dragPitch_ = 128 ;

public:
    explicit FilterOverlay(PianoWidget* piano, QWidget* parent = nullptr);

    const std::set<uint8_t>& activeNotes() const ;
    void setActiveNotes(std::set<uint8_t> notes);
    void clearActiveNotes();

    void insertNote(uint8_t note);
    void removeNote(uint8_t note);

protected:
    void paintEvent(QPaintEvent*) override ;
    void mousePressEvent(QMouseEvent* event) override ;
    void mouseMoveEvent(QMouseEvent* event) override ;
    void mouseReleaseEvent(QMouseEvent* event) override ;

private:
    void applyDrag(uint8_t pitch);

signals:
    void requestActivateNote(uint8_t note, bool active);
};

class MidiFilterWidget : public CollectionWidget {
    Q_OBJECT

private:
    PianoWidget* piano_ ;
    FilterOverlay* overlay_ ;
    std::map<int, uint8_t> active_ ;

public:
    explicit MidiFilterWidget(ComponentModel* model, QWidget* parent = nullptr);

    void updateCollection(const CollectionRequest& req) override ;

private:
    void handleCollectionAdd(const CollectionRequest& req);
    void handleCollectionRemove(const CollectionRequest& req);
    void handleCollectionGetAll(const CollectionRequest& req);

    int getIndexForNote(uint8_t note) const ;

public slots:
    void noteActivateRequested(uint8_t note, bool active);

};

 #endif // MIDI_FILTER_WIDGET_HPP_

