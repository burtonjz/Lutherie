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
#include "managers/ComponentManager.hpp"
#include "managers/GroupManager.hpp"
#include "managers/AnalysisManager.hpp"
#include "views/GraphPanel.hpp"
#include "api/ControlApiClient.hpp"
#include "meta/ComponentRegistry.hpp"
#include "config/Config.hpp"
#include "graphics/ToastNotification.hpp"
#include "app/Theme.hpp"
#include "widgets/ComponentParameters.hpp"

#include <kddockwidgets/DockWidget.h>
#include <kddockwidgets/core/FloatingWindow.h>
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
        {KDDockWidgets::MainWindowOption_None}, 
        parent
    ),
    setup_(nullptr),
    graph_(nullptr),
    parameterPanel_(new ControlPanel(this)),
    parameterDock_(new KDDWQt::DockWidget("__parameterDock")),
    modulationPanel_(new ControlPanel(this)),
    modulationDock_(new KDDWQt::DockWidget("__modulationDock")),
    analyzerDocks_(),
    componentDetailDocks_()
{
    StateManager::instance();
    setWindowTitle(QString(Theme::DEFAULT_WINDOW_TITLE) + "[*]");

    graph_ = new GraphPanel(this);
    graphDock_ = new KDDWQt::DockWidget("__graphDock");

    configureDocks();
    configureMenu();
    configureToolBar();

    // ==============================
    // ======== CONNECTIONS =========
    // ==============================

    // api client
    connect(
        ControlApiClient::instance(), &ControlApiClient::connected, 
        this, &Synth::onApiConnected
    );
    connect(
        ControlApiClient::instance(), &ControlApiClient::dataReceived, 
        this, &Synth::onControlMessageReceived
    );

    // component manager
    connect( 
        ComponentManager::instance(), &ComponentManager::componentAdded,
        this, &Synth::onComponentAdded
    );
    connect(
        ComponentManager::instance(), &ComponentManager::componentRenamed,
        this, &Synth::onComponentRenamed
    );
    
    // group manager
    connect(
        GroupManager::instance(), &GroupManager::groupCreated,
        this, &Synth::onComponentGroupCreated
    );
    connect(
        GroupManager::instance(), &GroupManager::groupUpdated,
        this, &Synth::onComponentGroupUpdated
    );
    connect(
        GroupManager::instance(), &GroupManager::groupRemoved,
        this, &Synth::onComponentGroupRemoved
    );
    connect(
        GroupManager::instance(), &GroupManager::groupRenamed, 
        this, &Synth::onGroupRenamed
    );
    
    // graph panel
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
}

