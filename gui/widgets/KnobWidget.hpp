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

class KnobWidget : public QAbstractSlider
{
    Q_OBJECT

private:
    bool dragging_ = false ;
    int dragStartY = 0 ;
    int dragStartValue_ = 0 ;
    int precision_ = 0 ;

public:
    explicit KnobWidget(
        Qt::Orientation orientation = Qt::Horizontal,
        QWidget* parent = nullptr
    );

    QSize sizeHint() const override ;
    QSize minimumSizeHint() const override ;

    int precision() const { return precision_ ; }
    void setPrecision(int p);

    double value() const    { return fromRaw(QAbstractSlider::value()); }
    
    double minimum() const  { return fromRaw(QAbstractSlider::minimum()); }
    void setMinimum(double v)  { QAbstractSlider::setMinimum(toRaw(v)); }

    double maximum() const  { return fromRaw(QAbstractSlider::maximum()); }
    void setMaximum(double v)  { QAbstractSlider::setMaximum(toRaw(v)); }

    void setRange(double lo, double hi) { QAbstractSlider::setRange(toRaw(lo), toRaw(hi)); }

    double singleStep() const { return fromRaw(QAbstractSlider::singleStep()); }
    void setSingleStep(double v) { QAbstractSlider::setSingleStep(toRaw(v)); }
    
    double pageStep() const   { return fromRaw(QAbstractSlider::pageStep()); }
    void setPageStep(double v)   { QAbstractSlider::setPageStep(toRaw(v)); }

protected:
    void paintEvent(QPaintEvent* event) override ;
    void mousePressEvent(QMouseEvent* event) override ;
    void mouseMoveEvent(QMouseEvent* event) override ;
    void mouseReleaseEvent(QMouseEvent* event) override ;
    void wheelEvent(QWheelEvent* event) override ;
    void keyPressEvent(QKeyEvent* event) override ;

private:
    double normalisedValue() const ;
    int toRaw(double v) const ;
    double fromRaw(int v) const ;

public slots:
    void setValue(double v, bool block = false);

signals:
    void valueChanged(double value);

};

#endif // KNOB_WIDGET_HPP_