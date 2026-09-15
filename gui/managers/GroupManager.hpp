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

#ifndef GROUP_MANAGER_HPP_
#define GROUP_MANAGER_HPP_

#include "models/GroupModel.hpp"

#include <QObject>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class GroupManager : public QObject {
    Q_OBJECT

private:
    int currentGroupId_ ;
    std::unordered_map<int, GroupModel*> groups_ ;
    std::unordered_map<int, QWidget*> parameterContent_ ;
    std::unordered_map<int, QWidget*> modulationContent_ ;

    explicit GroupManager(QObject* parent = nullptr);

public:
    static GroupManager* instance();

    GroupManager(const GroupManager&) = delete ;
    GroupManager& operator=(const GroupManager&) = delete ;
    GroupManager(GroupManager&&) = delete ;
    GroupManager& operator=(GroupManager&&) = delete ;

    GroupModel* getModel(int groupId) const ;
    GroupModel* getComponentGroup(int componentId) const ;

    QWidget* getParameters(int groupId) const ;
    QWidget* getModulationParameters(int groupId) const ;

    void setParameters(int groupId, QWidget* content);
    void setModulationParameters(int groupId, QWidget* content);

    void removeContent(int groupId);

    GroupModel* createGroup(std::vector<int> componentIds);
    void updateGroup(int groupId, std::vector<int> componentIds);
    void removeGroup(int groupId);

signals:
    void groupCreated(GroupModel* model);
    void groupRemoved(GroupModel* model);
    void groupRenamed(GroupModel* model);

};

#endif // GROUP_MANAGER_HPP_
