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

#include "views/GroupEditor.hpp"
#include "models/ComponentModel.hpp"
#include "app/Theme.hpp"

#include <QScrollArea>
#include <QCloseEvent>

GroupEditor::GroupEditor(const QString& name, KDDWQt::MainWindow* mainWindow):
    KDDWQt::DockWidget(name),
    container_(new QWidget()),
    params_(),
    paramsLayout_(new QVBoxLayout(container_)),
    closeButton_(new QPushButton("Close",container_))
{
    setOptions(
        KDDockWidgets::DockWidgetOption_None
    );

    setTitle(name);
    setupLayout();
    setFloating(true);
    close();

    setWidget(container_);
}

QString GroupEditor::getName() const {
    return title();
}

void GroupEditor::setName(const QString& name){
    setTitle(name);
}

void GroupEditor::addComponent(ComponentModel* model){
    if ( !model ) return ;
    int id = model->getId();

    if ( params_.contains(id) ) return ;
    int count = params_.size() ;

    params_[id] = new ComponentParameters(model, container_);

    paramsLayout_->addWidget(params_[id]);

    connect(
        params_[id], &ComponentParameters::parameterEdited,
        this, &GroupEditor::parameterEdited
    );
}

void GroupEditor::removeComponent(ComponentModel* model){
    if ( !model ) return ;
    int id = model->getId();

    if ( !params_.contains(id) ) return ;
    
    paramsLayout_->removeWidget(params_.at(id));
    params_.at(id)->deleteLater();
    params_.erase(id);
    
    // handle grid layout (fill in gap of removed component)
    relayoutParams();
}

ComponentParameters* GroupEditor::getComponentParameters(int componentId){
    if ( !params_.contains(componentId) ) return nullptr ;
    return params_.at(componentId);
}

void GroupEditor::setupLayout(){
    QVBoxLayout* mainLayout = new QVBoxLayout(container_);
    mainLayout->setContentsMargins(
        Theme::PARAMETER_WIDGET_MARGINS,
        0,
        Theme::PARAMETER_WIDGET_MARGINS,
        Theme::PARAMETER_WIDGET_MARGINS
    );

    // parameters
    auto gridContainer = new QWidget();
    gridContainer->setLayout(paramsLayout_);
    auto scroll = new QScrollArea(container_);
    scroll->setWidget(gridContainer);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    mainLayout->addWidget(scroll, 1);

    // buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(closeButton_);
    mainLayout->addLayout(buttonLayout);

    connect(
        closeButton_, &QPushButton::clicked, 
        this, &GroupEditor::onCloseButtonClicked 
    );

}

void GroupEditor::relayoutParams(){
    for ( auto [id, p]: params_ ){
        paramsLayout_->removeWidget(p);
    }

    for ( auto [id, p]: params_ ){
        paramsLayout_->addWidget(p);
    }

    adjustSize();
    setFixedSize(sizeHint());
}

void GroupEditor::onCloseButtonClicked(){
    close();
}