Synth::~Synth(){
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

    actionShowGraph_  = new QAction("Connection Graph", this);
    actionShowGraph_->setShortcut(QKeySequence("Ctrl+R"));
    menuView->addAction(actionShowGraph_);

    actionShowParameterPanel_ = new QAction("Parameter Panel", this);
    actionShowParameterPanel_->setShortcut(QKeySequence("Ctrl+P"));
    menuView->addAction(actionShowParameterPanel_);

    actionShowModulationPanel_ = new QAction("Modulation Panel", this);
    actionShowModulationPanel_->setShortcut(QKeySequence("Ctrl+M"));
    menuView->addAction(actionShowModulationPanel_);
    
    for ( const auto& [key, _]: analyzerDocks_ ){
        QString name = QString::fromStdString(ComponentRegistry::getComponentDescriptor(key).name);
        QAction* action = menuView->addAction(name);
        connect(
            action, &QAction::triggered,
            this, [this, key](){
                onActionToggleAnalyzer(key);
        });
    }

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
        actionShowGraph_, &QAction::triggered,
        this, &Synth::onActionToggleGraph
    );
    connect(
        actionShowParameterPanel_, &QAction::triggered, 
        this, &Synth::onActionToggleParameterPanel
    );
    connect(
        actionShowModulationPanel_, &QAction::triggered, 
        this, &Synth::onActionToggleModulationPanel
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

void Synth::configureDocks(){
    graphDock_->setWidget(graph_);
    graphDock_->setTitle("Connection Graph");
    addDockWidget(graphDock_, KDDW::Location_OnLeft);

    parameterDock_->setWidget(parameterPanel_);
    parameterDock_->setTitle("Parameters");
    addDockWidget(parameterDock_, KDDW::Location_OnRight, graphDock_);
    
    modulationDock_->setWidget(modulationPanel_);
    modulationDock_->setTitle("Modulation");
    parameterDock_->addDockWidgetAsTab(modulationDock_);

    parameterDock_->close();
    modulationDock_->close();

    // Analyzer Docks
    for ( auto typ : AnalysisManager::instance()->getAnalyzerTypes() ){
        QString name = QString::fromStdString(ComponentRegistry::getComponentDescriptor(typ).name);

        auto dock = new KDDWQt::DockWidget("__analyzerDock_" + name);
        dock->setWidget(AnalysisManager::instance()->getAnalyzerWidget(typ));
        
        dock->setTitle(name);
        dock->close();
        analyzerDocks_[typ] = dock ;
    }
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
    QMenu* analyzer = menu->addMenu("Analyzers");
    
    for ( const auto& [typ, desc] : ComponentRegistry::getAllComponentDescriptors() ){
        if ( desc.numSignalInputs == 0 && desc.numSignalOutputs > 0 ){
            QAction* action = sigGen->addAction(QString::fromStdString(desc.name));
            connect(
                action, &QAction::triggered, 
                this, [this, typ](){
                    graph_->onComponentSelected(typ);
                }
            );
        } else if ( desc.numSignalInputs > 0 && desc.numSignalOutputs > 0 ){
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
        if ( desc.isAnalyzer() ){
            QAction* action = analyzer->addAction(QString::fromStdString(desc.name));
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
        SPDLOG_DEBUG(
            "component add completer did not match a component name: {}", 
            componentName.toStdString()
        );
    });

    connect(menu, &QMenu::aboutToShow, search, &QLineEdit::clear);
    return menu ;
}

void Synth::createComponentDetailDock(int componentId, ComponentParameters* params){
    if ( !params ) return ;
    if ( componentDetailDocks_.contains(componentId) ){
        SPDLOG_WARN("cannot create component detail dock: already exists for componentId {}", componentId);
        return ;
    }

    auto dock = new KDDWQt::DockWidget(QString(
        "__componentDetailDock_%1_%2").arg(
        params->getModel()->getDescriptor().name ).arg(
        componentId
    ));
    dock->setWidget(params);
    dock->setTitle(params->getModel()->getName());
    componentDetailDocks_[componentId] = dock ;

    connect( 
        params, &QObject::destroyed,
        dock, [this, componentId, dock](){
            dock->deleteLater();
            componentDetailDocks_.erase(componentId);
        }
    );
}

void Synth::closeEvent(QCloseEvent* event){
    if ( setup_ ) setup_->close() ;
    event->accept();
}

void Synth::onApiConnected(){
}

void Synth::onControlMessageReceived(const json& j){
    QString action = QString::fromStdString(j["action"]);

    if ( action == "set_state" ){
        QString state = QString::fromStdString(j["state"]);
        if ( j["status"] != "success"){
            SPDLOG_WARN("request to set state was unsuccessful.");
            return ;
        }
        if (  state == "stop" ){
            emit engineStatusChanged(false);
        } else if ( state == "run" ) {
            emit engineStatusChanged(true);
        } else {
            SPDLOG_WARN("invalid state received from set_state: {}", state.toStdString());
        }
        return ;
    }

    if ( action == "get_configuration" ){
        if ( j["status"] != "success" ){
            SPDLOG_WARN("request to get configuration data failed.");
            return ;
        }

        saveData_ = j["data"] ;
        performSave();
    }
}

void Synth::onActionSetup(){
    if ( !setup_ ){
        setup_ = new Setup() ;
         connect(
            setup_, &Setup::audioChannelsUpdated,
            graph_, &GraphPanel::onAudioChannelsUpdated
        );
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
    
    j["action"] = "get_audio_configuration" ;
    ControlApiClient::instance()->sendMessage(j);
    j.clear();

    j["action"] = "set_state" ;
    j["state"] = "run" ;
    ControlApiClient::instance()->sendMessage(j);
}

void Synth::onActionStop(){
    if ( ! StateManager::instance()->isRunning() ) return ;
    json j ;
    j["action"] = "set_state" ;
    j["state"] = "stop" ;
    ControlApiClient::instance()->sendMessage(j);
}

void Synth::onEngineStatusChange(bool status){
    StateManager::instance()->setRunning(status);
    actionStart_->setVisible(!status);
    actionStop_->setVisible(status);
    actionSetup_->setDisabled(status);
    if ( status && setup_ ){
        setup_->close();
    }
}

void Synth::onActionLoad(){
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Load Configuration"),
        QDir::homePath(),
        tr("JSON Files (*.json);;All Files (*)")
    );

    if (filePath.isEmpty()) {
        return ; 
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
    ControlApiClient::instance()->sendMessage(saveData_);
}

void Synth::onActionSave(){
    if (saveFilePath_.isEmpty()){
        onActionSaveAs();
    } else {
        json j ;
        j["action"] = "get_configuration" ;
        ControlApiClient::instance()->sendMessage(j);
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
        ControlApiClient::instance()->sendMessage(j);
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
    SPDLOG_DEBUG("configuration state saved to file {}.", file.fileName().toStdString());
    setWindowModified(false);
    windowHandle()->requestUpdate();
    return ;
}

void Synth::onActionToggleAnalyzer(ComponentType typ){
    if ( !analyzerDocks_.contains(typ) ) return ;

    auto dock = analyzerDocks_.at(typ);
    if ( dock->isOpen() ){
        dock->close();
    } else {
        dock->open();
    }
}

void Synth::onActionToggleGraph(){
    if ( graphDock_->isOpen() ){
        graphDock_->close();
    } else {
        graphDock_->open();
    }
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
    auto desc = ComponentRegistry::getComponentDescriptor(typ); 
    QString name = QString::fromStdString(desc.name);

    auto params = ComponentManager::instance()->getParameters(componentId);
    if ( params ){
        if ( params->hasDetailedEditor() ){
            createComponentDetailDock(componentId, params);
        } else {
            parameterPanel_->addContent(name, params);
            connect(params, &QObject::destroyed, this, [this, params](){
                parameterPanel_->removeContent(params);
            });
        }
    }
    
    auto modParams = ComponentManager::instance()->getModulationParameters(componentId);
    if ( modParams ){
        modulationPanel_->addContent(name, modParams);
        connect(modParams, &QObject::destroyed, this, [this, modParams](){
            modulationPanel_->removeContent(modParams);
        });
    }
}

void Synth::onComponentRemoved(int componentId){
    /* 
    parameter/modulation dock children are cleaned up off the "destroyed" signal of the 
    content widgets, so nothing needed here unless we introduce additional removal tasks
    */
}

void Synth::onShowParameters(int componentId){
    // prioritize detailed view if exists
    if ( componentDetailDocks_.contains(componentId) ){
        auto dock = componentDetailDocks_.at(componentId);
        if ( dock && dock->isHidden() ){
            dock->open();
        }
        return ;
    }

    // otherwise, show generic parameters
    auto params = ComponentManager::instance()->getParameters(componentId);
    if ( !params || !parameterPanel_->hasContent(params) ) return ;

    if ( parameterDock_->isHidden() ) parameterDock_->open() ;
    parameterPanel_->maximizeSection(params);
} 

void Synth::onShowModulation(int componentId){
    auto model = ComponentManager::instance()->getModel(componentId);
    if ( !model ) return ;
    if ( model->getDescriptor().modulatableParameters.size() == 0 ) return ;

    if ( modulationDock_->isHidden() ) modulationDock_->open() ;
    modulationPanel_->maximizeSection(ComponentManager::instance()->getModulationParameters(componentId));
}

void Synth::onShowGroupParameters(int groupId){
    auto params = GroupManager::instance()->getParameters(groupId);
    auto model = GroupManager::instance()->getModel(groupId);

    if ( !params || !model ) return ;


    if ( parameterDock_->isHidden() ) parameterDock_->open() ;
    parameterPanel_->maximizeSection(params);

    // if any member of the group has detailed view, open those as well
    for ( auto id : model->getComponents() ){
        if ( componentDetailDocks_.contains(id) ){
            auto d = componentDetailDocks_.at(id);
            if ( d && d->isHidden() ) d->open();
        }
    }
}

void Synth::onShowGroupModulation(int groupId){
    auto modParams = GroupManager::instance()->getModulationParameters(groupId);
    if ( !modParams ) return ;

    if ( modulationDock_->isHidden() ) modulationDock_->open() ;
    modulationPanel_->maximizeSection(modParams);
}

void Synth::onComponentGroupCreated(int groupId, std::unordered_set<int> componentIds){
    auto m = GroupManager::instance()->getModel(groupId);
    if ( !m ) return ;

    QString name = m->getName();

    // create group container widgets
    QWidget* paramContent = new QWidget();
    QVBoxLayout* paramLayout = new QVBoxLayout(paramContent);
    paramLayout->setContentsMargins(0,0,0,0);
    paramLayout->setSpacing(1);
    
    QWidget* modContent = new QWidget();
    QVBoxLayout* modLayout = new QVBoxLayout(modContent);
    modLayout->setContentsMargins(0,0,0,0);
    modLayout->setSpacing(1);

    // loop through parameters and move content to group container
    bool paramAdded = false, modAdded = false ;
    for ( auto componentId : componentIds ){
        auto params = ComponentManager::instance()->getParameters(componentId);
        if ( params && ! params->hasDetailedEditor() ){
            if ( !paramAdded ){
                GroupManager::instance()->setParameters(groupId, paramContent);
                parameterPanel_->addContent(name, paramContent);
                connect(paramContent, &QObject::destroyed, this, [this, paramContent](){
                    parameterPanel_->removeContent(paramContent);
                });
                paramAdded = true ;
            }
            parameterPanel_->moveContent(params, paramContent);
        }

        auto modParams = ComponentManager::instance()->getModulationParameters(componentId);
        if ( modParams ){
            if ( !modAdded ){
                GroupManager::instance()->setModulationParameters(groupId, modContent);
                modulationPanel_->addContent(name, modContent);
                connect(modContent, &QObject::destroyed, this, [this, modContent](){
                    modulationPanel_->removeContent(modContent);
                });
                modAdded = true ;
            }
            modulationPanel_->moveContent(modParams, modContent);
        }
    }

    if ( !paramAdded ){ 
        paramContent->deleteLater();
    }

    if ( !modAdded ){ 
        modContent->deleteLater();
    }
}

void Synth::onComponentGroupRemoved(int groupId, std::unordered_set<int> componentIds){
    auto m = GroupManager::instance()->getModel(groupId);
    if ( !m ) return ;

    // loop through parameters and promote all content to top-level
    for ( auto componentId : componentIds ){
        auto params = ComponentManager::instance()->getParameters(componentId);
        auto modParams = ComponentManager::instance()->getModulationParameters(componentId);
        parameterPanel_->promoteContent(params);
        modulationPanel_->promoteContent(modParams);
    }

    GroupManager::instance()->removeContent(groupId);
}

void Synth::onComponentGroupUpdated(int groupId, std::unordered_set<int> componentIds){
    auto m = GroupManager::instance()->getModel(groupId);
    if ( !m ) return ;

    // content widgets might not exist if no group elements had parameters
    QWidget* paramContent = GroupManager::instance()->getParameters(groupId);
    bool newParamContent = false ;
    if ( !paramContent ){
        newParamContent = true ;
        paramContent = new QWidget();
        QVBoxLayout* paramLayout = new QVBoxLayout(paramContent);
        paramLayout->setContentsMargins(0,0,0,0);
        paramLayout->setSpacing(1);
    }

    auto modContent = GroupManager::instance()->getModulationParameters(groupId);
    bool newModContent = false ;
    if ( !modContent ){
        newModContent = true ;
        modContent = new QWidget();
        QVBoxLayout* modLayout = new QVBoxLayout(modContent);
        modLayout->setContentsMargins(0,0,0,0);
        modLayout->setSpacing(1);
    }

    // loop through parameters and move content to group container
    bool paramAdded = false, modAdded = false ;
    for ( auto componentId : componentIds ){
        auto params = ComponentManager::instance()->getParameters(componentId);
        if ( params && ! params->hasDetailedEditor() ){
            if ( newParamContent && !paramAdded ){
                GroupManager::instance()->setParameters(groupId, paramContent);
                parameterPanel_->addContent(m->getName(), paramContent);
                connect(paramContent, &QObject::destroyed, this, [this, paramContent](){
                    parameterPanel_->removeContent(paramContent);
                });
                paramAdded = true ;
            }
            parameterPanel_->moveContent(params, paramContent);
        }

        auto modParams = ComponentManager::instance()->getModulationParameters(componentId);
        if ( modParams ){
            if ( newModContent && !modAdded ){
                GroupManager::instance()->setModulationParameters(groupId, modContent);
                modulationPanel_->addContent(m->getName(), modContent);
                connect(modContent, &QObject::destroyed, this, [this, modContent](){
                    modulationPanel_->removeContent(modContent);
                });
                modAdded = true ;
            }
            modulationPanel_->moveContent(modParams, modContent);
        }
    }

    if ( newParamContent && !paramAdded ){
        paramContent->deleteLater();
    }

    if ( newModContent && !modAdded ){
        modContent->deleteLater();
    } 
}

void Synth::onComponentRenamed(int componentId){
    auto m = ComponentManager::instance()->getModel(componentId);

    if ( !m ){
        SPDLOG_ERROR("Component Model for id {} does not exist.", componentId);
        return ;
    } 

    // rename node
    auto n = graph_->getComponentNode(componentId);
    if ( n ){
        SPDLOG_DEBUG("renaming component node...");
        n->onRename(m->getName());
    }

    // tell panels to update headers
    auto paramContent = ComponentManager::instance()->getParameters(componentId);
    if ( paramContent && parameterPanel_->hasContent(paramContent) ){
        auto paramSection = parameterPanel_->getSection(paramContent);
        if ( paramSection ) paramSection->setTitle(m->getName());
    } else {
        SPDLOG_DEBUG(
            "not updating parameter panel. hasContent={}, contentInPanel={}",
            fmt::ptr(paramContent), parameterPanel_->hasContent(paramContent)
        );
    }
    
    auto modContent = ComponentManager::instance()->getModulationParameters(componentId);
    if ( modContent ){
        auto modSection = modulationPanel_->getSection(modContent);
        if ( modSection ) modSection->setTitle(m->getName());
    } else {
        SPDLOG_DEBUG(
            "not updating modulation panel. hasContent={}, contentInPanel={}",
            fmt::ptr(modContent), modulationPanel_->hasContent(modContent)
        );
    }
    
    // if the component has a detail view, update that dock
    if ( componentDetailDocks_.contains(componentId) ){
        auto dock = componentDetailDocks_.at(componentId);
        if ( dock ) dock->setTitle(m->getName());
    }
}

void Synth::onGroupRenamed(int groupId){
    auto m = GroupManager::instance()->getModel(groupId);
    if ( !m ) return ;

    // graph node renames
    auto n = graph_->getGroupNode(groupId);
    if ( n ){
        n->onRename(m->getName());
    }

    // tell panels to update headers
    auto paramContent = GroupManager::instance()->getParameters(groupId);
    if ( paramContent ){
        auto paramSection = parameterPanel_->getSection(paramContent);
        paramSection->setTitle(m->getName());
    }
    
    auto modContent = GroupManager::instance()->getModulationParameters(groupId);
    if ( modContent ){
        auto modSection = modulationPanel_->getSection(modContent);
        modSection->setTitle(m->getName());
    }
}
