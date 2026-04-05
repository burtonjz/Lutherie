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

#ifndef MODULATION_CONTROL_HPP_
#define MODULATION_CONTROL_HPP_

#include "types/ParameterType.hpp"
#include "widgets/ParameterWidget.hpp"
#include "widgets/ModulationIndicator.hpp"

#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

class ModulationControl : public QWidget {
    Q_OBJECT

private:
    ParameterType parameter_ ;

    QLabel* paramLabel_ ;
    SliderWidget* depthSlider_ ;
    QLabel* strategyLabel_ ;
    QComboBox* strategySelector_ ;
    ModulationIndicator* modIndicator_ ;

public:
    ModulationControl(ParameterType p, QWidget* parent = nullptr);

    void setConnectionStatus(bool active);
    
private:
    void setupLayout();

public slots:
    void onModelDepthChanged(ParameterType p, double depth);
    void onModelStrategyChanged(ParameterType p, ModulationStrategy strategy);

signals:
    void depthEdited(ParameterType p, double depth);
    void strategyEdited(ParameterType p, ModulationStrategy strategy);

};

#endif // MODULATION_CONTROL_HPP_