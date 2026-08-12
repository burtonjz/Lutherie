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

#ifndef __UI_SYNTH_HPP_
#define __UI_SYNTH_HPP_

#include "types/ComponentType.hpp"

#include <kddockwidgets/MainWindow.h>
#include <QUiLoader>
#include <QFile>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QEvent>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;
namespace KDDWQt = KDDockWidgets::QtWidgets ;

// forward declarations
class ComponentParameters ;
class ControlPanel ;

class Synth : public KDDWQt::MainWindow {
    Q_OBJECT

private:
    // docks
    KDDWQt::DockWidget* peripheralConfigDock_ ;

    KDDWQt::DockWidget* graphDock_ ;

    ControlPanel* parameterPanel_ ;
    KDDWQt::DockWidget* parameterDock_ ;

    ControlPanel* modulationPanel_ ;
    KDDWQt::DockWidget* modulationDock_ ;

    std::unordered_map<ComponentType, KDDWQt::DockWidget*> analyzerDocks_ ;
    std::unordered_map<int, KDDWQt::DockWidget*> componentDetailDocks_ ;

    // save/load 
    QString saveFilePath_ ;
    json saveData_ ;

    // ========= MENUS / ACTIONS =========
    // file menu
    QAction* actionLoad_ ;
    QAction* actionSave_ ;
    QAction* actionSaveAs_ ;

    // view menu
    QAction* actionShowGraph_ ;
    QAction* actionShowParameterPanel_ ;
    QAction* actionShowModulationPanel_ ;

    // toolbar 
    QAction* actionPeripheralConfig_ ;
    QAction* actionStart_ ;
    QAction* actionStop_ ;

    QMenuBar* menuBar_ ;
    QToolBar* toolBar_ ;

    // for component filtering
    QMenu* componentMenu_ ;
    std::map<std::string, QMenu*> tagMenu_ ;
    std::map<QAction*, ComponentType> actionType_ ;
    std::set<ComponentType> visibleType_ ;
    std::vector<QAction*> componentMenuQuickAction_ ;

public:
    Synth(QWidget* parent = nullptr);
    ~Synth();

private:
    void configureMenu();
    void configureToolBar();
    void configureDocks();
    void makeExternalConnections();

    QMenu* buildComponentMenu();

    void createComponentDetailDock(int componentId, ComponentParameters* params);
    void performSave();

signals:
    void engineStatusChanged(bool status);

private slots:
    void onApiConnected();
    void onControlMessageReceived(const json& json);

    void onEngineStatusChange(bool status);

    // tool bar menu actions
    void onActionPeripheralConfig();
    void onActionStart();
    void onActionStop();

    // file menu actions
    void onActionLoad();
    void onActionSave();
    void onActionSaveAs();

    // view menu actions
    void onActionToggleAnalyzer(ComponentType typ);
    void onActionToggleGraph();
    void onActionToggleParameterPanel();
    void onActionToggleModulationPanel();

public slots:
    void onComponentAdded(int componentId, ComponentType typ);
    void onComponentRemoved(int componentId);

    void onShowParameters(int componentId);
    void onShowModulation(int componentId);
    void onShowGroupParameters(int groupId);
    void onShowGroupModulation(int groupId);

    // for framing groups in panels
    void onComponentGroupCreated(int groupId, std::vector<int> componentIds);
    void onComponentGroupRemoved(int groupId, std::vector<int> componentIds);
    void onComponentGroupUpdated(int groupId, std::vector<int> componentIds);

    void onComponentRenamed(int componentId);
    void onGroupRenamed(int groupId);

};

#endif // __UI_SYNTH_HPP_