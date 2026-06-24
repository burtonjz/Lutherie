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

    int sampleStart, sampleEnd ;
    if ( start ){
        sampleStart = 0 ;
        sampleEnd = std::get<float>(model_->getParameterValue(ParameterType::START_POSITION));
    } else {
        sampleStart = std::get<float>(model_->getParameterValue(ParameterType::DURATION));
        sampleEnd = totalNumSamples_ ;
    }
    qDebug() << "samples:" << sampleStart << sampleEnd ; 
    
    int startPos = sampleToX(sampleStart);
    int endPos   = sampleToX(sampleEnd);

    // shaded area
    QRect shaded = {
        startPos, Theme::WAVEFORM_MARGIN_TOP, 
        sampleEnd - sampleStart, Theme::WAVEFORM_FIXED_PLOT_HEIGHT
    };
    painter.fillRect(shaded, Theme::CHOPPER_SHADE_COLOR);

    // selection handle
    int handlePos = start ? endPos : startPos ;
    int tick = start ? Theme::CHOPPER_HANDLE_WIDTH : -Theme::CHOPPER_HANDLE_WIDTH ;
    int yTop = Theme::WAVEFORM_MARGIN_TOP ;
    int yBot = Theme::WAVEFORM_HEIGHT - Theme::WAVEFORM_MARGIN_BOTTOM ;

    painter.setPen(QPen(Theme::CHOPPER_HANDLE_COLOR, 2));
    
    painter.drawLine(
        handlePos, yTop,
        handlePos, yBot
    );
    painter.drawLine(
        handlePos, yTop,
        handlePos + tick, yTop
    );
    painter.drawLine(
        handlePos, yBot,
        handlePos + tick, yBot
    );
}

void BufferChopper::onParameterChanged(ParameterType p){
    if ( p == ParameterType::START_POSITION || p == ParameterType::DURATION ){
        update();
    }
}