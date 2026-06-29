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

BufferChopper::BufferChopper(ComponentModel* model, size_t channel, QWidget* parent):
    BufferWaveform(model, channel, parent)
{

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

    int shadeStartX, shadeEndX, handlePosX ;
    if ( start ){
        shadeStartX = sampleToX(0);
        shadeEndX = startPosX_ ;
        handlePosX = shadeEndX ;
    } else {
        shadeStartX = endPosX_ ;
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
    if ( event->type() == QEvent::HoverEnter ) {
        qDebug() << "Mouse Entered!";
        return true; 
    } else if ( event->type() == QEvent::HoverLeave ) {
        qDebug() << "Mouse Left!";
        return true;
    } else if ( event->type() == QEvent::HoverMove ) {
        return true;
    }
    
    return QWidget::event(event); // Let other events pass through
}

void BufferChopper::onParameterChanged(ParameterType p){
    if ( p == ParameterType::START_POSITION || p == ParameterType::DURATION ){
        calculateStartEndPos();
        update();
    }
}