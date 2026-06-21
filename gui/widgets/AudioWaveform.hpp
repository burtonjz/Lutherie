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

#ifndef AUDIO_WAVEFORM_WIDGET_HPP_
#define AUDIO_WAVEFORM_WIDGET_HPP_

#include "models/ComponentModel.hpp"

#include <QWidget>

class AudioWaveformWidget : public QWidget {
    Q_OBJECT

private:
    float sampleRate_ ;
    size_t channel_ ;
    ComponentModel* model_ ;
    bool upstream_ ;

    float minVolt_ ;
    float maxVolt_ ;

    QImage cachedFrame_ ;
    size_t samplesPerPixel_ ;
    std::vector<std::pair<float, float>> voltages_ ;

    QTimer* resizeDebounce_ ;

    

public:
    explicit AudioWaveformWidget(ComponentModel* model, size_t channel, QWidget* parent = nullptr);

    void setBufferModel(ComponentModel* model);
    
    bool isUpstream() const ;
    void setUpstream(bool upstream);

protected:
    void paintEvent(QPaintEvent* event) override ;
    void resizeEvent(QResizeEvent* event) override ;
    void showEvent(QShowEvent* event) override ;

private:
    bool hasDisplayBuffer() const ;
    const std::vector<double>& getDisplayBuffer() const ;
    void disconnectModel();
    void connectModel();

    void rebuild();
    void renderToCache();
    void drawWaveform(QPainter& Painter);

    float sampleToX(size_t sampleIndex, size_t totalSamples) const ;
    float voltageToY(float voltage) const ;

public slots:
    void onBufferDataUpdated(size_t channel);
    
};

#endif // AUDIO_WAVEFORM_WIDGET_HPP_