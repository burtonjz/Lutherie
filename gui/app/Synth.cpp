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

#include "app/Synth.hpp"
#include "managers/StateManager.hpp"
#include "views/GraphPanel.hpp"
#include "api/ApiClient.hpp"
#include "meta/ComponentRegistry.hpp"
#include "config/Config.hpp"
#include "widgets/SpectrumAnalyzerWidget.hpp"
#include "graphics/ToastNotification.hpp"
#include "app/Theme.hpp"

#include <kddockwidgets/DockWidget.h>
#include <QStandardItemModel>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QWindow>
#include <QToolButton>
#include <QLineEdit>
#include <QWidgetAction>
#include <QCompleter>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>

namespace KDDW   = KDDockWidgets ;

Synth::Synth(QWidget* parent):
    KDDockWidgets::QtWidgets::MainWindow(
        Theme::DEFAULT_WINDOW_TITLE,
        {KDDockWidgets::MainWindowOption_HasCentralWidget}, 
        parent
    ),
    setup_(nullptr),
    componentManager_(new ComponentManager(this)),
    groupManager_(new GroupManager(this)),
    graph_(nullptr),
    spectrumWidget_(nullptr),
    parameterPanel_(new ControlPanel(this)),
    parameterDock_(new KDDWQt::DockWidget("__parameterDock")),
    modulationPanel_(new ControlPanel(this)),
    modulationDock_(new KDDWQt::DockWidget("__modulationDock"))
{
    StateManager::instance();
    setWindowTitle(QString(Theme::DEFAULT_WINDOW_TITLE) + "[*]");

    graph_ = new GraphPanel(componentManager_, this);

    // Docks
    parameterDock_->setWidget(parameterPanel_);
    parameterDock_->setTitle("Parameters");
    addDockWidget(parameterDock_, KDDW::Location_OnRight);

    modulationDock_->setWidget(modulationPanel_);
    modulationDock_->setTitle("Modulation");
    addDockWidget(modulationDock_, KDDW::Location_OnBottom, parameterDock_);

    parameterDock_->close();
    modulationDock_->close();
    
    // menus
    configureMenu();
    configureToolBar();

    // central widget
    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    layout->addWidget(graph_);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setPersistentCentralWidget(container);

    // ==============================
    // ======== CONNECTIONS =========
    // ==============================

    // api client
    connect(
        ApiClient::instance(), &ApiClient::connected, 
        this, &Synth::onApiConnected
    );
    connect(
        ApiClient::instance(), &ApiClient::dataReceived, 
        this, &Synth::onApiDataReceived
    );

    // component manager
    connect( 
        componentManager_, &ComponentManager::componentAdded,
        this, &Synth::onComponentAdded
    );
    
    // graph to main
    connect(
        graph_, &GraphPanel::requestShowParameters,
        this, &Synth::onShowParameters
    );
    connect(
        graph_, &GraphPanel::requestShowModulation,
        this, &Synth::onShowModulation
    );
    connect(
        graph_, &GraphPanel::requestShowGroupParameters,
        this, &Synth::onShowGroupParameters
    );
    connect(
        graph_, &GraphPanel::requestShowGroupModulation,
        this, &Synth::onShowGroupModulation
    );
    connect(
        graph_, &GraphPanel::requestComponentRename,
        this, &Synth::onRequestComponentRename
    );
    connect(
        graph_, &GraphPanel::requestGroupRename, 
        this, &Synth::onRequestGroupRename
    );

    // graph to group
    connect(
        graph_, &GraphPanel::requestGroupCreate,
        groupManager_, &GroupManager::onRequestGroupCreate
    );
    connect(
        graph_, &GraphPanel::requestGroupUpdate,
        groupManager_, &GroupManager::onRequestGroupUpdate
    );
    connect(
        graph_, &GraphPanel::requestGroupRemove,
        groupManager_, &GroupManager::onRequestGroupRemove
    );

    // group to graph
    connect(
        groupManager_, &GroupManager::groupCreated,
        graph_, &GraphPanel::onComponentGroupCreated
    );
    connect(
        groupManager_, &GroupManager::groupUpdated,
        graph_, &GraphPanel::onComponentGroupUpdated
    );
    connect(
        groupManager_, &GroupManager::groupRemoved,
        graph_, &GraphPanel::onComponentGroupRemoved
    );

    // group to main
    connect(
        groupManager_, &GroupManager::groupCreated,
        this, &Synth::onComponentGroupCreated
    );
    connect(
        groupManager_, &GroupManager::groupUpdated,
        this, &Synth::onComponentGroupUpdated
    );
    connect(
        groupManager_, &GroupManager::groupRemoved,
        this, &Synth::onComponentGroupRemoved
    );
}

