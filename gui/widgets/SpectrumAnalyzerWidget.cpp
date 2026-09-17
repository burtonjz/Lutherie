/*
 * Copyright (C) 2025 Jared Burton
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

#include "widgets/SpectrumAnalyzerWidget.hpp"
#include "app/Theme.hpp"
#include "config/Config.hpp"
#include <QPaintEvent>
#include <QPainterPath>
#include <cmath>
#include <algorithm>
#include <spdlog/spdlog.h>


SpectrumAnalyzerWidget::SpectrumAnalyzerWidget(QWidget *parent):
    QWidget(parent),
    controls_(new GraphLayerControls(this)),
    gridFreqCache_(),
    layerData_(),
    minFreq_(*Theme::SPECTRUM_FREQUENCY_GRID.begin()),
    maxFreq_(*(Theme::SPECTRUM_FREQUENCY_GRID.end()-1)),
    minDb_(*Theme::SPECTRUM_DECIBEL_GRID.begin()),
    maxDb_(*(Theme::SPECTRUM_DECIBEL_GRID.end()-1)),
    updateTimer_(new QTimer(this))
{
    Config::load();

    sampleRate_ = Config::get<double>(
        "audio.sample_rate").value_or(44100);

    expectedDataSize_ = Config::get<unsigned int>(
        "analysis.spectrum_analyzer.buffer_size").value() / 2 ;
    
    binCache_.right.resize(expectedDataSize_);

    int footerY = height() - Theme::SPECTRUM_MARGIN_BOTTOM + 8 ;
    controls_->setGeometry(
        Theme::SPECTRUM_MARGIN_LEFT, 
        footerY,
        width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT,
        Theme::SPECTRUM_MARGIN_BOTTOM - 12 
    );

    updateTimer_->setInterval(Theme::ANALYZER_UPDATE_MS);
    connect(
        updateTimer_, &QTimer::timeout, 
        this, &SpectrumAnalyzerWidget::onUpdateTimeout
    );
    updateTimer_->start();
}

void SpectrumAnalyzerWidget::addLayer(int componentId, const QString& label){
    if ( controls_->isLayerPresent(componentId) ) return ;
    controls_->addLayer(componentId, label);
    LayerData& d = layerData_[componentId];
    d.data.resize(expectedDataSize_);
}

void SpectrumAnalyzerWidget::removeLayer(int componentId){
    if ( !controls_->isLayerPresent(componentId) ) return ;
    controls_->removeLayer(componentId);
    layerData_.erase(componentId);
}

void SpectrumAnalyzerWidget::renameLayer(int componentId, const QString& label){
    controls_->renameLayer(componentId, label);
}

void SpectrumAnalyzerWidget::toggleLayer(int componentId, bool enabled){
    controls_->toggleLayer(componentId, enabled);
}

void SpectrumAnalyzerWidget::onData(int componentId, const double* data, size_t count){
    // data received are magnitudes only
    if ( !controls_->isLayerPresent(componentId) ) return ;

    auto& layer = layerData_.at(componentId);

    // it is not expected to get a data resize from the streaming engine
    // as this is defined in config. if we do resize, it invalidates the binCache. 
    if ( count != layer.data.size() ){
        SPDLOG_WARN(
            "Received different count {} than expected {} from Streaming API Client",
            count, layer.data.size()
        );
        layer.data.resize(count);
        binCache_.right.resize(count);
        binCacheDirty_ = true ;
    }

    layer.data.assign(data, data + count);
    layer.dirty = true ;
}

void SpectrumAnalyzerWidget::onUpdateTimeout(){
    bool anyDirty = std::any_of(
        layerData_.begin(), layerData_.end(),
        [](const auto& pair){ return pair.second.dirty ; }
    );

    if ( !anyDirty ) return ;

    static const double fadeStepFactor = std::exp(
        -2.0 * Theme::ANALYZER_UPDATE_MS / Theme::ANALYZER_FADE_DURATION_MS
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

    renderToCache() ;
    update(); 
}

void SpectrumAnalyzerWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);

    if ( !cachedFrame_.isNull() ){
        painter.drawImage(0,0, cachedFrame_);
    }

    if ( gridCacheDirty_ || cachedGrid_.size() != size() ){
        rebuildGridCache();
        gridCacheDirty_ = false ;
    }

    if ( !cachedGrid_.isNull() ){
        painter.drawImage(0,0, cachedGrid_);
    }
}

void SpectrumAnalyzerWidget::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    if ( !cachedFrame_.isNull() ){
        cachedFrame_ = cachedFrame_.scaled(
            event->size(), 
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        );
    }
    int footerY = height() - Theme::SPECTRUM_MARGIN_BOTTOM + 28 ;
    controls_->setGeometry(
        Theme::SPECTRUM_MARGIN_LEFT, 
        footerY,
        width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT,
        Theme::SPECTRUM_MARGIN_BOTTOM - 12 
    );

    binCacheDirty_ = true ;
    gridCacheDirty_ = true ;

    update();
}

void SpectrumAnalyzerWidget::drawGrid(QPainter &painter) {
    painter.setPen(Theme::SPECTRUM_GRID_COLOR);
    
    int plotWidth = width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT ;
    int plotHeight = height() - Theme::SPECTRUM_MARGIN_TOP - Theme::SPECTRUM_MARGIN_BOTTOM ;

    // horizontal db grid lines
    for ( auto& pos : gridDbCache_ ){
        painter.drawLine(
            Theme::SPECTRUM_MARGIN_LEFT, pos,
            Theme::SPECTRUM_MARGIN_LEFT + plotWidth, pos
        );
    }

    // vertical db grid lines
    for ( auto& pos : gridFreqCache_ ){
        painter.drawLine(
            pos, Theme::SPECTRUM_MARGIN_TOP,
            pos, Theme::SPECTRUM_MARGIN_TOP + plotHeight
        );
    }
}

void SpectrumAnalyzerWidget::drawSpectrum(QPainter &painter) {
    size_t numPoints = binCache_.right.size() * 2 ;
    if ( lineBuffer_.capacity() < static_cast<int>(numPoints) ){
        lineBuffer_.reserve(numPoints);
    }

    if ( binCacheDirty_ ){
        rebuildBinCache();
        binCacheDirty_ = false ;
    }

    for ( auto& [id, layer] : layerData_ ){
        if ( 
            !controls_->isLayerEnabled(id) ||
            !layer.dirty 
        ) continue ;

        lineBuffer_.clear();
        
        // bins shouldn't be explicitly draw unless 
        // we can actually draw at a reasonable resolution
        double minPixelDist = 2.0 / this->devicePixelRatioF();

        double pendingLeft = binCache_.start ;
        double pendingY = dbToY(0.0) ;
        for ( size_t i = 0 ; i < binCache_.right.size() ; ++i ){ 
            double right = binCache_.right[i] ;
            double y = std::max(dbToY(layer.data[i]), pendingY);

            if ( right - pendingLeft < minPixelDist ){
                // not long enough for a line
                // keep track of peak and move to next bin
                pendingY = y ;
                continue ; 
            }

            lineBuffer_.append(QPointF(pendingLeft, y));
            lineBuffer_.append(QPointF(right, y));

            // reset for future runs
            pendingLeft = right ;
            pendingY = 0.0 ;
        }
        
        painter.setPen(QPen(controls_->layerColor(id), 2));
        painter.drawPolyline(lineBuffer_);

        layer.dirty = false ;
    }
}

void SpectrumAnalyzerWidget::drawLabels(QPainter &painter) {
    painter.setPen(Theme::COMPONENT_TEXT);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    
    int plotHeight = height() - Theme::SPECTRUM_MARGIN_TOP - Theme::SPECTRUM_MARGIN_BOTTOM ;
    
    // X-axis labels (frequency)
    int i = 0 ;
    for ( const auto& label : Theme::SPECTRUM_FREQUENCY_LABELS ){
        painter.drawText(
            gridFreqCache_[i++] - 8,  
            plotHeight + Theme::SPECTRUM_MARGIN_TOP + 20, 
            label
        );
    }

    // Y-axis labels (dB)
    i = 0 ;
    for ( const double& db : Theme::SPECTRUM_DECIBEL_GRID ){
        QString label = QString::number(static_cast<int>(db)) + " dB";
        painter.drawText(5, gridDbCache_[i++] + 5, label);
    }
}

void SpectrumAnalyzerWidget::renderToCache() {
    if ( cachedFrame_.size() != size() ){
        cachedFrame_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
        cachedFrame_.fill(Theme::SPECTRUM_BACKGROUND_COLOR);
    }

    QPainter painter(&cachedFrame_);
    drawSpectrum(painter);
}

void SpectrumAnalyzerWidget::rebuildBinCache(){
    // recompute bin frequency positionings when bin is invalidated
    const double binCenterFactor = sampleRate_ / ( binCache_.right.size() * 2 );    
    const double halfBinWidth = binCenterFactor / 2.0 ;
    
    // center frequency of first bin is 0, but we're gonna push it
    // to minFreq. first bin probably passes minFreq but just to 
    // be safe let's clamp the right side as well.
    binCache_.start = freqToX(minFreq_) ; 
    binCache_.right[0] = halfBinWidth > minFreq_ ? freqToX(halfBinWidth) : binCache_.start ;
    bool hitMax = false ;
    for ( size_t i = 1 ; i < binCache_.right.size(); ++i ){
        // if bins hit max, just keep copying that value instead of recomputing
        if ( hitMax ){
            binCache_.right[i] = binCache_.right[i-1];
            continue ;
        } 

        double rightFreq = binCenterFactor * i + halfBinWidth ;

        // safety clamp right
        if ( rightFreq > maxFreq_ ){
            rightFreq = maxFreq_ ;
            hitMax = true ;
        } 
        binCache_.right[i] = freqToX(rightFreq);
    }
}

void SpectrumAnalyzerWidget::rebuildGridCache(){
    cachedGrid_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    cachedGrid_.fill(Qt::transparent);

    QPainter painter(&cachedGrid_);
    painter.setRenderHint(QPainter::Antialiasing); // text/lines look better with it

    // recompute grid data caches
    auto& freq = Theme::SPECTRUM_FREQUENCY_GRID ;
    
    gridFreqCache_.resize(freq.size());
    size_t i = 0 ;
    for ( const auto& freq : freq ){
        gridFreqCache_[i++] = freqToX(freq);
    }

    gridDbCache_.clear();
    for ( const auto& db : Theme::SPECTRUM_DECIBEL_GRID ){
        gridDbCache_.push_back(dbToY(db));
    }

    drawGrid(painter);
    drawLabels(painter);
}

double SpectrumAnalyzerWidget::freqToX(double freq) const {
    int plotWidth = width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT ;
    
    // Logarithmic mapping
    double logMin = std::log10(minFreq_);
    double logMax = std::log10(maxFreq_);
    double logFreq = std::log10(freq);
    
    double normalized = (logFreq - logMin) / (logMax - logMin);
    return Theme::SPECTRUM_MARGIN_LEFT + normalized * plotWidth;
}

double SpectrumAnalyzerWidget::dbToY(double db) const {
    int plotHeight = height() - Theme::SPECTRUM_MARGIN_TOP - Theme::SPECTRUM_MARGIN_BOTTOM ;
    db = std::clamp(db, minDb_, maxDb_);
    
    // Linear mapping (inverted - lower dB = higher on screen)
    double normalized = (db - minDb_) / (maxDb_ - minDb_) ;
    return Theme::SPECTRUM_MARGIN_TOP + (1.0 - normalized) * plotHeight ;
}