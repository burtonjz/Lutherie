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

#ifndef ANALYSIS_MANAGER_HPP_
#define ANALYSIS_MANAGER_HPP_

#include "types/ComponentType.hpp"
#include "interfaces/IAnalyzerWidget.hpp"

#include <QObject>
#include <QUdpSocket>


class AnalysisManager: public QObject {
    Q_OBJECT

private:
    std::unordered_map<ComponentType, IAnalyzerWidget*> analyzerWidgets_ ;
    std::unordered_map<int, ComponentType> registeredComponents_ ;

    QUdpSocket* udpSocket_ ;
    quint16 port_ ;
     
public:
    explicit AnalysisManager(QObject* parent = nullptr);

    void setPort(quint16 port);

    QWidget* getAnalyzerWidget(ComponentType typ) const;
    std::vector<ComponentType> getAnalyzerTypes() const ;

private slots:
    void onReadyRead();

public slots:
    void onComponentAdded(int componentId, ComponentType typ);
    void onComponentRemoved(int componentId);
    void onComponentRename(int componentId, QString name);

};

#endif // ANALYSIS_MANAGER_HPP_