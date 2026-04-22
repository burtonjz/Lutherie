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

#include "widgets/OscilloscopeWidget.hpp"
#include "config/Config.hpp"
#include "app/Theme.hpp"

#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>

OscilloscopeWidget::OscilloscopeWidget(QWidget* parent):
    QWidget(parent),
    controls_(new GraphLayerControls(this)),
    layerData_(),
    sampleRate_(Config::get<double>("audio.sample_rate").value()),
    minAmp_(Theme::OSCILLOSCOPE_MIN_AMPLITUDE),
    maxAmp_(Theme::OSCILLOSCOPE_MAX_AMPLITUDE),
    updateTimer_(new QTimer(this)),
    dataReady_(false)
{
    Config::load();

    sampleRate_ = Config::get<float>("audio.sample_rate").value_or(44100);
    updateTimer_->setInterval(16); 

    int footerY = height() - Theme::OSCILLOSCOPE_MARGIN_BOTTOM + 8;
    controls_->setGeometry(
        Theme::OSCILLOSCOPE_MARGIN_LEFT,
        footerY,
        width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT,
        Theme::OSCILLOSCOPE_MARGIN_BOTTOM - 12
    );

    connect(updateTimer_, &QTimer::timeout, this, &OscilloscopeWidget::onUpdateTimeout);
    updateTimer_->start();
}

void OscilloscopeWidget::setAmplitudeRange(float minAmp, float maxAmp){
    minAmp_ = minAmp;
    maxAmp_ = maxAmp;
    update();
}

void OscilloscopeWidget::setSampleRate(float sampleRate){
    sampleRate_ = sampleRate;
    update();
}

void OscilloscopeWidget::addLayer(int componentId, const QString& label){
    if ( controls_->isLayerPresent(componentId) ) return;
    controls_->addLayer(componentId, label);
    layerData_[componentId];
}

void OscilloscopeWidget::removeLayer(int componentId){
    if ( !controls_->isLayerPresent(componentId) ) return;
    controls_->removeLayer(componentId);
    layerData_.erase(componentId);
}

void OscilloscopeWidget::renameLayer(int componentId, const QString& label){
    controls_->renameLayer(componentId, label);
}

void OscilloscopeWidget::toggleLayer(int componentId, bool enabled){
    controls_->toggleLayer(componentId, enabled);
}

void OscilloscopeWidget::onData(int componentId, const float* data, size_t count){
    if ( !controls_->isLayerPresent(componentId) ) return;

    auto& layerBuffer = layerData_.at(componentId);
    layerBuffer.assign(data, data + count);

    dataReady_ = true;
}

void OscilloscopeWidget::onUpdateTimeout(){
    if ( dataReady_ ){
        renderToCache();
        dataReady_ = false;
    } 
    
    if ( !cachedFrame_.isNull() ){
        // phosphor fade regardless of new data input
        QPainter fade(&cachedFrame_);
        fade.fillRect(cachedFrame_.rect(), QColor(0, 0, 0, 10));
    }
    
    update();
}

void OscilloscopeWidget::paintEvent(QPaintEvent* event){
    Q_UNUSED(event);
    QPainter painter(this);
    if ( !cachedFrame_.isNull() ){
        painter.drawImage(0, 0, cachedFrame_);
    }
    drawGrid(painter);
    drawLabels(painter);
}

void OscilloscopeWidget::resizeEvent(QResizeEvent* event){
    Q_UNUSED(event)
    if ( !cachedFrame_.isNull() ){
        cachedFrame_ = cachedFrame_.scaled(
            event->size(), 
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        );
    }
    int footerY = height() - Theme::OSCILLOSCOPE_MARGIN_BOTTOM + 28;
    controls_->setGeometry(
        Theme::OSCILLOSCOPE_MARGIN_LEFT,
        footerY,
        width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT,
        Theme::OSCILLOSCOPE_MARGIN_BOTTOM - 12
    );
    update();
}

