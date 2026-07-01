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

#include "widgets/BufferChopper.hpp"
#include "app/Theme.hpp"

#include <QPainter>
#include <QMouseEvent>

BufferChopper::BufferChopper(ComponentModel* model, size_t channel, QWidget* parent):
    BufferWaveform(model, channel, parent),
    isDragging_(false)
{
    setAttribute(Qt::WA_Hover, true);
}

void BufferChopper::paintEvent(QPaintEvent* event){
    BufferWaveform::paintEvent(event);
    QPainter painter(this);
    drawSelectionRegion(painter, true);
    drawSelectionRegion(painter, false);
}

void BufferChopper::drawSelectionRegion(QPainter& painter, bool start){
    if ( !model_ ) return ;
    if ( totalNumSamples_ == 0 ) return ;

    // handle drag logic
    int startX, endX ;
    if ( isDragging_ ){
        startX = dragStart_ ? dragPosX_ : startPosX_ ;
        endX = dragStart_ ? endPosX_ : dragPosX_ ;
    } else {
        startX = startPosX_ ;
        endX = endPosX_ ;
    }

    // shaded region
    int shadeStartX, shadeEndX, handlePosX ;
    if ( start ){
        shadeStartX = sampleToX(0);
        shadeEndX = startX ;
        handlePosX = shadeEndX ;
    } else {
        shadeStartX = endX ;
        shadeEndX = sampleToX(totalNumSamples_);
        handlePosX = shadeStartX ;
    }

    // shaded area
    QRect shaded = {
        shadeStartX, Theme::WAVEFORM_MARGIN_TOP, 
        shadeEndX - shadeStartX, Theme::WAVEFORM_FIXED_PLOT_HEIGHT
    };
    painter.fillRect(shaded, Theme::CHOPPER_SHADE_COLOR);

    // selection handle
    drawSelectionHandle(painter, handlePosX, start);
}

void BufferChopper::drawSelectionHandle(QPainter& painter, int posX, bool dir){
    int yTop = Theme::WAVEFORM_MARGIN_TOP ;
    int yBot = Theme::WAVEFORM_HEIGHT - Theme::WAVEFORM_MARGIN_BOTTOM ;
    int tick = Theme::CHOPPER_HANDLE_WIDTH ;
    if ( !dir ) tick *= -1 ;

    painter.setPen(QPen(Theme::CHOPPER_HANDLE_COLOR, 2));
    
    painter.drawLine(
        posX, yTop,
        posX, yBot
    );
    painter.drawLine(
        posX, yTop,
        posX + tick, yTop
    );
    painter.drawLine(
        posX, yBot,
        posX + tick, yBot
    );
}

void BufferChopper::resizeEvent(QResizeEvent* event){
    BufferWaveform::resizeEvent(event);

    calculateStartEndPos();
    update();
}

void BufferChopper::calculateStartEndPos(){
    int start = std::get<float>(model_->getParameterValue(ParameterType::START_POSITION));
    int duration = std::get<float>(model_->getParameterValue(ParameterType::DURATION));
    startPosX_ = sampleToX(start);
    endPosX_ = sampleToX(start + duration);
}

bool BufferChopper::event(QEvent *event){
    // change cursor if in hover range
    if ( event->type() == QEvent::HoverMove ){
        QHoverEvent* e = dynamic_cast<QHoverEvent*>(event);
        QPointF pos = mapFromGlobal(e->globalPosition());
        setHoverCursor(pos);
        return true;
    }
    
    return QWidget::event(event);
}

void BufferChopper::mousePressEvent(QMouseEvent* e){
    QPointF pos = mapFromGlobal(e->globalPosition());

    if ( e->button() == Qt::LeftButton ){
        startDrag(pos);
    }
}

void BufferChopper::mouseMoveEvent(QMouseEvent* e){
    if ( !isDragging_ ) return ;

    QPointF pos = mapFromGlobal(e->globalPosition());
    updateDrag(pos);
}

void BufferChopper::mouseReleaseEvent(QMouseEvent* e){
    if ( !isDragging_ ) return ;

    if ( e->button() == Qt::LeftButton ){
        QPointF pos = mapFromGlobal(e->globalPosition());
        finishDrag(pos);
    }
}

void BufferChopper::setHoverCursor(QPointF pos){
    int startX, endX ;
    if ( isDragging_ ){
        startX = dragStart_ ? dragPosX_ : startPosX_ ;
        endX = dragStart_ ? endPosX_ : dragPosX_ ;
        return ;
    } else {
        startX = startPosX_ ;
        endX = endPosX_ ;
    }
    
    
    if ( abs(pos.x() - startX ) < Theme::CHOPPER_HANDLE_WIDTH * 2 ){
        if ( cursor() != Qt::SizeHorCursor ) setCursor(Qt::SizeHorCursor);
    } else if ( abs(pos.x() - endX ) < Theme::CHOPPER_HANDLE_WIDTH * 2 ){
        if ( cursor() != Qt::SizeHorCursor ) setCursor(Qt::SizeHorCursor);
    } else {
        unsetCursor();
    }
}

bool BufferChopper::startDrag(QPointF pos){
    if ( cursor() != Qt::SizeHorCursor ) return false ;

    if ( abs(pos.x() - startPosX_ ) < Theme::CHOPPER_HANDLE_WIDTH * 2 ){
        dragStart_ = true ;
        dragPosX_ = pos.x();
    } else {
        dragStart_ = false ;
        dragPosX_ = pos.x();
    }
    
    isDragging_ = true ;
    return true ;
}

void BufferChopper::updateDrag(QPointF pos){
    dragPosX_ = pos.x();
    if ( dragStart_ ){
        if ( dragPosX_ >= endPosX_ ){
            dragPosX_ = endPosX_ - Theme::CHOPPER_HANDLE_WIDTH ;
        }
        int plotStart = sampleToX(0);
        if ( dragPosX_ < plotStart ){
            dragPosX_ = plotStart ;
        }
    } else {
        if ( dragPosX_ < startPosX_ ){
            dragPosX_ = startPosX_ + Theme::CHOPPER_HANDLE_WIDTH ;
        }
        int plotEnd = plotWidth_ + Theme::WAVEFORM_MARGIN_LEFT ;
        if ( dragPosX_ > plotEnd ){
            dragPosX_ = plotEnd ;
        }
    }
    update();
}

void BufferChopper::finishDrag(QPointF pos){
    isDragging_ = false ;
    if ( dragStart_ ){
        startPosX_ = dragPosX_ ;
        emit parameterEdited(model_->getId(), ParameterType::START_POSITION, xToSample(startPosX_));
    } else {
        endPosX_ = dragPosX_ ;
        float duration = xToSample(endPosX_) - std::get<float>(model_->getParameterValue(ParameterType::START_POSITION));
        emit parameterEdited(model_->getId(), ParameterType::DURATION, duration);
    }
    update();
    
}

void BufferChopper::onParameterChanged(ParameterType p){
    if ( p == ParameterType::START_POSITION || p == ParameterType::DURATION ){
        calculateStartEndPos();
        update();
    }
}