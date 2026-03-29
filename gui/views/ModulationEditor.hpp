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

#ifndef MODULATION_EDITOR_HPP_
#define MODULATION_EDITOR_HPP_

#include "models/ModulationModel.hpp"
#include "types/ParameterType.hpp"
#include "widgets/ModulationControl.hpp"

#include <kddockwidgets/DockWidget.h>
#include <kddockwidgets/MainWindow.h>
#include <QWidget>
#include <QScrollArea>
#include <QPushButton>
#include <QGridLayout>
#include <map>
#include <utility>
#include <vector>

namespace KDDW = KDDockWidgets::QtWidgets ;

class ModulationEditor : public KDDW::DockWidget {
    Q_OBJECT

private:
    QWidget* container_ ;
    QWidget* gridContainer_ ;
    QScrollArea* scroll_ ;
    std::map<std::pair<int, ParameterType>, ModulationControl*> modulationControls_ ;
    std::vector<std::pair<int, ParameterType>> controlOrder_ ;

    QGridLayout* ctrlLayout_ ;
    QPushButton* closeButton_ ;

public:
    ModulationEditor(QString name, KDDW::MainWindow* mainWindow);
    ~ModulationEditor();

    void add(ModulationModel* model);
    void remove(int componentId, ParameterType p);

    QString getName() const ;
    void setName(const QString& name);

    void setModulationStatus(int componentId, ParameterType p, bool active);
    
private:
    void setupLayout();
    void updateLayout();
    
private slots:
    void onCloseButtonClicked();

signals:
    void modulationDepthEdited(int componentId, ParameterType p, double depth);
    void modulationStrategyEdited(int componentId, ParameterType p, ModulationStrategy strategy);

};

#endif // MODULATION_EDITOR_HPP_