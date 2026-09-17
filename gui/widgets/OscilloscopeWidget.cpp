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
#include <spdlog/spdlog.h>

OscilloscopeWidget::OscilloscopeWidget(QWidget* parent):
    QWidget(parent),
    controls_(new GraphLayerControls(this)),
    layerData_(),
    sampleRate_(Config::get<double>("audio.sample_rate").value()),
    minTime_(0.0),
    maxTime_(),
    minVolt_(*Theme::OSCILLOSCOPE_VOLTAGE_GRID.begin()),
    maxVolt_(*(Theme::OSCILLOSCOPE_VOLTAGE_GRID.end()-1)),
    updateTimer_(new QTimer(this))
{
    Config::load();

    sampleRate_ = Config::get<double>("audio.sample_rate").value();

    expectedDataSize_ = Config::get<int>("analysis.oscilloscope.window_size").value();
    sampleCache_.resize(expectedDataSize_);

    int footerY = height() - Theme::OSCILLOSCOPE_MARGIN_BOTTOM + 8 ;
    controls_->setGeometry(
        Theme::OSCILLOSCOPE_MARGIN_LEFT,
        footerY,
        width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT,
        Theme::OSCILLOSCOPE_MARGIN_BOTTOM - 12
    );

    updateTimer_->setInterval(Theme::ANALYZER_UPDATE_MS);
    connect(
        updateTimer_, &QTimer::timeout, 
        this, &OscilloscopeWidget::onUpdateTimeout
    );
    updateTimer_->start();
}

void OscilloscopeWidget::addLayer(int componentId, const QString& label){
    if ( controls_->isLayerPresent(componentId) ) return ;
    controls_->addLayer(componentId, label);
    LayerData& d = layerData_[componentId];
    d.data.resize(expectedDataSize_);
}

void OscilloscopeWidget::removeLayer(int componentId){
    if ( !controls_->isLayerPresent(componentId) ) return ; 
    controls_->removeLayer(componentId);
    layerData_.erase(componentId);
}

void OscilloscopeWidget::renameLayer(int componentId, const QString& label){
    controls_->renameLayer(componentId, label);
}

void OscilloscopeWidget::toggleLayer(int componentId, bool enabled){
    controls_->toggleLayer(componentId, enabled);
}

void OscilloscopeWidget::onData(int componentId, const double* data, size_t count){
    if ( !controls_->isLayerPresent(componentId) ) return ;

    auto& layer = layerData_.at(componentId);

    // it is not expected to get a data resize from the streaming engine
    // as this is defined in config. if we do resize, it invalidates the sampleCache. 
    if ( count != layer.data.size() ){
        SPDLOG_WARN(
            "Received different count {} than expected {} from Streaming API Client",
            count, layer.data.size()
        );
        layer.data.resize(count);
        sampleCache_.resize(count);
        sampleCacheDirty_ = true ;
    }

    layer.data.assign(data, data + count);
    layer.dirty = true ;
}

void OscilloscopeWidget::onUpdateTimeout(){
    bool anyDirty = std::any_of(
        layerData_.begin(), layerData_.end(),
        [](const auto& pair){ return pair.second.dirty ; }
    );

    if ( !anyDirty ) return ;

    static const double fadeStepFactor = std::exp(
        -3.0 * Theme::ANALYZER_UPDATE_MS / Theme::ANALYZER_FADE_DURATION_MS
    );
    static const int fadeAlpha = std::clamp(
        int((1.0 - fadeStepFactor) * 255.0), 
        0, 255
    );

    if ( !cachedFrame_.isNull() ){
        QPainter fade(&cachedFrame_);
        fade.fillRect(
            cachedFrame_.rect(), 
            QColor(0, 0, 0, fadeAlpha)
        );
    }
    
    renderToCache();
    update();
}

void OscilloscopeWidget::paintEvent(QPaintEvent* event){
    Q_UNUSED(event);

    QPainter painter(this);

    if ( !cachedFrame_.isNull() ){
        painter.drawImage(0, 0, cachedFrame_);
    }

    if ( gridCacheDirty_ || cachedGrid_.size() != size() ){
        rebuildGridCache();
        gridCacheDirty_ = false ;
    }

    if ( !cachedGrid_.isNull() ){
        painter.drawImage(0,0,cachedGrid_);
    }
}

void OscilloscopeWidget::resizeEvent(QResizeEvent* event){
    if ( !cachedFrame_.isNull() ){
        cachedFrame_ = cachedFrame_.scaled(
            event->size(), 
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        );
    }
    int footerY = height() - Theme::OSCILLOSCOPE_MARGIN_BOTTOM + 28 ;
    controls_->setGeometry(
        Theme::OSCILLOSCOPE_MARGIN_LEFT,
        footerY,
        width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT,
        Theme::OSCILLOSCOPE_MARGIN_BOTTOM - 12
    );

    sampleCacheDirty_ = true ;
    gridCacheDirty_ = true ;

    update();
}

