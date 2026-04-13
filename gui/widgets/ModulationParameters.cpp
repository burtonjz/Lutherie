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

#include "widgets/ModulationParameters.hpp"
#include "widgets/ModulationControl.hpp"
#include "app/Theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QTimer>

ModulationParameters::ModulationParameters(ComponentModel* model, QWidget* parent):
    QWidget(parent),
    componentId_(model->getId()),
    modulationControls_(),
    controlChangedTimer_(new QTimer(this)),
    pendingDepth_(),
    pendingStrategy_(),
    ctrlLayout_(new QGridLayout(this))
{
    auto d = model->getDescriptor();

    for ( auto [p, m] : model->getModulationModels() ){
        if ( ! hasModulationControl(p) ){
            modulationControls_[p] = addModulationControl(m);
        }
    }

    layoutControls();

    // handle changes
    controlChangedTimer_->setSingleShot(true);
    controlChangedTimer_->setInterval(300);

    connect(
        controlChangedTimer_, &QTimer::timeout, 
        this, &ModulationParameters::flushPendingChanges
    ); 
}

bool ModulationParameters::hasModulationControl(ParameterType p) const {
    return modulationControls_.contains(p);
}

ModulationControl* ModulationParameters::addModulationControl(ModulationModel* m){
    ModulationControl* ctrl = new ModulationControl(m->getType(), this);

    // pass changes from model through
    connect(
        m, &ModulationModel::modulationStrategyChanged,
        this, &ModulationParameters::onModelStrategyChanged
    );

    connect(
        m, &ModulationModel::modulationDepthChanged,
        this, &ModulationParameters::onModelDepthChanged
    );

    // send control edits through this
    connect(
        ctrl, &ModulationControl::depthEdited,
        this, &ModulationParameters::onDepthEdited
    );

    connect(
        ctrl, &ModulationControl::strategyEdited,
        this, &ModulationParameters::onStrategyEdited
    );

    return ctrl ;
}

void ModulationParameters::layoutControls(){
    // parameter widgets horizontally spaced
    ctrlLayout_->setSpacing(Theme::PARAMETER_WIDGET_SPACING);

    int count = 0 ;
    for ( auto [p, ctrl] : modulationControls_ ){
        int row = count / 1 ;
        int col = count % 1 ;
        ctrlLayout_->addWidget(ctrl, row, col);
        ++count ;
    }

    ctrlLayout_->setRowStretch(ctrlLayout_->rowCount(), 1);
    ctrlLayout_->setColumnStretch(ctrlLayout_->columnCount(), 1);

    updateGeometry();
}

void ModulationParameters::setConnectionStatus(ParameterType p, bool active){
    if ( !hasModulationControl(p) ) return ;

    modulationControls_.at(p)->setConnectionStatus(active);
}

void ModulationParameters::onDepthEdited(){
    auto w = dynamic_cast<ModulationControl*>(sender());
    if ( !w ) return ;

    ParameterType p = w->getType();
    double d = w->getDepth();
    
    pendingDepth_[p] = d ;
    controlChangedTimer_->start();
}

void ModulationParameters::onStrategyEdited(){
    auto w = dynamic_cast<ModulationControl*>(sender());
    if ( !w ) return ;

    ParameterType p = w->getType();
    ModulationStrategy s = w->getStrategy();
    
    pendingStrategy_[p] = s ;
    controlChangedTimer_->start();
}

void ModulationParameters::flushPendingChanges(){
    for ( auto [p, val] : pendingDepth_ ){
        emit modulationDepthEdited(componentId_,p,val) ;
    }

    for ( auto [p, val] : pendingStrategy_ ){
        emit modulationStrategyEdited(componentId_, p, val);
    }
    pendingDepth_.clear();
    pendingStrategy_.clear();
}

void ModulationParameters::onModelDepthChanged(int componentId, ParameterType p, double depth){
    if ( componentId != componentId_ || ! hasModulationControl(p)) return ;
    
    modulationControls_.at(p)->onModelDepthChanged(p, depth);
}

void ModulationParameters::onModelStrategyChanged(int componentId, ParameterType p, ModulationStrategy strategy){
    if ( componentId != componentId_ || ! hasModulationControl(p)) return ;
    
    modulationControls_.at(p)->onModelStrategyChanged(p, strategy);
}