Synth::~Synth(){
    if ( spectrumWidget_ ) spectrumWidget_->close();
}

void Synth::configureMenu(){
    // file menu
    auto* menuFile = menuBar()->addMenu("File");

    actionLoad_ = new QAction("Load Patch", this);
    actionLoad_->setShortcut(QKeySequence("Ctrl+O"));

    actionSave_ = new QAction("Save", this);
    actionSave_->setShortcut(QKeySequence::Save);

    actionSaveAs_ = new QAction("Save As...", this);
    actionSaveAs_->setShortcut(QKeySequence::SaveAs);

    menuFile->addSeparator();
    menuFile->addAction(actionLoad_);
    menuFile->addSeparator();
    menuFile->addAction(actionSaveAs_);
    menuFile->addAction(actionSave_);

    // view menu
    auto* menuView = menuBar()->addMenu("View");

    actionShowParameterPanel_ = new QAction("Parameter Panel", this);
    actionShowParameterPanel_->setShortcut(QKeySequence("Ctrl+P"));

    actionShowModulationPanel_ = new QAction("Parameter Panel", this);
    actionShowModulationPanel_->setShortcut(QKeySequence("Ctrl+M"));

    actionSpectrumAnalyzer_ = new QAction("Spectrum Analyzer", this);
    actionSpectrumAnalyzer_->setShortcut(QKeySequence("Ctrl+E"));

    menuView->addAction(actionShowParameterPanel_);
    menuView->addAction(actionShowModulationPanel_);
    menuView->addAction(actionSpectrumAnalyzer_);

    auto* menuTools = menuBar()->addMenu("Tools");
    auto* menuHelp = menuBar()->addMenu("Help");

    // connections
    connect(
        actionLoad_, &QAction::triggered, 
        this, &Synth::onActionLoad
    );
    connect(
        actionSave_, &QAction::triggered, 
        this, &Synth::onActionSave
    );
    connect(
        actionSaveAs_, &QAction::triggered, 
        this, &Synth::onActionSaveAs
    );
    connect(
        actionShowParameterPanel_, &QAction::triggered, 
        this, &Synth::onActionToggleParameterPanel
    );
    connect(
        actionShowModulationPanel_, &QAction::triggered, 
        this, &Synth::onActionToggleModulationPanel
    );
    connect(
        actionSpectrumAnalyzer_, &QAction::triggered, 
        this, &Synth::onActionToggleSpectrumAnalyzer
    );
}

