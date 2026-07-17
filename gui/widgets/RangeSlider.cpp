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

#include "widgets/RangeSlider.hpp"
#include "app/Theme.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <spdlog/spdlog.h>

RangeSlider::RangeSlider(
    double absMin, double absMax, 
    double curMin, double curMax,
    size_t precision, QWidget* parent
):
    QWidget(parent),
    minimum_(absMin),
    maximum_(absMax),
    precision_(precision),
    lower_(curMin),
    higher_(curMax),
    lowerPos_(valToPos(lower_)),
    higherPos_(valToPos(higher_)),
    isDragging_(false),
    dragLower_(false)
{    
    handlePosY_ = Theme::RANGE_SLIDER_TOP_MARGIN 
        + Theme::RANGE_SLIDER_HANDLE_RADIUS / 2.0 ;
    textPosY_ = Theme::RANGE_SLIDER_TOP_MARGIN 
        - Theme::RANGE_SLIDER_HANDLE_RADIUS ;

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

RangeSlider::RangeSlider(
        double absMin, double absMax, 
        size_t precision, QWidget* parent
):
    RangeSlider(absMin, absMax, absMin, absMax, precision, parent)
{}
    
QSize RangeSlider::sizeHint() const {
    return {
        Theme::RANGE_SLIDER_WIDTH + Theme::RANGE_SLIDER_X_MARGIN * 2,
        Theme::RANGE_SLIDER_HEIGHT
    };
}

double RangeSlider::getMinimum() const {
    return minimum_ ;
}

void RangeSlider::setMinimum(double min){
    if ( min > maximum_ ){
        SPDLOG_ERROR("cannot set minimum to value below maximum");
        return ;
    }
    minimum_ = min ;
}

double RangeSlider::getMaximum() const {
    return maximum_ ;
}

void RangeSlider::setMaximum(double max){
    if ( max < minimum_ ){
        SPDLOG_ERROR("cannot set maximum to value above minimum");
        return ;
    }
    maximum_ = max ;
}

size_t RangeSlider::getPrecision() const {
    return precision_ ;
}

void RangeSlider::setPrecision(size_t s){
    precision_ = s ;
}

void RangeSlider::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPoint lowerPos = QPoint(
        lowerPos_,
        handlePosY_
    );
    QPoint higherPos = QPoint(
        higherPos_,
        handlePosY_
    );

    p.setPen(Qt::NoPen);

    // bar from start to lower handle: unfilled
    p.setBrush(Theme::TEXT_SECONDARY);
    p.drawRect(QRectF(
        Theme::RANGE_SLIDER_X_MARGIN, 
        Theme::RANGE_SLIDER_TOP_MARGIN,
        lowerPos_ - Theme::RANGE_SLIDER_X_MARGIN,
        Theme::RANGE_SLIDER_HANDLE_RADIUS
    ));

    // bar from lower handle to higher handle: filled
    p.setBrush(Theme::ACCENT_COLOR);
    p.drawRect(QRectF(
        lowerPos_, 
        Theme::RANGE_SLIDER_TOP_MARGIN,
        higherPos_ - lowerPos_,
        Theme::RANGE_SLIDER_HANDLE_RADIUS
    ));

    // bar from higher handle to end: unfilled
    p.setBrush(Theme::TEXT_SECONDARY);
    p.drawRect(QRectF(
        higherPos_, 
        Theme::RANGE_SLIDER_TOP_MARGIN,
        Theme::RANGE_SLIDER_X_MARGIN + 
            Theme::RANGE_SLIDER_WIDTH - higherPos_,
        Theme::RANGE_SLIDER_HANDLE_RADIUS
    ));

    // draw handle circles
    p.setBrush(Theme::TEXT_PRIMARY);
    p.setPen(QPen(Theme::COMPONENT_BORDER, 1));

    p.drawEllipse(
        lowerPos,
        Theme::RANGE_SLIDER_HANDLE_RADIUS,
        Theme::RANGE_SLIDER_HANDLE_RADIUS
    );

    p.drawEllipse(
        higherPos,
        Theme::RANGE_SLIDER_HANDLE_RADIUS,
        Theme::RANGE_SLIDER_HANDLE_RADIUS
    ); 

    // draw text labels
    QString lowerText = QString::number(
        posToVal(lowerPos_), 'f', precision_
    );
    QString higherText = QString::number(
        posToVal(higherPos_), 'f', precision_
    );

    QFontMetrics fm = p.fontMetrics();
    p.setPen(Theme::TEXT_PRIMARY);

    int higherTextWidth = fm.horizontalAdvance(higherText);

    // lower label: left-aligned to the start of the plot
    p.drawText(
        Theme::RANGE_SLIDER_X_MARGIN,
        textPosY_,
        lowerText
    );

    // higher label: right-aligned to the end of the plot
    p.drawText(
        Theme::RANGE_SLIDER_X_MARGIN + Theme::RANGE_SLIDER_WIDTH - higherTextWidth,
        textPosY_,
        higherText
    );
}

