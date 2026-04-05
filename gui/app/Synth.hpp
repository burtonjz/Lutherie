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

#include "app/Setup.hpp"
#include "views/GraphPanel.hpp"
#include "views/ControlPanel.hpp"
#include "widgets/SpectrumAnalyzerWidget.hpp"

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

class Synth : public KDDWQt::MainWindow {
    Q_OBJECT

private:
    Setup* setup_ ;
    GraphPanel* graph_ ;
    ComponentManager* componentManager_ ;
    SpectrumAnalyzerWidget* spectrumWidget_ ;

    // docks
    ControlPanel* parameterPanel_ ;
    KDDWQt::DockWidget* parameterDock_ ;

    ControlPanel* modulationPanel_ ;
    KDDWQt::DockWidget* modulationDock_ ;

    // save/load 
    QString saveFilePath_ ;
    json saveData_ ;

    // menu Actions
    QAction* actionLoad_ ;
    QAction* actionSave_ ;
    QAction* actionSaveAs_ ;
    QAction* actionSpectrumAnalyzer_ ;

    // toolbar Actions
    QAction* actionSetup_ ;
    QAction* actionStart_ ;
    QAction* actionStop_ ;

    QMenuBar* menuBar_ ;
    QToolBar* toolBar_ ;

public:
    Synth(QWidget* parent = nullptr);
    ~Synth();

private:
    void configureMenuActions();
    void configureToolBar();
    QMenu* buildComponentMenu();

    void performSave();

    void closeEvent(QCloseEvent* event) override ;

signals:
    void engineStatusChanged(bool status);

private slots:
    void onApiConnected();
    void onApiDataReceived(const json& json);

    void onEngineStatusChange(bool status);

    void onActionSetup();
    void onActionStart();
    void onActionStop();
    void onActionLoad();
    void onActionSave();
    void onActionSaveAs();
    void onActionSpectrumAnalyzer();

public slots:
    void onComponentAdded(int componentId, ComponentType typ);
    void onComponentRemoved(int componentId);

};

#endif // __UI_SYNTH_HPP_