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

#ifndef BUFFER_CHOPPER_HPP_
#define BUFFER_CHOPPER_HPP_

#include "widgets/BufferWaveform.hpp"
#include "models/ComponentModel.hpp"

class BufferChopper : public BufferWaveform {
    Q_OBJECT

private:
    int startPosX_ ;
    int endPosX_ ;

    bool isDragging_ ;
    bool DragStart_ ;
    int dragPosX_ ;

public:
    explicit BufferChopper(ComponentModel* model, size_t channel, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override ;
    void drawSelectionRegion(QPainter& painter, bool start);
    void drawSelectionHandle(QPainter& painter, int posX, bool dir);

    void resizeEvent(QResizeEvent* event) override ;
    void calculateStartEndPos();

    bool event(QEvent *event) override ;

public slots:
    void onParameterChanged(ParameterType p);
    
};

#endif // BUFFER_CHOPPER_HPP_

