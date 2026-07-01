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

#include "widgets/BufferWaveform.hpp"
#include "config/Config.hpp"
#include "app/Theme.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QTimer>
#include <QTime>

BufferWaveform::BufferWaveform(ComponentModel* model, size_t channel, QWidget* parent):
    QWidget(parent),
    model_(nullptr),
    upstream_(false),
    minVolt_(-1.0),
    maxVolt_(1.0),
    plotWidth_(Theme::WAVEFORM_MIN_PLOT_WIDTH),
    channel_(channel),
    samplesPerPixel_(1),
    totalNumSamples_(0),
    voltages_(),
    resizeDebounce_(new QTimer(this))
{
    Config::load();
    sampleRate_ = Config::get<float>("audio.sample_rate").value();

    setBufferModel(model);

    resizeDebounce_->setSingleShot(true);
    connect(
        resizeDebounce_, &QTimer::timeout,
        this, &BufferWaveform::rebuild
    );
    
    resize(
        Theme::WAVEFORM_MIN_WIDTH,
        Theme::WAVEFORM_HEIGHT
    );
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

void BufferWaveform::setBufferModel(ComponentModel* model){
    disconnectModel();
    model_ = model ;
    connectModel();
    rebuild();
}

bool BufferWaveform::isUpstream() const {
    return upstream_ ;
}

void BufferWaveform::setUpstream(bool upstream){
    if ( upstream_ == upstream ) return ;
    disconnectModel();
    upstream_ = upstream ;
    connectModel();
    rebuild();
}

void BufferWaveform::paintEvent(QPaintEvent* event){
    Q_UNUSED(event)
    QPainter painter(this);
    if ( !cachedFrame_.isNull() ){
        painter.drawImage(0,0, cachedFrame_);
    }
}

void BufferWaveform::resizeEvent(QResizeEvent* event){
    QWidget::resizeEvent(event);

    if ( !cachedFrame_.isNull() ){
        cachedFrame_ = cachedFrame_.scaled(
            event->size(),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        );
    }

    plotWidth_ = width() 
        - Theme::WAVEFORM_MARGIN_LEFT
        - Theme::WAVEFORM_MARGIN_RIGHT ;
    update();
    resizeDebounce_->start(50);
}

void BufferWaveform::showEvent(QShowEvent* event){
    QWidget::showEvent(event);
    rebuild();
}

QSize BufferWaveform::sizeHint() const {
    return { 
        Theme::WAVEFORM_MIN_PLOT_WIDTH,
        Theme::WAVEFORM_HEIGHT
    };
}

bool BufferWaveform::hasDisplayBuffer() const {
    if ( !model_ ){
        return false ;
    }

    if ( upstream_ ){
        return model_->hasUpstreamBuffer(channel_);
    } else {
        return model_->hasBuffer(channel_);
    }
}

const std::vector<double>& BufferWaveform::getDisplayBuffer() const {
    if ( upstream_ ){
        return model_->getUpstreamBuffer(channel_);
    } 

    return model_->getBuffer(channel_);
}

void BufferWaveform::disconnectModel(){
    if ( model_ ){
        disconnect(
            model_,
            upstream_ ? &ComponentModel::upstreamBufferUpdated
                : &ComponentModel::bufferUpdated, 
            this, &BufferWaveform::onBufferDataUpdated
        );
    }
}
void BufferWaveform::connectModel(){
    if ( model_ ){
        connect(
            model_,
            upstream_ ? &ComponentModel::upstreamBufferUpdated 
                : &ComponentModel::bufferUpdated,
            this, &BufferWaveform::onBufferDataUpdated
        );
    }
}

void BufferWaveform::rebuild(){
    if ( !hasDisplayBuffer() ){
        renderToCache();
        return ;
    }

    const auto& buf = getDisplayBuffer();
    
    int plotWidth = width() - Theme::WAVEFORM_MARGIN_LEFT - Theme::WAVEFORM_MARGIN_RIGHT ;
    if ( plotWidth <= 0 ) return ;
    
    totalNumSamples_ = buf.size();
    if ( totalNumSamples_ == 0 ) return ;

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

void BufferWaveform::renderToCache(){
    if ( size().isEmpty() ) return ;

    cachedFrame_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    cachedFrame_.fill(Theme::WAVEFORM_BACKGROUND_COLOR);

    QPainter painter(&cachedFrame_);
    painter.setRenderHint(QPainter::Antialiasing);

    drawWaveform(painter);
    drawGrid(painter);
    update();
}

void BufferWaveform::drawWaveform(QPainter& painter){
    double midY = height() 
        - Theme::WAVEFORM_MARGIN_BOTTOM
        - Theme::WAVEFORM_MARGIN_TOP ;
    double startX = Theme::WAVEFORM_MARGIN_LEFT ;

    QPainterPath path ;
    const size_t n = voltages_.size();
    if ( n < 2 ) return ; // 0 case, also can't draw with only one bucket

    // start with max values left to right
    float x0 = sampleToX(0);
    float yTop0 = voltageToY(voltages_[0].second);
    path.moveTo(x0, yTop0);

    for ( size_t i = 1; i < n; ++i ){
        float x = sampleToX(i * samplesPerPixel_);
        float yTop = voltageToY(voltages_[i].second);
        path.lineTo(x,yTop);
    }

    // now min values right to left
    for ( size_t i = n - 1; i > 0; --i ){
        float x = sampleToX(i * samplesPerPixel_);
        float yBot = voltageToY(voltages_[i].first);
        path.lineTo(x, yBot);
    }

    path.closeSubpath();
    painter.fillPath(path, Theme::WAVEFORM_WAVE_COLOR);
}

void BufferWaveform::drawGrid(QPainter& painter){
    int plotWidth = width() - Theme::WAVEFORM_MARGIN_LEFT - Theme::WAVEFORM_MARGIN_RIGHT ;
    int plotHeight = height() - Theme::WAVEFORM_MARGIN_TOP - Theme::WAVEFORM_MARGIN_BOTTOM ;

    QFont textFont = font() ;
    textFont.setPixelSize(Theme::WAVEFORM_GRID_FONT_SIZE);
    painter.setFont(textFont);
    int fontYAdjust = Theme::WAVEFORM_GRID_FONT_SIZE / 2 ;

    for ( float volt : {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f} ){
        if ( volt == floor(volt) ){ 
            // whole number, solid line
            painter.setPen(QPen(Theme::WAVEFORM_GRID_COLOR, 1));
        } else { 
            // fraction, dotted line
            painter.setPen(QPen(Theme::WAVEFORM_GRID_COLOR, 1, Qt::DotLine));
        }

        int y = voltageToY(volt);
        // -0.5 to account for line width
        painter.drawLine( 
            QPointF(Theme::WAVEFORM_MARGIN_LEFT, y - 0.5),
            QPointF(Theme::WAVEFORM_MARGIN_LEFT + plotWidth, y - 0.5)
        );
        painter.drawText(5, y + fontYAdjust, 
            QString("%1 V").arg(volt, 0, 'f', 1)
        );
    }

    // time divisions
    if ( totalNumSamples_ == 0 ) return ;

    const double totalTimeSeconds = totalNumSamples_ / sampleRate_ ;
    const int maxNumSteps = plotWidth_ / Theme::WAVEFORM_TIME_WIDTH ;
    const int maxTimeStep = totalTimeSeconds / maxNumSteps ;
    
    // get best time step
    int timeStep = 0 ;

    for ( const auto& delta : Theme::WAVEFORM_DELTA_TIMES ){
        if ( delta < maxTimeStep ){
            timeStep = delta ;
        } else {
            break ;
        }
    }

    if ( timeStep == 0 ) timeStep = 1 ;

    for ( int time = 0; time < totalTimeSeconds; ){
        int sample = sampleRate_ * time ;
        int x = sampleToX(sample);
        painter.drawText(x, 5 + fontYAdjust, 
            secondsToText(time)
        );
        
        time += timeStep ; 
    }
    
}

float BufferWaveform::sampleToX(size_t sampleIndex) const {
    if ( totalNumSamples_ == 0 ) return 0 ;
    int plotWidth = width() - Theme::WAVEFORM_MARGIN_LEFT - Theme::WAVEFORM_MARGIN_RIGHT ;
    float normalized = static_cast<float>(sampleIndex) / (totalNumSamples_ - 1);
    return Theme::WAVEFORM_MARGIN_LEFT + normalized * plotWidth ;
}

int BufferWaveform::xToSample(int posX) const {
    if ( totalNumSamples_ == 0 ) return 0 ;
    int plotWidth = width() - Theme::WAVEFORM_MARGIN_LEFT - Theme::WAVEFORM_MARGIN_RIGHT ;
    float normalized = (static_cast<float>(posX) - Theme::WAVEFORM_MARGIN_LEFT) / plotWidth ; 
    if ( normalized > 1.0f ){
        qWarning() << "normalized x value is " << normalized << "which is out of range. setting to max";
        normalized = 1.0f ;
    }
    return normalized * (totalNumSamples_ - 1);
}

float BufferWaveform::voltageToY(float voltage) const {
    int plotHeight = height() - Theme::WAVEFORM_MARGIN_TOP - Theme::WAVEFORM_MARGIN_BOTTOM ;
    float normalized = (voltage - minVolt_ ) / (maxVolt_ - minVolt_);
    return Theme::WAVEFORM_MARGIN_TOP + (1.0f - normalized) * plotHeight ;
}

QString BufferWaveform::secondsToText(int seconds) const {
    if ( seconds < 60 ){
        return QString::number(seconds) + "s" ;
    }

    int minutes = seconds / 60 ;
    seconds = seconds % 60 ;

    if ( minutes < 60 ){
        return QString::number(minutes) + ":" 
            + QString("%1").arg(seconds, 2, 10, '0');
    }

    int hours = minutes / 60 ;
    minutes = minutes % 60 ;

    return QString::number(hours) + ":"
        + QString("%1:").arg(minutes, 2, 10, '0')
        + QString("%1").arg(seconds, 2, 10, '0');
}

void BufferWaveform::onBufferDataUpdated(size_t channel){
    qDebug() << "audio buffer updated. channel:" << channel << ". Internal channel " << channel_ ;
    if ( channel != channel_ ) return ;
    rebuild();
}