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

#include "widgets/KnobWidget.hpp"
#include "app/Theme.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QtMath>
#include <algorithm>

KnobWidget::KnobWidget(Qt::Orientation orientation, QWidget* parent): 
    QAbstractSlider(parent)
{
    Q_UNUSED(orientation)
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
}

QSize KnobWidget::sizeHint() const {
    return { 
        Theme::KNOB_WIDGET_SIZE, 
        Theme::KNOB_WIDGET_SIZE 
    };
}

QSize KnobWidget::minimumSizeHint() const {
    return sizeHint();
}

double KnobWidget::normalisedValue() const {
    if ( maximum() == minimum() ) return 0.0 ;
    return static_cast<double>(value() - minimum()) /
           static_cast<double>(maximum() - minimum());
}

int KnobWidget::normToSlider(double t) const {
    t = std::clamp(t, 0.0, 1.0);
    return minimum() + static_cast<int>(std::round(t * (maximum() - minimum())));
}

void KnobWidget::paintEvent(QPaintEvent*){
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect();
    const double cx = bounds.width()  / 2.0 ;
    const double cy = bounds.height() / 2.0 ;
    const double side = std::min(bounds.width(), bounds.height());

    const double arcRadius  = side * 0.38 ;
    const double knobRadius = side * 0.28 ;
    const double arcWidth   = Theme::KNOB_WIDGET_ARC_WIDTH;

    // Arc angles: Qt uses 1/16th degree units, measured clockwise from 3 o'clock.
    // QPainter::drawArc uses counter-clockwise spans, so sweep is negative.
    const double startDeg = Theme::KNOB_WIDGET_START_ANGLE ;
    const double sweepDeg = Theme::KNOB_WIDGET_SWEEP_ANGLE ;
    const double t = normalisedValue();
    const double valueDeg = sweepDeg * t ;

    const int qtStart = static_cast<int>((startDeg) * 16.0); // convert to Qt angle space
    const int qtSweep = static_cast<int>(-sweepDeg * 16.0);
    const int qtValSweep = static_cast<int>(-valueDeg * 16.0);

    const QRectF arcRect(
        cx - arcRadius, cy - arcRadius,
        arcRadius * 2.0, arcRadius * 2.0
    );

    // Track arc
    QPen trackPen(Theme::KNOB_WIDGET_TRACK_COLOR, arcWidth, Qt::SolidLine, Qt::RoundCap);
    p.setPen(trackPen);
    p.drawArc(arcRect, qtStart, qtSweep);

    // Value arc (filled portion)
    if (t > 0.0) {
        QPen valuePen(
            dragging_ ? Theme::KNOB_WIDGET_HIGHLIGHT_COLOR
                            : Theme::KNOB_WIDGET_ARC_COLOR,
            arcWidth, Qt::SolidLine, Qt::RoundCap
        );
        p.setPen(valuePen);
        p.drawArc(arcRect, qtStart, qtValSweep);
    }

    // Knob body
    const QRectF knobRect(
        cx - knobRadius, cy - knobRadius,
        knobRadius * 2.0, knobRadius * 2.0
    );
    QColor bodyColor = dragging_ ? Theme::KNOB_WIDGET_HIGHLIGHT_COLOR.darker(180)
        : Theme::KNOB_WIDGET_BODY_COLOR ;
    p.setBrush(bodyColor);
    p.setPen(QPen(Theme::KNOB_WIDGET_BODY_BORDER_COLOR, 1.0));
    p.drawEllipse(knobRect);

    // Notch (pointer line)
    // Angle of current value in Qt painter space (degrees from 3 o'clock, CCW positive)
    const double notchDeg = startDeg - valueDeg ;
    const double notchRad = qDegreesToRadians(notchDeg);
    const QPointF inner(
        cx + std::cos(notchRad) * knobRadius * 0.35,
        cy - std::sin(notchRad) * knobRadius * 0.35
    );
    const QPointF outer(
        cx + std::cos(notchRad) * knobRadius * 0.88,
        cy - std::sin(notchRad) * knobRadius * 0.88
    );

    QPen notchPen(
        Theme::KNOB_WIDGET_NOTCH_COLOR, 2.0, 
        Qt::SolidLine, Qt::RoundCap
    );
    p.setPen(notchPen);
    p.drawLine(inner, outer);
}

void KnobWidget::mousePressEvent(QMouseEvent* event){
    if ( event->button() == Qt::LeftButton ){
        dragging_ = true;
        dragStartY = event->pos().y();
        dragStartValue_ = value();
        update();
    }
}

void KnobWidget::mouseMoveEvent(QMouseEvent* event){
    if ( !dragging_ ) return ;

    const int dy = dragStartY - event->pos().y(); // up = increase
    const double range = static_cast<double>(maximum() - minimum());
    const double delta = (static_cast<double>(dy) / Theme::KNOB_WIDGET_DRAG_SENSITIVITY) * range ;
    const int newVal = std::clamp(
        static_cast<int>(std::round(dragStartValue_ + delta)),
        minimum(), maximum()
    );

    if ( newVal != value() ){
        setValue(newVal);
    }
}

void KnobWidget::mouseReleaseEvent(QMouseEvent* event){
    if ( event->button() == Qt::LeftButton ) {
        dragging_ = false ;
        update();
    }
}

void KnobWidget::wheelEvent(QWheelEvent* event){
    const int steps = event->angleDelta().y() / 120 ;
    setValue(std::clamp(value() + steps * singleStep(), minimum(), maximum()));
    event->accept();
}

void KnobWidget::keyPressEvent(QKeyEvent* event){
    switch ( event->key() ) {
        case Qt::Key_Left:
        case Qt::Key_Down:
            setValue(std::clamp(value() - singleStep(), minimum(), maximum()));
            break ;
        case Qt::Key_Right:
        case Qt::Key_Up:
            setValue(std::clamp(value() + singleStep(), minimum(), maximum()));
            break ;
        case Qt::Key_PageDown:
            setValue(std::clamp(value() - pageStep(), minimum(), maximum()));
            break ;
        case Qt::Key_PageUp:
            setValue(std::clamp(value() + pageStep(), minimum(), maximum()));
            break ;
        case Qt::Key_Home:
            setValue(minimum());
            break ;
        case Qt::Key_End:
            setValue(maximum());
            break ;
        default:
            QAbstractSlider::keyPressEvent(event);
    }
}