void Synth::configureToolBar(){
    toolBar_ = new QToolBar(this);
    toolBar_->setFixedHeight(Theme::TOOLBAR_HEIGHT);
    toolBar_->setMovable(false);

    actionSetup_ = new QAction("Setup", this);
    actionSetup_->setMenuRole(QAction::NoRole);
    toolBar_->addAction(actionSetup_);

    actionStart_ = new QAction("Start", this);
    actionStart_->setMenuRole(QAction::NoRole);
    actionStart_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    toolBar_->addAction(actionStart_);
    
    actionStop_ = new QAction("Stop", this);
    actionStop_->setMenuRole(QAction::NoRole);
    actionStop_->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    actionStop_->setVisible(false);
    toolBar_->addAction(actionStop_);

    // component menu
    QMenu* componentMenu = buildComponentMenu();
    QToolButton* addComponent = new QToolButton(this);
    addComponent->setText("Add Component");
    addComponent->setMenu(componentMenu);
    addComponent->setPopupMode(QToolButton::InstantPopup);
    toolBar_->addWidget(addComponent);

    addToolBar(Qt::TopToolBarArea, toolBar_);

    // connections
    connect(
        this, &Synth::engineStatusChanged, 
        this, &Synth::onEngineStatusChange
    );
    connect(
        actionSetup_, &QAction::triggered, 
        this, &Synth::onActionSetup
    );
    connect(
        actionStart_, &QAction::triggered, 
        this, &Synth::onActionStart
    );
    connect(
        actionStop_, &QAction::triggered, 
        this, &Synth::onActionStop
    );
}

QMenu* Synth::buildComponentMenu(){
    QMenu* menu = new QMenu(this);

    QStringList componentNames ;
    for ( const auto& [typ, descriptor] : ComponentRegistry::getAllComponentDescriptors() ){
        componentNames.append(QString::fromStdString(descriptor.name));
    }

    QLineEdit* search = new QLineEdit(menu);
    search->setPlaceholderText("Search...");
    search->setClearButtonEnabled(true);

    QCompleter* completer = new QCompleter(componentNames, search);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    search->setCompleter(completer);

    QWidgetAction* searchAction = new QWidgetAction(menu);
    searchAction->setDefaultWidget(search);
    menu->addAction(searchAction);
    menu->addSeparator();

    // submenus
    QMenu* sigGen = menu->addMenu("Signal Generators");
    QMenu* sigProc = menu->addMenu("Signal Processors");
    QMenu* midiGen = menu->addMenu("MIDI Generators");
    QMenu* midiProc = menu->addMenu("MIDI Processors");
    QMenu* modulator = menu->addMenu("Modulators");
    
    for ( const auto& [typ, desc] : ComponentRegistry::getAllComponentDescriptors() ){
        if ( desc.numAudioInputs == 0 && desc.numAudioOutputs > 0 ){
            QAction* action = sigGen->addAction(QString::fromStdString(desc.name));
            connect(
                action, &QAction::triggered, 
                this, [this, typ](){
                    graph_->onComponentSelected(typ);
                }
            );
        } else if ( desc.numAudioInputs > 0 && desc.numAudioOutputs > 0 ){
            QAction* action = sigProc->addAction(QString::fromStdString(desc.name));
            connect(
                action, &QAction::triggered, 
                this, [this, typ](){
                    graph_->onComponentSelected(typ);
                }
            );
        }
        if ( desc.numMidiInputs == 0 && desc.numMidiOutputs > 0 ){
            QAction* action = midiGen->addAction(QString::fromStdString(desc.name));
            connect(
                action, &QAction::triggered, 
                this, [this, typ](){
                    graph_->onComponentSelected(typ);
                }
            );
        } else if ( desc.numMidiInputs > 0 && desc.numMidiOutputs > 0 ){
            QAction* action = midiProc->addAction(QString::fromStdString(desc.name));
            connect(
                action, &QAction::triggered, 
                this, [this, typ](){
                    graph_->onComponentSelected(typ);
                }
            );
        }
        if ( desc.canModulate ){
            QAction* action = modulator->addAction(QString::fromStdString(desc.name));
            connect(
                action, &QAction::triggered, 
                this, [this, typ](){
                    graph_->onComponentSelected(typ);
                }
            );
        }
    }

    // completer connections
    connect(
        completer, QOverload<const QString&>::of(&QCompleter::activated),
        this, [search, this](const QString& componentName){
        QTimer::singleShot(0, search, [search](){ search->clear();});
        for ( const auto& [typ, desc] : ComponentRegistry::getAllComponentDescriptors() ){
            if ( componentName == desc.name ){
                graph_->onComponentSelected(typ);
                return ;
            }    
        }
        qDebug() << "component add completer did not match a component name: " << componentName ;
    });

    connect(menu, &QMenu::aboutToShow, search, &QLineEdit::clear);
    return menu ;
}

