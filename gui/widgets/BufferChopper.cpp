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

void BufferChopper::updateCollection(const CollectionRequest& req){
    // only responds to SET -- BufferChopper only supports one min/max combo.
    switch(req.action){
        case CollectionAction::SET:
        case CollectionAction::GET_ALL:
            break ;
        default:
            qWarning() << "unsupported collection action received for buffer chopper" ;
            return ;
    }

    const CollectionDescriptor& cd = model_->getDescriptor().getCollection();
    if ( !req.valid(cd) ){
        qWarning() << "Invalid collection request received for buffer chopper." ;
        return ;
    }
    
    nlohmann::basic_json<> sampleRange = req.value.value();
    startSample_ = sampleRange[0];
    endSample_ = sampleRange[1];

    startPosX_ = sampleToX(startSample_);
    endPosX_   = sampleToX(endSample_);

    qDebug() << "collection update resulted in startSample=" << startSample_ << ", endSample=" << endSample_ << ", startPosX_=" << startPosX_
        << "endPosX_=" << endPosX_ ;
    update();
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

    startPosX_ = sampleToX(startSample_);
    endPosX_ = sampleToX(endSample_);
    update();
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
        startSample_ = xToSample(startPosX_);
        sendCollectionUpdate();
    } else {
        endPosX_ = dragPosX_ ;
        endSample_ = xToSample(endPosX_);
        sendCollectionUpdate();
    }
    update();
    
}

void BufferChopper::sendCollectionUpdate(){
    json val({xToSample(startPosX_), xToSample(endPosX_)});
    CollectionRequest req = {
        .action = CollectionAction::SET,
        .componentId = model_->getId(),
        .value = val,
        .index = 0
    };
    emit collectionEdited(req);
}