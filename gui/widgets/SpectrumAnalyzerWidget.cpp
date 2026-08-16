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


SpectrumAnalyzerWidget::SpectrumAnalyzerWidget(QWidget *parent):
    QWidget(parent),
    controls_(new GraphLayerControls(this)),
    layerData_(),
    minFreq_(Theme::SPECTRUM_MIN_FREQUENCY),
    maxFreq_(Theme::SPECTRUM_MAX_FREQUENCY),
    minDb_(Theme::SPECTRUM_MIN_DECIBEL),
    maxDb_(Theme::SPECTRUM_MAX_DECIBEL),
    updateTimer_(new QTimer(this)),
    fadeTimer_()
{
    Config::load();

    sampleRate_ = Config::get<float>("audio.sample_rate").value_or(44100);

    int footerY = height() - Theme::SPECTRUM_MARGIN_BOTTOM + 8 ;
    controls_->setGeometry(
        Theme::SPECTRUM_MARGIN_LEFT, 
        footerY,
        width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT,
        Theme::SPECTRUM_MARGIN_BOTTOM - 12 
    );

    updateTimer_->setInterval(33); // ~30 FPS
    connect(
        updateTimer_, &QTimer::timeout, 
        this, &SpectrumAnalyzerWidget::onUpdateTimeout
    );
    updateTimer_->start();
    fadeTimer_.start();
}

void SpectrumAnalyzerWidget::setFrequencyRange(float minHz, float maxHz) {
    minFreq_ = minHz ;
    maxFreq_ = maxHz ;
    update();
}

void SpectrumAnalyzerWidget::setMagnitudeRange(float minDb, float maxDb) {
    minDb_ = minDb ;
    maxDb_ = maxDb ;
    update();
}

void SpectrumAnalyzerWidget::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate ;
    update();
}