void Synth::closeEvent(QCloseEvent* event){
    if ( spectrumWidget_ ) spectrumWidget_->close();
    if ( setup_ ) setup_->close() ;

    event->accept();
}

void Synth::onApiConnected(){
}

void Synth::onApiDataReceived(const json& j){
    QString action = QString::fromStdString(j["action"]);

    if ( action == "set_state" ){
        QString state = QString::fromStdString(j["state"]);
        if ( j["status"] != "success"){
            qDebug() << "request to set state was unsuccessful." ;
            return ;
        }
        if (  state == "stop" ){
            emit engineStatusChanged(false);
        } else if ( state == "run" ) {
            emit engineStatusChanged(true);
        } else {
            qDebug() << "invalid state received from set_state" << state ;
        }
        return ;
    }

    if ( action == "get_configuration" ){
        if ( j["status"] != "success" ){
            qDebug() << "request to get configuration data failed." ;
            return ;
        }

        saveData_ = j["data"] ;
        performSave();
    }
}

void Synth::onActionSetup(){
    if ( !setup_ ){
        setup_ = new Setup() ;
        setup_->show();
    } else {
        if (!setup_->isVisible()){
            setup_->show();
        }
    }
}

void Synth::onActionStart(){
    if ( StateManager::instance()->isRunning() ) return ;
    
    json j ;
    j["action"] = "set_state" ;
    j["state"] = "run" ;
    ApiClient::instance()->sendMessage(j);
}

void Synth::onActionStop(){
    if ( ! StateManager::instance()->isRunning() ) return ;
    json j ;
    j["action"] = "set_state" ;
    j["state"] = "stop" ;
    ApiClient::instance()->sendMessage(j);
}

void Synth::onEngineStatusChange(bool status){
    StateManager::instance()->setRunning(status);
    actionStart_->setVisible(!status);
    actionStop_->setVisible(status);
}

void Synth::onActionLoad(){
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Load Configuration"),
        QDir::homePath(),
        tr("JSON Files (*.json);;All Files (*)")
    );

    if (filePath.isEmpty()) {
        return; 
    }
    
    QFile file(filePath);
    if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        ToastNotification::show(graph_->scene(), graph_,
            "Filed to open file " + filePath + ": " + file.errorString()
        );
        return;
    }
    
    QByteArray fileData = file.readAll();
    file.close();
    
    try {
        saveData_ = json::parse(fileData.data());
        saveFilePath_ = filePath ;
    } catch (std::exception& e ){
        ToastNotification::show(graph_->scene(), graph_, 
            "Failed to load file " + filePath + ". Invalid json: " + e.what()
        );
        return ;
    }
    
    // send API request
    saveData_["action"] = "load_patch" ;
    ApiClient::instance()->sendMessage(saveData_);
}

void Synth::onActionSave(){
    if (saveFilePath_.isEmpty()){
        onActionSaveAs();
    } else {
        json j ;
        j["action"] = "get_configuration" ;
        ApiClient::instance()->sendMessage(j);
    }
}

void Synth::onActionSaveAs(){
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Configuration"),
        QDir::homePath(),
        tr("JSON Files (*.json);;All Files (*)")
    );

    if (!filePath.isEmpty()){
        if ( !filePath.endsWith(".json")){
            filePath.append(".json");
        }
        saveFilePath_ = filePath ;
        json j ;
        j["action"] = "get_configuration" ;
        ApiClient::instance()->sendMessage(j);
    }
}

