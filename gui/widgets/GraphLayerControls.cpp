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

#include "widgets/GraphLayerControls.hpp"
#include "app/Theme.hpp"

#include <QResizeEvent>

GraphLayerControls::GraphLayerControls(QWidget* parent)
    : QWidget(parent)
    , layout_(new QGridLayout(this))
{
    layout_->setContentsMargins(4, 4, 4, 4);
    layout_->setSpacing(6);
    setLayout(layout_);
}

bool GraphLayerControls::isLayerPresent(int componentId) const {
    return layers_.contains(componentId);
}

void GraphLayerControls::addLayer(int componentId, const QString& label) {
    if ( layers_.contains(componentId) ) return ;

    const int idx = layers_.size() % Theme::SPECTRUM_LINE_COLORS.size() ;
    const QColor color = Theme::SPECTRUM_LINE_COLORS[idx];

    // Color swatch
    auto* swatch = new QFrame(this);
    swatch->setFixedSize(14, 14);
    swatch->setStyleSheet(
        QString("background:%1; border-radius:3px;").arg(color.name())
    );

    // Index label
    auto* indexLabel = new QLabel(QString("#%1").arg(idx), this);
    indexLabel->setFixedWidth(28);

    // checkbox (show / hide)
    auto* checkbox = new QCheckBox(this);
    checkbox->setChecked(true);
    connect(
        checkbox, &QCheckBox::toggled, 
        this, [this, componentId](bool on) {
            emit layerToggled(componentId, on);
    });

    // Name label
    auto* nameLabel = new QLabel(label, this);
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    layers_[componentId] = {
        .index=idx, 
        .color=color, 
        .checkbox=checkbox, 
        .colorSwatch=swatch, 
        .indexLabel = indexLabel, 
        .nameLabel = nameLabel
    };
    rebuildLayout();
}

void GraphLayerControls::removeLayer(int componentId) {
    if (!layers_.contains(componentId)) return;

    auto& e = layers_[componentId];
    e.checkbox->deleteLater() ;
    e.colorSwatch->deleteLater() ;
    e.indexLabel->deleteLater() ;
    e.nameLabel->deleteLater() ;
    layers_.erase(componentId);

    rebuildLayout();
}

void GraphLayerControls::renameLayer(int componentId, const QString& label) {
    if ( layers_.contains(componentId) ){
        layers_.at(componentId).nameLabel->setText(label);
    }       
}

void GraphLayerControls::toggleLayer(int componentId, bool enabled) {
    if ( layers_.contains(componentId) ){
        layers_.at(componentId).checkbox->setChecked(enabled);
    }
}

bool GraphLayerControls::isLayerEnabled(int componentId) const {
    return layers_.contains(componentId)
        && layers_.at(componentId).checkbox->isChecked();
}

QColor GraphLayerControls::layerColor(int componentId) const {
    return layers_.contains(componentId)
        ? layers_.at(componentId).color
        : Qt::white ;
}

void GraphLayerControls::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    rebuildLayout();
}

int GraphLayerControls::computeColumnCount() const {
    // Each square of controls is ~180px wide; at least 1 column
    return std::max(1, width() / 180);
}

void GraphLayerControls::rebuildLayout() {
    // Pull every widget out without deleting it
    while ( layout_->count() ){
        layout_->takeAt(0);   
    }
        
    const int cols = computeColumnCount();
    int col = 0, row = 0 ;

    for ( auto& [id, layer] : layers_ ) {
        /* Each layer creates a nested horizontal grouping:
         [checkbox] [swatch] [#idx] [name label]
        */ 

        int base = col * 4 ;   // 4 grid columns per layer
        layout_->addWidget(layer.checkbox,    row, base + 0, Qt::AlignVCenter);
        layout_->addWidget(layer.colorSwatch, row, base + 1, Qt::AlignVCenter);
        layout_->addWidget(layer.indexLabel,  row, base + 2, Qt::AlignVCenter);
        layout_->addWidget(layer.nameLabel,   row, base + 3, Qt::AlignVCenter);

        layout_->setColumnStretch(base + 3, 1); // name label takes spare space

        ++col ;
        if ( col >= cols ) { col = 0; ++row; }
    }
}