void RangeSlider::mousePressEvent(QMouseEvent* e){
    QPointF pos = mapFromGlobal(e->globalPosition());

    if ( e->button() == Qt::LeftButton ){
        startDrag(pos);
    }
}

void RangeSlider::mouseMoveEvent(QMouseEvent* e){
    if ( !isDragging_ ) return ;

    QPointF pos = mapFromGlobal(e->globalPosition());
    updateDrag(pos);
}

void RangeSlider::mouseReleaseEvent(QMouseEvent* e){
    if ( !isDragging_ ) return ;
    if ( e->button() == Qt::LeftButton ){
        QPointF pos = mapFromGlobal(e->globalPosition());
        finishDrag(pos);
    }
}

double RangeSlider::valToPos(double val) const {
    return Theme::RANGE_SLIDER_X_MARGIN +
        ( val - minimum_ ) * Theme::RANGE_SLIDER_WIDTH
        / ( maximum_ - minimum_ );
}

double RangeSlider::posToVal(double posX) const {
    return ( ( posX - Theme::RANGE_SLIDER_X_MARGIN ) / 
        Theme::RANGE_SLIDER_WIDTH ) *
        ( maximum_ - minimum_ ) + minimum_ ;
}

void RangeSlider::startDrag(QPointF pos){
    QRectF lowerRect = {
        lowerPos_ - Theme::RANGE_SLIDER_HANDLE_RADIUS,
        handlePosY_ - Theme::RANGE_SLIDER_HANDLE_RADIUS * 2,
        Theme::RANGE_SLIDER_HANDLE_RADIUS * 2,
        Theme::RANGE_SLIDER_HANDLE_RADIUS * 4,
    };
    QRectF higherRect = {
        higherPos_ - Theme::RANGE_SLIDER_HANDLE_RADIUS,
        handlePosY_ - Theme::RANGE_SLIDER_HANDLE_RADIUS * 2,
        Theme::RANGE_SLIDER_HANDLE_RADIUS * 2,
        Theme::RANGE_SLIDER_HANDLE_RADIUS * 4,
    };

    if ( lowerRect.contains(pos) ){
        isDragging_ = true ;
        dragLower_ = true ;
        return ;
    }

    if ( higherRect.contains(pos) ){
        isDragging_ = true ;
        dragLower_ = false ;
        return ;
    }
}

void RangeSlider::updateDrag(QPointF pos){
    if ( dragLower_ ){
        lowerPos_ = std::clamp(
            pos.x(),
            static_cast<double>(Theme::RANGE_SLIDER_X_MARGIN),
            higherPos_ - Theme::RANGE_SLIDER_HANDLE_RADIUS * 2
        );
        lower_ = posToVal(lowerPos_);
    } else {
        higherPos_ = std::clamp(
            pos.x(),
            lowerPos_ + Theme::RANGE_SLIDER_HANDLE_RADIUS * 2,
            static_cast<double>(Theme::RANGE_SLIDER_WIDTH 
                + Theme::RANGE_SLIDER_X_MARGIN)
        );
        higher_ = posToVal(higherPos_);
    }
    update();
}

void RangeSlider::finishDrag(QPointF pos){
    isDragging_ = false ;
    emit rangeUpdated(lower_, higher_);
}
