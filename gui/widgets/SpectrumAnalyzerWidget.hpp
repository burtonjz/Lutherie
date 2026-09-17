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


#ifndef SPECTRUM_ANALYZER_WIDGET_HPP
#define SPECTRUM_ANALYZER_WIDGET_HPP

#include "interfaces/IAnalyzerWidget.hpp"
#include "widgets/GraphLayerControls.hpp"

#include <QWidget>
#include <vector>
#include <QPainter>
#include <QTimer>

class SpectrumAnalyzerWidget : public QWidget, public IAnalyzerWidget {
    Q_OBJECT

private:
    GraphLayerControls* controls_ ;
    size_t expectedDataSize_ ;

    // cache for bin frequency values so long as 
    struct FrequencyBins { 
        double start ; 
        std::vector<double> right ;
    };
    FrequencyBins binCache_ ;
    bool binCacheDirty_ = false ;

    std::vector<double> gridFreqCache_ ;
    std::vector<double> gridDbCache_ ;
    bool gridCacheDirty_ = true ;

    struct LayerData {
        std::vector<double> data ;
        bool dirty = false ;
    };
    std::unordered_map<int, LayerData> layerData_ ;

    double sampleRate_ ;
    
    // Display ranges
    double minFreq_ ;
    double maxFreq_ ;
    double minDb_ ;
    double maxDb_ ;
    
    QTimer* updateTimer_ ;

    QImage cachedFrame_ ;
    QImage cachedGrid_ ;
    QPolygonF lineBuffer_ ;

public:
    explicit SpectrumAnalyzerWidget(QWidget *parent = nullptr);

    // IAnalyzerWidget
    void addLayer(int componentId, const QString& label) override ;
    void removeLayer(int componentId) override ;
    void renameLayer(int componentId, const QString& label) override ;
    void toggleLayer(int componentId, bool enabled) override ;
    void onData(int componentId, const double* data, size_t count) override ;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onUpdateTimeout();

private:
    // draw/render
    void drawGrid(QPainter &painter);
    void drawSpectrum(QPainter &painter);
    void drawLabels(QPainter &painter);
    void renderToCache();

    void rebuildBinCache();
    void rebuildGridCache();

    // coordinate manipulation
    double freqToX(double freq) const ;
    double dbToY(double db) const ;
};

#endif // SPECTRUM_ANALYZER_WIDGET_HPP