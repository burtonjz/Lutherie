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

#ifndef GROUP_MODEL_HPP_
#define GROUP_MODEL_HPP_

#include "models/ComponentModel.hpp"
#include "types/ParameterType.hpp"

#include <QObject>
#include <vector>
#include <map>

enum class ParameterExposure {
    Visible, // normal visibility
    Hidden,  // not shown, not modulatable
    Locked   // shown as read-only display, not modulatable
};

class GroupModel : public QObject {
    Q_OBJECT

private:
    struct ParameterConfig {
        ParameterExposure exposure = ParameterExposure::Visible ;
    };
    
    int id_ ;
    QString name_ ;
    std::map<int, ComponentModel*> models_ ;

public:
    GroupModel(int id, QString name = "");

    int getId() const ;
    QString getName() const ;

    void addComponent(ComponentModel* model);
    void removeComponent(int componentId);
    const std::vector<int> getComponents() const ;

    // ParameterExposure getExposure(int componentId, ParameterType p) const ;
    // void setExposure(int componentId, ParameterType p, ParameterExposure e);

    // bool isVisible(int componentId, ParameterType p) const ;
    // bool isLocked(int componentId, ParameterType p) const ;

signals:
    void ParameterExposureChanged(int componentId, ParameterType p, ParameterExposure e);

};

#endif // GROUP_MODEL_HPP_