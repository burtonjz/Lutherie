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

#ifndef MODULATION_PARAMETERS_HPP_
#define MODULATION_PARAMETERS_HPP_

#include "models/ComponentModel.hpp"
#include "widgets/ModulationControl.hpp"

#include <QWidget>
#include <QGridLayout>

class ModulationParameters : public QWidget {
    Q_OBJECT

private:
    int componentId_ ;
    std::map<ParameterType, ModulationControl*> modulationControls_ ;

    QTimer* controlChangedTimer_ ;
    std::unordered_map<ParameterType, double> pendingDepth_ ;
    std::unordered_map<ParameterType, ModulationStrategy> pendingStrategy_ ;

    QGridLayout* ctrlLayout_ ;

public:
    explicit ModulationParameters(ComponentModel* model, QWidget* parent = nullptr);
    ~ModulationParameters() override = default ;

    bool hasModulationControl(ParameterType p) const ;

    void setConnectionStatus(ParameterType p, bool active);

private:
    ModulationControl* addModulationControl(ModulationModel* m);
    void layoutControls();

private slots:
    void onDepthEdited();
    void onStrategyEdited();
    void flushPendingChanges();

    void onModelDepthChanged(int componentId, ParameterType p, double depth);
    void onModelStrategyChanged(int componentId, ParameterType p, ModulationStrategy strategy);

signals:
    void modulationDepthEdited(int componentId, ParameterType p, double depth);
    void modulationStrategyEdited(int componentId, ParameterType p, ModulationStrategy strategy);

};

#endif // MODULATION_PARAMETERS_HPP_