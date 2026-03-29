/*
 * Copyright (C) 2025 Jared Burton
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

#ifndef COMPONENT_EDITOR_HPP_
#define COMPONENT_EDITOR_HPP_

#include "types/ParameterType.hpp"
#include "widgets/ComponentParameters.hpp"

#include <kddockwidgets/DockWidget.h>
#include <kddockwidgets/MainWindow.h>
#include <QPushButton>

// forward declarations
class ComponentModel ;
namespace KDDW = KDDockWidgets::QtWidgets ;

class ComponentEditor : public KDDW::DockWidget {
    Q_OBJECT
    
private:
    QWidget* container_ ;
    ComponentParameters* params_ ;
    QPushButton* resetButton_ ;
    QPushButton* closeButton_ ;

public:
    explicit ComponentEditor(ComponentModel* model, KDDW::MainWindow* mainWindow);
    ~ComponentEditor() override ;

    ComponentParameters* getComponentParameters() const ;

    QString getName() const ;
    void setName(const QString& name);
     
private:
    void setupLayout();

signals:
    // passthrough for ComponentParameters
    void parameterEdited(int componentId, ParameterType p, ParameterValue value);

private slots:
    void onCloseButtonClicked();
    void onResetButtonClicked();

};


#endif // COMPONENT_EDITOR_HPP_