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

#include <QObject>
#include <vector>

class GroupModel : public QObject {
    Q_OBJECT

private:
    int id_ ;
    QString name_ ;
    std::vector<int> componentIds_ ;

public:
    GroupModel(int id, QString name = "");

    int getId() const ;
    const QString& getName() const ;
    void setName(QString name);

    void addComponent(int componentId);
    void removeComponent(int componentId);
    bool hasComponent(int componentId);
    const std::vector<int>& getComponents() const ;

signals:
    void groupRenamed(int groupId);

};

#endif // GROUP_MODEL_HPP_