void Synth::performSave(){
    saveData_["nodes"] = graph_->serializeNodes();
    QFile file(saveFilePath_);
    if (!file.open(QIODevice::WriteOnly)){
        QMessageBox::warning(this, "Save Failed",
            "Could not open file for writing:\n" + saveFilePath_ );
        return ;
    }

    QByteArray data = saveData_.dump(2).c_str();
    file.write(data);
    file.close();
    qDebug() << "file" << saveFilePath_ << "saved." ; 
    setWindowModified(false);
    windowHandle()->requestUpdate();
    return ;
}

void Synth::onActionToggleSpectrumAnalyzer(){
    if ( !spectrumWidget_ ){
        spectrumWidget_ = new SpectrumAnalyzerWidget();
        int port = Config::get<int>("analysis.spectrum_analyzer.port").value_or(54322);
        spectrumWidget_->setPort(port);
        spectrumWidget_->setAttribute(Qt::WA_DeleteOnClose);
    }

    spectrumWidget_->show();
    spectrumWidget_->raise();
    spectrumWidget_->activateWindow();
}

void Synth::onActionToggleParameterPanel(){
    if ( parameterDock_->isOpen() ){
        parameterDock_->close();
    } else {
        parameterDock_->open();
    }
}

void Synth::onActionToggleModulationPanel(){
    if ( modulationDock_->isOpen() ){
        modulationDock_->close();
    } else {
        modulationDock_->open();
    }
}

void Synth::onComponentAdded(int componentId, ComponentType typ){
    auto params = componentManager_->getParameters(componentId);

    auto modParams = componentManager_->getModulationParameters(componentId);

    QString name = QString::fromStdString(params->getModel()->getDescriptor().name);

    parameterPanel_->addContent(name, params);
    if ( ! parameterDock_->isOpen() ) parameterDock_->open();
    connect(params, &QObject::destroyed, this, [this, params](){
        parameterPanel_->removeContent(params);
    });

    modulationPanel_->addContent(name + " Modulation", modParams);
    if ( ! modulationDock_->isOpen() ) modulationDock_->open();
    connect(modParams, &QObject::destroyed, this, [this, modParams](){
        modulationPanel_->removeContent(modParams);
    });
}

void Synth::onComponentRemoved(int componentId){
}

void Synth::onShowParameters(int componentId){
    if ( parameterDock_->isHidden() ) parameterDock_->open() ;
    parameterPanel_->maximizeSection(componentManager_->getParameters(componentId));
}

void Synth::onShowModulation(int componentId){
    if ( modulationDock_->isHidden() ) modulationDock_->open() ;
    modulationPanel_->maximizeSection(componentManager_->getModulationParameters(componentId));
}

void Synth::onShowGroupParameters(int groupId){
}

void Synth::onShowGroupModulation(int groupId){
}

void Synth::onComponentGroupCreated(int groupId, std::unordered_set<int> componentIds){

}
void Synth::onComponentGroupRemoved(int groupId, std::unordered_set<int> componentIds){

}
void Synth::onComponentGroupUpdated(int groupId, std::unordered_set<int> componentIds){

}

void Synth::onRequestComponentRename(int componentId, QString name){
    auto m = componentManager_->getModel(componentId);
    if ( !m ) return ;

    m->setName(name);
    
    // graph node renames
    auto n = graph_->getComponentNode(componentId);
    if ( n ){
        n->onRename(name);
    }

    // tell panels to update headers
    auto paramContent = componentManager_->getParameters(componentId);
    auto paramSection = parameterPanel_->getSection(paramContent);
    paramSection->setTitle(name);
    
    auto modContent = componentManager_->getModulationParameters(componentId);
    auto modSection = modulationPanel_->getSection(modContent);
    modSection->setTitle(name);
}

void Synth::onRequestGroupRename(int groupId, QString name){
    auto m = groupManager_->getModel(groupId);
    if ( !m ) return ;

    m->setName(name);

    // graph node renames
    auto n = graph_->getGroupNode(groupId);
    if ( n ){
        n->onRename(name);
    }

    // tell panels to update headers
}