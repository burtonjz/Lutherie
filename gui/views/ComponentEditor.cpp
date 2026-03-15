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

ComponentEditor::ComponentEditor(ComponentModel* model, QWidget* parent):
    QWidget(nullptr),
    params_(new ComponentParameters(model, this)),
    name_(new QLabel(QString::fromStdString(model->getDescriptor().name), this)),
    closeButton_(new QPushButton("Close",this)),
    resetButton_(new QPushButton("Reset", this))
{
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    setWindowTitle(" ");

    setAttribute(Qt::WA_ShowWithoutActivating);
    
    setupLayout();

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
    return name_->text();
}

void ComponentEditor::setName(const QString& name){
    name_->setText(name);
}

void ComponentEditor::changeEvent(QEvent *event){
    // handle close events on focus loss
    if ( event->type() == QEvent::ActivationChange && !isActiveWindow() ){
        QWidget* active = QApplication::activeWindow();
        if ( !active ){
            return ; // ignore null new active windows -- means its a drag/resize/whatever
        }
        onCloseButtonClicked(); 
    }
    QWidget::changeEvent(event);
}
   
void ComponentEditor::setupLayout(){
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(
        Theme::COMPONENT_DETAIL_MARGINS,
        0,
        Theme::COMPONENT_DETAIL_MARGINS,
        Theme::COMPONENT_DETAIL_MARGINS
    );

    mainLayout->addWidget(name_);

    // params
    mainLayout->addSpacing(Theme::COMPONENT_DETAIL_MARGINS);
    mainLayout->addWidget(params_, 1);

    // buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(closeButton_);
    buttonLayout->addWidget(resetButton_);
    mainLayout->addLayout(buttonLayout);

    connect(
        closeButton_, &QPushButton::clicked, 
        this, &ComponentEditor::onCloseButtonClicked 
    );
    connect(
        resetButton_, &QPushButton::clicked, 
        this, &ComponentEditor::onResetButtonClicked
    );

    adjustSize();
}

void ComponentEditor::closeEvent(QCloseEvent* event){
    event->ignore();
    hide();
}

void ComponentEditor::onCloseButtonClicked(){
    hide();
}

void ComponentEditor::onResetButtonClicked(){
    // TODO ask model to reset all parameters to default
}