void SpectrumAnalyzerWidget::addLayer(int componentId, const QString& label){
    if ( controls_->isLayerPresent(componentId) ) return ;
    controls_->addLayer(componentId, label);
    layerData_[componentId];
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

void SpectrumAnalyzerWidget::onData(int componentId, const float* data, size_t count){
    if ( !controls_->isLayerPresent(componentId) ) return ;

    auto& layer = layerData_.at(componentId);

    layer.data.resize(count);
    layer.data.assign(data, data + count);

    layer.lastUpdate.restart();
}

void SpectrumAnalyzerWidget::onUpdateTimeout() {
    if ( !cachedFrame_.isNull() ){
        // fade out analysis signal
        auto elapsedMs = fadeTimer_.restart();
        double decay = 1.0 - std::exp(-double(elapsedMs) / Theme::ANALYZER_FADE_DURATION_MS);
        int alpha = std::clamp(int(decay * 255.0), 0, 255);

        QPainter fade(&cachedFrame_);
        fade.fillRect(
            cachedFrame_.rect(), 
            QColor(0, 0, 0, alpha)
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
    drawGrid(painter);
    drawLabels(painter);
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
    update();
    
}

void SpectrumAnalyzerWidget::drawGrid(QPainter &painter) {
    painter.setPen(Theme::SPECTRUM_GRID_COLOR);
    
    int plotWidth = width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT ;
    int plotHeight = height() - Theme::SPECTRUM_MARGIN_TOP - Theme::SPECTRUM_MARGIN_BOTTOM ;
    
    // Horizontal grid lines (dB)
    for (float db = minDb_; db <= maxDb_; db += 10.0) {
        int y = static_cast<int>(dbToY(db));
        painter.drawLine(Theme::SPECTRUM_MARGIN_LEFT, y, Theme::SPECTRUM_MARGIN_LEFT + plotWidth, y);
    }
    
    // Vertical grid lines (frequency, logarithmic)
    std::vector<float> freqs = {20, 50, 100, 2.0, 500, 1000, 2000, 5000, 10000, 20000};
    for (float freq : freqs) {
        if (freq >= minFreq_ && freq <= maxFreq_) {
            int x = static_cast<int>(freqToX(freq));
            painter.drawLine(x, Theme::SPECTRUM_MARGIN_TOP, x, Theme::SPECTRUM_MARGIN_TOP + plotHeight);
        }
    }
}

void SpectrumAnalyzerWidget::drawSpectrum(QPainter &painter) {
    for ( auto& [id, layer] : layerData_ ){
        if ( 
            layer.data.empty() || 
            !controls_->isLayerEnabled(id) ||
            layer.lastUpdate.elapsed() > Theme::ANALYZER_STALE_DATA_DURATION_MS
        ) continue ;

        QPainterPath path ;
        bool firstPoint = true ;
        
        int plotWidth = width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT ;

        int sampleInterval = Theme::SPECTRUM_PIXEL_RESOLUTION ;
        for ( int px = 0 ; px < plotWidth; px += sampleInterval ){
            float xLeft  = Theme::SPECTRUM_MARGIN_LEFT + px ;
            float xRight = Theme::SPECTRUM_MARGIN_LEFT + px + sampleInterval ;

            float freqLeft  = xToFreq(xLeft);
            float freqRight = xToFreq(xRight);

            if ( freqRight < minFreq_ || freqLeft > maxFreq_ ) continue ;

            size_t binLeft  = freqToBin(freqLeft,  layer.data.size());
            size_t binRight = freqToBin(freqRight, layer.data.size());

            binLeft  = std::min(binLeft,  layer.data.size() - 1);
            binRight = std::min(binRight, layer.data.size() - 1);
            if ( binRight < binLeft ) binRight = binLeft ;

            float peakDb = layer.data[binLeft];
            for ( size_t bin = binLeft + 1 ; bin <= binRight ; ++bin ){
                peakDb = std::max(peakDb, layer.data[bin]);
            }

            float db = std::max(minDb_, std::min(maxDb_, peakDb));
            float x = xLeft ;
            float y = dbToY(db);

            if ( firstPoint ){
                path.moveTo(x,y);
                firstPoint = false ;
            } else {
                path.lineTo(x,y);
            }
        }
        
        painter.setPen(QPen(controls_->layerColor(id), 2));
        painter.drawPath(path);
    }
}

void SpectrumAnalyzerWidget::drawLabels(QPainter &painter) {
    painter.setPen(Theme::COMPONENT_TEXT);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    
    int plotWidth = width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT ;
    int plotHeight = height() - Theme::SPECTRUM_MARGIN_TOP - Theme::SPECTRUM_MARGIN_BOTTOM ;
    
    // Y-axis labels (dB)
    for (float db = minDb_; db <= maxDb_; db += 20.0) {
        int y = static_cast<int>(dbToY(db));
        QString label = QString::number(static_cast<int>(db)) + " dB";
        painter.drawText(5, y + 5, label);
    }
    
    // X-axis labels (frequency)
    std::vector<std::pair<float, QString>> freqLabels = {
        {20, "20Hz"},
        {50, "50"},
        {100, "100"},
        {200, "200"},
        {500, "500"},
        {1000, "1kHz"},
        {2000, "2k"},
        {5000, "5k"},
        {10000, "10k"},
        {20000, "20k"}
    };
    
    for (const auto& [freq, label] : freqLabels) {
        if (freq >= minFreq_ && freq <= maxFreq_) {
            int x = static_cast<int>(freqToX(freq));
            painter.drawText(x - 8,  plotHeight + Theme::SPECTRUM_MARGIN_TOP + 20, label);
        }
    }
}

void SpectrumAnalyzerWidget::renderToCache() {
    if ( cachedFrame_.size() != size() ){
        cachedFrame_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
        cachedFrame_.fill(Theme::SPECTRUM_BACKGROUND_COLOR);
    }

    QPainter painter(&cachedFrame_);
    painter.setRenderHint(QPainter::Antialiasing);

    drawSpectrum(painter);
}

float SpectrumAnalyzerWidget::freqToX(float freq) const {
    int plotWidth = width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT ;
    
    // Logarithmic mapping
    float logMin = std::log10(minFreq_);
    float logMax = std::log10(maxFreq_);
    float logFreq = std::log10(freq);
    
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return Theme::SPECTRUM_MARGIN_LEFT + normalized * plotWidth;
}

float SpectrumAnalyzerWidget::xToFreq(float x) const {    
    int plotWidth = width() - Theme::SPECTRUM_MARGIN_LEFT - Theme::SPECTRUM_MARGIN_RIGHT ;
    float normalized = (x - Theme::SPECTRUM_MARGIN_LEFT) / plotWidth ;

    float logMin = std::log10(minFreq_);
    float logMax = std::log10(maxFreq_);
    float logFreq = logMin + normalized * (logMax - logMin);

    return std::pow(10.0, logFreq);
}

float SpectrumAnalyzerWidget::dbToY(float db) const {
    int plotHeight = height() - Theme::SPECTRUM_MARGIN_TOP - Theme::SPECTRUM_MARGIN_BOTTOM ;
    
    // Linear mapping (inverted - lower dB = higher on screen)
    float normalized = (db - minDb_) / (maxDb_ - minDb_) ;
    return Theme::SPECTRUM_MARGIN_TOP + (1.0 - normalized) * plotHeight ;
}

float SpectrumAnalyzerWidget::binToFreq(size_t bin, size_t count) const {
    return (bin * sampleRate_) / ( count * 2) ;
}

size_t SpectrumAnalyzerWidget::freqToBin(float freq, size_t count) const {
    return static_cast<size_t>((freq * count * 2) / sampleRate_ );
}