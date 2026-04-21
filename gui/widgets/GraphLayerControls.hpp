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

#ifndef GRAPH_LAYER_CONTROLS_HPP_
#define GRAPH_LAYER_CONTROLS_HPP_

#include <QWidget>

#include <QWidget>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QFrame>
#include <QMap>

class GraphLayerControls : public QWidget {
    Q_OBJECT

private:
    QGridLayout* layout_;

    struct LayerEntry {
        int index ;
        QColor color ;
        QCheckBox* checkbox ;
        QFrame* colorSwatch ;
        QLabel* indexLabel ;
        QLabel* nameLabel ;
    };
    std::map<int, LayerEntry> layers_ ;

public:
    explicit GraphLayerControls(QWidget* parent = nullptr);

    bool isLayerPresent(int componentId) const ;
    void addLayer(int componentId, const QString& label);
    void removeLayer(int componentId);
    void renameLayer(int componentId, const QString& label);
    void toggleLayer(int componentId, bool enabled);
    bool isLayerEnabled(int componentId) const ;
    QColor layerColor(int componentId) const ;

signals:
    void layerToggled(int componentId, bool enabled);

protected:
    void resizeEvent(QResizeEvent* event) override ;

private:
    void rebuildLayout();
    int  computeColumnCount() const ;

};

#endif // GRAPH_LAYER_CONTROLS_HPP_