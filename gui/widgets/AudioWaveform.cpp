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

#include "widgets/AudioWaveform.hpp"
#include "config/Config.hpp"
#include "app/Theme.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QTimer>

AudioWaveformWidget::AudioWaveformWidget(ComponentModel* model, size_t channel, QWidget* parent):
    QWidget(parent),
    model_(nullptr),
    upstream_(false),
    minVolt_(-1.0),
    maxVolt_(1.0),
    channel_(channel),
    voltages_(),
    resizeDebounce_(new QTimer(this))
{
    Config::load();
    sampleRate_ = Config::get<float>("audio.sample_rate").value();

    setBufferModel(model);

    resizeDebounce_->setSingleShot(true);
    connect(
        resizeDebounce_, &QTimer::timeout,
        this, &AudioWaveformWidget::rebuild
    );
}

void AudioWaveformWidget::setBufferModel(ComponentModel* model){
    disconnectModel();
    model_ = model ;
    connectModel();
    rebuild();
}

bool AudioWaveformWidget::isUpstream() const {
    return upstream_ ;
}

void AudioWaveformWidget::setUpstream(bool upstream){
    if ( upstream_ == upstream ) return ;
    disconnectModel();
    upstream_ = upstream ;
    connectModel();
    rebuild();
}

void AudioWaveformWidget::paintEvent(QPaintEvent* event){
    Q_UNUSED(event)
    QPainter painter(this);
    if ( !cachedFrame_.isNull() ){
        painter.drawImage(0,0, cachedFrame_);
    }
}

void AudioWaveformWidget::resizeEvent(QResizeEvent* event){
    if ( !cachedFrame_.isNull() ){
        cachedFrame_ = cachedFrame_.scaled(
            event->size(),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        );
    }
    update();
    resizeDebounce_->start(150);
}

void AudioWaveformWidget::showEvent(QShowEvent* event){
    QWidget::showEvent(event);
    rebuild();
}

bool AudioWaveformWidget::hasDisplayBuffer() const {
    if ( !model_ ){
        return false ;
    }

    if ( upstream_ ){
        return model_->hasUpstreamBuffer(channel_);
    } else {
        return model_->hasBuffer(channel_);
    }
}

const std::vector<double>& AudioWaveformWidget::getDisplayBuffer() const {
    if ( upstream_ ){
        return model_->getUpstreamBuffer(channel_);
    } 

    return model_->getBuffer(channel_);
}

void AudioWaveformWidget::disconnectModel(){
    if ( model_ ){
        disconnect(
            model_,
            upstream_ ? &ComponentModel::upstreamBufferUpdated
                : &ComponentModel::bufferUpdated, 
            this, &AudioWaveformWidget::onBufferDataUpdated
        );
    }
}
void AudioWaveformWidget::connectModel(){
    if ( model_ ){
        connect(
            model_,
            upstream_ ? &ComponentModel::upstreamBufferUpdated 
                : &ComponentModel::bufferUpdated,
            this, &AudioWaveformWidget::onBufferDataUpdated
        );
    }
}

void AudioWaveformWidget::rebuild(){
    if ( !hasDisplayBuffer() ) return ;
    const auto& buf = getDisplayBuffer();
    
    int plotWidth = width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT ;
    if ( plotWidth <= 0 || buf.empty() ) return ;

    samplesPerPixel_ = std::max<size_t>(1, buf.size() / static_cast<size_t>(plotWidth));
     
    // populate voltages
    voltages_.clear();
    for ( size_t i = 0 ; i < buf.size(); i += samplesPerPixel_ ){
        size_t end = std::min(i + samplesPerPixel_, buf.size());

        auto itMin = std::min_element(buf.begin() + i, buf.begin() + end);
        auto itMax = std::max_element(buf.begin() + i, buf.begin() + end);

        voltages_.push_back({*itMin, *itMax});
    }

    renderToCache();
}

void AudioWaveformWidget::renderToCache(){
    if ( size().isEmpty() ) return ;

    cachedFrame_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    cachedFrame_.fill(Theme::OSCILLOSCOPE_BACKGROUND_COLOR);

    QPainter painter(&cachedFrame_);
    painter.setRenderHint(QPainter::Antialiasing);

    drawWaveform(painter);
    update();
}

void AudioWaveformWidget::drawWaveform(QPainter& painter){
    double midY = height() 
        - Theme::OSCILLOSCOPE_MARGIN_BOTTOM
        - Theme::OSCILLOSCOPE_MARGIN_TOP ;
    double startX = Theme::OSCILLOSCOPE_MARGIN_LEFT ;

    QPainterPath path ;
    const size_t n = voltages_.size();
    if ( n < 2 ) return ; // 0 case, also can't draw with only one bucket

    // start with max values left to right
    float x0 = sampleToX(0, n);
    float yTop0 = voltageToY(voltages_[0].second);
    path.moveTo(x0, yTop0);

    for ( size_t i = 1; i < n; ++i ){
        float x = sampleToX(i, n);
        float yTop = voltageToY(voltages_[i].second);
        path.lineTo(x,yTop);
    }

    // now min values right to left
    for ( size_t i = n - 1; i > 0; --i ){
        float x = sampleToX(i,n);
        float yBot = voltageToY(voltages_[i].first);
        path.lineTo(x, yBot);
    }

    path.closeSubpath();
    painter.fillPath(path, Theme::SOCKET_BUFFER);
}

float AudioWaveformWidget::sampleToX(size_t sampleIndex, size_t totalSamples) const {
    int plotWidth = width() - Theme::OSCILLOSCOPE_MARGIN_LEFT - Theme::OSCILLOSCOPE_MARGIN_RIGHT ;
    float normalized = static_cast<float>(sampleIndex) / (totalSamples - 1);
    return Theme::OSCILLOSCOPE_MARGIN_LEFT + normalized * plotWidth ;
}

float AudioWaveformWidget::voltageToY(float voltage) const {
    int plotHeight = height() - Theme::OSCILLOSCOPE_MARGIN_TOP - Theme::OSCILLOSCOPE_MARGIN_BOTTOM ;
    float normalized = (voltage - minVolt_ ) / (maxVolt_ - minVolt_);
    return Theme::OSCILLOSCOPE_MARGIN_TOP + (1.0f - normalized) * plotHeight ;
}

void AudioWaveformWidget::onBufferDataUpdated(size_t channel){
    qDebug() << "audio buffer updated. channel:" << channel << ". Internal channel " << channel_ ;
    if ( channel != channel_ ) return ;
    rebuild();
}