void OscilloscopeWidget::drawGrid(QPainter& painter){
    painter.setPen(Theme::OSCILLOSCOPE_GRID_COLOR);

    int plotWidth = width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT ;
    int plotHeight = height() - Theme::OSCILLOSCOPE_MARGIN_TOP - Theme::OSCILLOSCOPE_MARGIN_BOTTOM ;

    // voltage
    painter.setPen(QPen(Theme::OSCILLOSCOPE_GRID_COLOR, 1, Qt::DotLine));
    for ( auto& pos : gridVoltageCache_ ){
        painter.drawLine(
            Theme::OSCILLOSCOPE_MARGIN_LEFT, pos,
            Theme::OSCILLOSCOPE_MARGIN_LEFT + plotWidth, pos
        );
    }

    // time
    for ( auto& pos : gridTimeCache_ ){
        painter.drawLine(
            pos.second, Theme::OSCILLOSCOPE_MARGIN_TOP,
            pos.second, Theme::OSCILLOSCOPE_MARGIN_TOP + plotHeight
        );
    }
}

void OscilloscopeWidget::drawWaveform(QPainter& painter){
    size_t numPoints = sampleCache_.size();
    if ( lineBuffer_.capacity() < static_cast<int>(numPoints) ){
        lineBuffer_.reserve(numPoints);
    }

    if ( sampleCacheDirty_ ){
        rebuildSampleCache();
        sampleCacheDirty_ = false ;
    }

    for ( auto& [id, layer] : layerData_ ){
        if ( 
            !controls_->isLayerEnabled(id) ||
            !layer.dirty 
        ) continue ;

        lineBuffer_.clear();
        for ( size_t i = 0; i < sampleCache_.size(); ++i ){
            double samplePos = sampleCache_[i];
            double y = voltageToY(layer.data[i]);

            lineBuffer_.append(QPointF(samplePos, y));
        }

        painter.setPen(QPen(Theme::OSCILLOSCOPE_GRID_COLOR, 1, Qt::SolidLine));
        painter.drawPolyline(lineBuffer_);

        layer.dirty = false ;
    }
}

void OscilloscopeWidget::drawLabels(QPainter& painter){
    painter.setPen(Theme::COMPONENT_TEXT);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    int plotHeight = height() - Theme::OSCILLOSCOPE_MARGIN_TOP - Theme::OSCILLOSCOPE_MARGIN_BOTTOM ;

    // X-axis labels (time)
    for ( const auto& [label, pos] : gridTimeCache_ ){
        painter.drawText(
            pos - 8,
            plotHeight + Theme::OSCILLOSCOPE_MARGIN_TOP + 20,
            label
        );
    }

    // Y-axis labels (voltage)
    int i = 0 ;
    for ( const double& v : Theme::OSCILLOSCOPE_VOLTAGE_GRID ){
        QString label = QString::number(v) + " v" ;
        painter.drawText(5, gridVoltageCache_[i++] + 5, label);
    }
}

void OscilloscopeWidget::renderToCache(){
    if ( cachedFrame_.size() != size() ){
        cachedFrame_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
        cachedFrame_.fill(Theme::OSCILLOSCOPE_BACKGROUND_COLOR);
    }

    QPainter painter(&cachedFrame_);
    drawWaveform(painter);
}

void OscilloscopeWidget::rebuildSampleCache(){
    size_t samples = sampleCache_.size();
    for ( size_t i = 0 ; i < samples; ++i ){
        sampleCache_[i] = sampleToX(i, samples);
    }
}

void OscilloscopeWidget::rebuildGridCache(){
    cachedGrid_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    cachedGrid_.fill(Qt::transparent);

    QPainter painter(&cachedGrid_);
    painter.setRenderHint(QPainter::Antialiasing); // text/lines look better with it

    // recompute grid data caches
    const size_t divisions = Theme::OSCILLOSCOPE_TIME_DIVISIONS  ;
    gridTimeCache_.resize(divisions + 1);

    for ( size_t i = 0 ; i <= divisions; ++i ){
        size_t sample = (sampleCache_.size() * i) / divisions ;
        double timeMs = (sample / sampleRate_) * 1000.0f ;  
        QString label = QString::number(timeMs, 'f', 1) + "ms" ;
        gridTimeCache_[i] = {label, sampleToX(sample, sampleCache_.size())};
    }
    
    auto& voltages = Theme::OSCILLOSCOPE_VOLTAGE_GRID ;
    gridVoltageCache_.resize(voltages.size());
    
    size_t i = 0 ;
    for ( const auto& v : voltages ){
        gridVoltageCache_[i++] = voltageToY(v);
    }

    // draw 
    drawGrid(painter);
    drawLabels(painter);
}

double OscilloscopeWidget::sampleToX(size_t sampleIndex, size_t total) const {
    int plotWidth = width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT ;
    double normalized = static_cast<double>(sampleIndex) / (total - 1);
    return Theme::OSCILLOSCOPE_MARGIN_LEFT + normalized * plotWidth ;
}

double OscilloscopeWidget::voltageToY(double amplitude) const {
    int plotHeight = height() - Theme::OSCILLOSCOPE_MARGIN_TOP - Theme::OSCILLOSCOPE_MARGIN_BOTTOM ;
    double normalized = (amplitude - minVolt_) / (maxVolt_ - minVolt_);
    return Theme::OSCILLOSCOPE_MARGIN_TOP + (1.0f - normalized) * plotHeight ;
}