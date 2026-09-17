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

#include "interfaces/IAnalyzerWidget.hpp"
#include "widgets/GraphLayerControls.hpp"

#include <QWidget>
#include <QTimer>

class OscilloscopeWidget : public QWidget, public IAnalyzerWidget {
    Q_OBJECT
private:
    GraphLayerControls* controls_ ;
    size_t expectedDataSize_ ;

    std::vector<double> sampleCache_ ;
    bool sampleCacheDirty_ = false ;

    std::vector<double> gridVoltageCache_ ;
    std::vector<std::pair<QString, double>> gridTimeCache_ ;
    bool gridCacheDirty_ = true ;

    struct LayerData {
        std::vector<double> data ;
        bool dirty = false ;
    };
    std::unordered_map<int, LayerData> layerData_ ;
    double sampleRate_ ;

    double minTime_ ;
    double maxTime_ ;
    double minVolt_ ;
    double maxVolt_ ;

    QTimer* updateTimer_ ;

    QImage cachedFrame_ ;
    QImage cachedGrid_ ;
    QPolygonF lineBuffer_ ;

public:
    explicit OscilloscopeWidget(QWidget* parent = nullptr);

    // IAnalyzerWidget
    void addLayer(int componentId, const QString& label) override ;
    void removeLayer(int componentId) override ;
    void renameLayer(int componentId, const QString& label) override ;
    void toggleLayer(int componentId, bool enabled) override ;
    void onData(int componentId, const double* data, size_t count) override ;

protected:
    void paintEvent(QPaintEvent* event) override ;
    void resizeEvent(QResizeEvent* event) override ;

private slots:
    void onUpdateTimeout();

private:
    // draw/render
    void drawGrid(QPainter& painter);
    void drawWaveform(QPainter& painter);
    void drawLabels(QPainter& painter);
    void renderToCache();

    void rebuildSampleCache();
    void rebuildGridCache();
    
    // coordinate helpers
    double sampleToX(size_t sampleIndex, size_t total) const ;
    double voltageToY(double amplitude) const ;
};

