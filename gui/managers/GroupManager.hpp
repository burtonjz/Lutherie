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
#include <unordered_set>
#include <unordered_map>

class GroupManager : public QObject {
    Q_OBJECT

private:
    int currentGroupId_ ;
    std::unordered_map<int, GroupModel*> groups_ ;

public:
    explicit GroupManager(QObject* parent = nullptr);
    ~GroupManager();

    GroupModel* getModel(int groupId) const ;
    GroupModel* getComponentGroup(int componentId) const ;

public slots:
    void onRequestGroupCreate(std::vector<int> componentIds);
    void onRequestGroupUpdate(int groupId, std::vector<int> componentIds);
    void onRequestGroupRemove(int groupId);

signals:
    void groupCreated(int groupId, const std::unordered_set<int>& componentIds);
    void groupUpdated(int groupId, const std::unordered_set<int>& componentIds);
    void groupRemoved(int groupId, const std::unordered_set<int>& componentIds);



};

#endif // GROUP_MANAGER_HPP_
