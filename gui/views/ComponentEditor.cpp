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

#include "views/ComponentEditor.hpp"
#include "models/ComponentModel.hpp"

#include "app/Theme.hpp"

#include <QEvent>
#include <QCloseEvent>
#include <QBoxLayout>
#include <QScrollArea>
#include <QMainWindow>

ComponentEditor::ComponentEditor(ComponentModel* model, KDDW::MainWindow* mainWindow):
    KDDW::DockWidget(QString("dock_%1").arg(model->getId())),
    container_(new QWidget()),
    params_(new ComponentParameters(model, container_)),
    closeButton_(new QPushButton("Close",container_)),
    resetButton_(new QPushButton("Reset", container_))
{
    setOptions(
        KDDockWidgets::DockWidgetOption_None
    );

    setTitle(QString::fromStdString(model->getDescriptor().name));
    setupLayout();
    setFloating(true);
    close();
    
    connect(
        params_, &ComponentParameters::parameterEdited,
        this, &ComponentEditor::parameterEdited
    );
}

ComponentEditor::~ComponentEditor(){
    params_->deleteLater();
}

ComponentParameters* ComponentEditor::getComponentParameters() const {
    return params_ ;
}

QString ComponentEditor::getName() const {
    return title();
}

void ComponentEditor::setName(const QString& name){
    setTitle(name);
}
   
void ComponentEditor::setupLayout(){
    QVBoxLayout* mainLayout = new QVBoxLayout(container_);
    mainLayout->setContentsMargins(
        Theme::COMPONENT_DETAIL_MARGINS,
        0,
        Theme::COMPONENT_DETAIL_MARGINS,
        Theme::COMPONENT_DETAIL_MARGINS
    );

    // params
    mainLayout->addSpacing(Theme::COMPONENT_DETAIL_MARGINS);
    mainLayout->addWidget(params_);

    // buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(closeButton_);
    buttonLayout->addWidget(resetButton_);
    mainLayout->addLayout(buttonLayout);

    setWidget(container_);
    adjustSize();

    // set fixed size policy to container as long as we don't have a specialty widget that is not fixed.
    auto* w = params_->getSpecializedWidget();
    if ( !w || (
        w->sizePolicy().horizontalPolicy() == QSizePolicy::Fixed &&
        w->sizePolicy().verticalPolicy() == QSizePolicy::Fixed
    )){
        container_->setFixedSize(sizeHint());
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    connect(
        closeButton_, &QPushButton::clicked, 
        this, &ComponentEditor::onCloseButtonClicked 
    );
    connect(
        resetButton_, &QPushButton::clicked, 
        this, &ComponentEditor::onResetButtonClicked
    );
}

void ComponentEditor::onCloseButtonClicked(){
    close();
}

void ComponentEditor::onResetButtonClicked(){
    // TODO ask model to reset all parameters to default
}