void OscilloscopeWidget::drawGrid(QPainter& painter){
    painter.setPen(Theme::OSCILLOSCOPE_GRID_COLOR);

    int plotWidth = width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT;
    int plotHeight = height() - Theme::OSCILLOSCOPE_MARGIN_TOP - Theme::OSCILLOSCOPE_MARGIN_BOTTOM;

    // center line
    int centerY = static_cast<int>(amplitudeToY(0.0f));
    painter.setPen(QPen(Theme::OSCILLOSCOPE_GRID_COLOR, 1));
    painter.drawLine(Theme::OSCILLOSCOPE_MARGIN_LEFT, centerY,
                     Theme::OSCILLOSCOPE_MARGIN_LEFT + plotWidth, centerY);

    // +0.5 and -0.5 lines
    painter.setPen(QPen(Theme::OSCILLOSCOPE_GRID_COLOR, 1, Qt::DotLine));
    for ( float amp : {-0.5f, 0.5f} ){
        int y = static_cast<int>(amplitudeToY(amp));
        painter.drawLine(Theme::OSCILLOSCOPE_MARGIN_LEFT, y,
                         Theme::OSCILLOSCOPE_MARGIN_LEFT + plotWidth, y);
    }

    // vertical time divisions
    int divisions = 8 ;
    for ( int d = 1; d < divisions; ++d ){
        int x = Theme::OSCILLOSCOPE_MARGIN_LEFT + (plotWidth * d / divisions);
        painter.drawLine(x, Theme::OSCILLOSCOPE_MARGIN_TOP,
                         x, Theme::OSCILLOSCOPE_MARGIN_TOP + plotHeight);
    }
}

void OscilloscopeWidget::drawWaveform(QPainter& painter){
    for ( auto& [id, data] : layerData_ ){
        if ( data.empty() || !controls_->isLayerEnabled(id) ) continue ;

        QPainterPath path ;
        bool firstPoint = true ;

        for ( size_t i = 0; i < data.size(); ++i ){
            float x = sampleToX(i, data.size());
            float y = amplitudeToY(std::clamp(data[i], minAmp_, maxAmp_));

            if ( firstPoint ){
                path.moveTo(x, y);
                firstPoint = false;
            } else {
                path.lineTo(x, y);
            }
        }

        painter.setPen(QPen(controls_->layerColor(id), 1.5));
        painter.drawPath(path);
    }
}

void OscilloscopeWidget::drawLabels(QPainter& painter){
    painter.setPen(Theme::COMPONENT_TEXT);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    int plotWidth = width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT ;
    int plotHeight = height() - Theme::OSCILLOSCOPE_MARGIN_TOP - Theme::OSCILLOSCOPE_MARGIN_BOTTOM ;

    // Y-axis amplitude labels
    float dAmp = (Theme::OSCILLOSCOPE_MAX_AMPLITUDE - Theme::OSCILLOSCOPE_MIN_AMPLITUDE) / 5.0 ;
    for ( float amp : Theme::OSCILLOSCOPE_AMPLITUDE_LABELS ){
        int y = static_cast<int>(amplitudeToY(amp));
        QString label = QString::number(amp, 'f', 2);
        painter.drawText(5, y + 5, label);
    }

    // X-axis time division labels
    int divisions = 8 ;
    int windowSize = Config::get<int>("analysis.oscilloscope.window_size").value_or(1024);
    for ( int d = 0; d <= divisions; ++d ){
        size_t sample = (windowSize * d) / divisions ;
        float timeMs = (sample / sampleRate_) * 1000.0f ;
        int x = Theme::OSCILLOSCOPE_MARGIN_LEFT + (plotWidth * d / divisions);
        QString label = QString::number(timeMs, 'f', 1) + "ms" ;
        painter.drawText(x - 12, plotHeight + Theme::OSCILLOSCOPE_MARGIN_TOP + 20, label);
    }
}

void OscilloscopeWidget::renderToCache(){
    if ( cachedFrame_.size() != size() ){
        cachedFrame_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
        cachedFrame_.fill(Theme::OSCILLOSCOPE_BACKGROUND_COLOR);
    }

    QPainter painter(&cachedFrame_);
    painter.setRenderHint(QPainter::Antialiasing);

    drawWaveform(painter);
}

float OscilloscopeWidget::sampleToX(size_t sampleIndex, size_t totalSamples) const {
    int plotWidth = width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT ;
    float normalized = static_cast<float>(sampleIndex) / (totalSamples - 1);
    return Theme::OSCILLOSCOPE_MARGIN_LEFT + normalized * plotWidth ;
}

float OscilloscopeWidget::amplitudeToY(float amplitude) const {
    int plotHeight = height() - Theme::OSCILLOSCOPE_MARGIN_TOP - Theme::OSCILLOSCOPE_MARGIN_BOTTOM ;
    float normalized = (amplitude - minAmp_) / (maxAmp_ - minAmp_);
    return Theme::OSCILLOSCOPE_MARGIN_TOP + (1.0f - normalized) * plotHeight ;
}