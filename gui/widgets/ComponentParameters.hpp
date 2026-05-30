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

#ifndef COMPONENT_PARAMETERS_HPP_
#define COMPONENT_PARAMETERS_HPP_

#include "models/ComponentModel.hpp"

#include "widgets/ParameterWidget.hpp"
#include "widgets/FileSelectorWidget.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QGridLayout>

class ComponentParameters : public QWidget {
    Q_OBJECT

private:
    ComponentModel* model_ ;
    std::map<ParameterType, ParameterWidget*> parameterWidgets_ ; // general independent parameters
    QWidget* detailedEditor_ ; // specialized controls designed to display in its own dock.
    FileSelectorWidget* fileSelector_ ; // only for file components (see descriptor)

    QTimer* parameterChangedTimer_ ;
    std::unordered_map<ParameterType, ParameterValue> pendingChanges_ ;

    QVBoxLayout* mainLayout_ ;
    QGridLayout* paramLayout_ ;

public:
    explicit ComponentParameters(ComponentModel* model, QWidget* parent = nullptr);
    ~ComponentParameters() override = default ;

    ComponentModel* getModel() const ;
    QWidget* getDetailedEditor() const ;
    bool hasDetailedEditor() const ;

protected:
    ParameterWidget* createParameterWidget(ParameterType p);
    QWidget* createDetailedEditor(ComponentType t);
    void resizeEvent(QResizeEvent* event) override ;

private:
    void rebuildLayout();
    int computeColumnCount() const ;

signals:
    void parameterEdited(int componentId, ParameterType p, ParameterValue value);
    void fileSelected(int componentId, std::string path);
    
public slots:
    void onValueChange();

private slots:
    void flushPendingChanges();

};

#endif // COMPONENT_PARAMETERS_HPP_