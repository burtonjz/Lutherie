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

#ifndef MODULATION_MODEL_HPP_
#define MODULATION_MODEL_HPP_

#include "types/ParameterType.hpp"

#include <QObject>

class ModulationModel : public QObject {
    Q_OBJECT

private:
    int id_ ;
    ParameterType parameter_ ;

    double depth_ ;
    ModulationStrategy strategy_ ;

    bool isConnected_ ;
    int connectedId_ ; // component sending outbound modulation signal

public:
    ModulationModel(int id, ParameterType p);

    int getId() const ;
    ParameterType getType() const ;

    double getDepth() const ;
    void setDepth(double depth, bool block = false);

    ModulationStrategy getStrategy() const ;
    void setStrategy(ModulationStrategy strat, bool block = false);

    void sync() const ;

signals:
    void modulationDepthChanged(int componentId, ParameterType p, double depth) const ;
    void modulationStrategyChanged(int componentId, ParameterType p, ModulationStrategy strategy) const ;
};

#endif // MODULATION_MODEL_HPP_