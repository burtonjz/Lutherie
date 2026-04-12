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

#ifndef KNOB_WIDGET_HPP_
#define KNOB_WIDGET_HPP_
#include <QAbstractSlider>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSizePolicy>

// A rotary knob widget that is a drop-in replacement for QSlider.
// Inherits QAbstractSlider, so minimum/maximum/value/setValue/valueChanged
// all work identically. Drag vertically or scroll to change value.
class KnobWidget : public QAbstractSlider
{
    Q_OBJECT

private:
    bool dragging_ = false ;
    int dragStartY = 0 ;
    int dragStartValue_ = 0 ;

public:
    explicit KnobWidget(
        Qt::Orientation orientation = Qt::Horizontal,
        QWidget* parent = nullptr
    );

    QSize sizeHint() const override ;
    QSize minimumSizeHint() const override ;

protected:
    void paintEvent(QPaintEvent* event) override ;
    void mousePressEvent(QMouseEvent* event) override ;
    void mouseMoveEvent(QMouseEvent* event) override ;
    void mouseReleaseEvent(QMouseEvent* event) override ;
    void wheelEvent(QWheelEvent* event) override ;
    void keyPressEvent(QKeyEvent* event) override ;

private:
    double normalisedValue() const ;

    int normToSlider(double t) const ;

};

#endif // KNOB_WIDGET_HPP_