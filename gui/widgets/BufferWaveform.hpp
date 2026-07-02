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

#ifndef BUFFER_WAVEFORM_HPP_
#define BUFFER_WAVEFORM_HPP_

#include "models/ComponentModel.hpp"
#include "widgets/CollectionWidget.hpp"

#include <QWidget>

class BufferWaveform : public CollectionWidget {
    Q_OBJECT

protected:
    float sampleRate_ ;
    size_t channel_ ;
    ComponentModel* model_ ;
    bool upstream_ ;

    int plotWidth_ ;
    float minVolt_ ;
    float maxVolt_ ;

    QImage cachedFrame_ ;

    size_t samplesPerPixel_ ;
    size_t totalNumSamples_ ;
    std::vector<std::pair<float, float>> voltages_ ;

    QTimer* resizeDebounce_ ;

public:
    explicit BufferWaveform(ComponentModel* model, size_t channel, QWidget* parent = nullptr);

    void setBufferModel(ComponentModel* model);
    
    bool isUpstream() const ;
    void setUpstream(bool upstream);

protected:
    virtual void updateCollection(const CollectionRequest& req) override ;
    
    void paintEvent(QPaintEvent* event) override ;
    void resizeEvent(QResizeEvent* event) override ;
    void showEvent(QShowEvent* event) override ;
    QSize sizeHint() const override ;

    bool hasDisplayBuffer() const ;
    const std::vector<double>& getDisplayBuffer() const ;
    void disconnectModel();
    void connectModel();

    void rebuild();
    void renderToCache();
    void drawWaveform(QPainter& painter);
    void drawGrid(QPainter& painter);

    float sampleToX(size_t sampleIndex) const ;
    int xToSample(int posX) const ;

    float voltageToY(float voltage) const ;

    QString secondsToText(int seconds) const ;

public slots:
    void onBufferDataUpdated(size_t channel);
    
};

#endif // BUFFER_WAVEFORM_HPP_