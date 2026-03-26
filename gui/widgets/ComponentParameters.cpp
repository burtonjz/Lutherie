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

#include "widgets/ComponentParameters.hpp"
#include "app/Theme.hpp"
#include "types/ComponentType.hpp"
#include "widgets/PianoRollWidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QTimer>

ComponentParameters::ComponentParameters(ComponentModel* model, QWidget* parent):
    QWidget(parent),
    model_(model)
{
    auto d = model_->getDescriptor();
    specializedWidget_ = createSpecializedWidget(d.type);

    for ( auto p: d.controllableParameters ){
        parameterWidgets_[p] = createParameterWidget(p);
    }

    layoutParameters();

    // handle parameter changes
    parameterChangedTimer_ = new QTimer(this);
    parameterChangedTimer_->setSingleShot(true);
    parameterChangedTimer_->setInterval(300);

    connect(
        parameterChangedTimer_, &QTimer::timeout, 
        this, &ComponentParameters::flushPendingChanges
    ); 
}

ComponentModel* ComponentParameters::getModel() const {
    return model_ ;
}

QWidget* ComponentParameters::getSpecializedWidget() const {
    return specializedWidget_ ; 
}

ParameterWidget* ComponentParameters::createParameterWidget(ParameterType p){
    ParameterWidget* w ;
    switch(p){
    case ParameterType::WAVEFORM:
        w = new WaveformWidget(this);
        break ;
    case ParameterType::FILTER_TYPE:
        w = new FilterTypeWidget(this);
        break ;
    case ParameterType::TRIGGER:
        w = new MonophonicTriggerBehaviorWidget(this);
        break ;
    case ParameterType::STATUS:
        w = new StatusWidget(this);
        break ;
    case ParameterType::DELAY:
        w = new DelayWidget(this);
        break ;
    default:
        w = new SliderWidget(p, this);
        break ;
    }

    connect(
        w, &ParameterWidget::valueChanged, 
        this, &ComponentParameters::onValueChange
    );
    connect(
        model_, &ComponentModel::parameterValueChanged,
        w, &ParameterWidget::onModelParameterChanged
    );
    
    return w ;
}

QWidget* ComponentParameters::createSpecializedWidget(ComponentType t){
    switch(t){
    case ComponentType::Sequencer:
    {
        auto* scroll = new QScrollArea(this) ;
        PianoRollWidget* pianoRoll = new PianoRollWidget(model_, this);
        scroll->setWidget(pianoRoll);
        connect(
            model_, &ComponentModel::parameterValueChanged, 
            pianoRoll, &PianoRollWidget::onParameterChanged
        );
        return scroll ;
    }
    default:
        return nullptr ;
        
    }
}

void ComponentParameters::layoutParameters(){
    auto layout = new QVBoxLayout(this);

    // component-specific widget belongs at the top
    if ( specializedWidget_ ){
        layout->addWidget(specializedWidget_);
    }

    // parameter widgets horizontally spaced
    QHBoxLayout* parameterLayout = new QHBoxLayout();
    parameterLayout->setSpacing(Theme::COMPONENT_DETAIL_WIDGET_SPACING);

    for ( auto p : model_->getDescriptor().controllableParameters ){
        parameterLayout->addWidget(parameterWidgets_[p]);
    }

    layout->addLayout(parameterLayout);

    adjustSize();
}

void ComponentParameters::onValueChange(){
    auto w = dynamic_cast<ParameterWidget*>(sender());
    if ( !w ) return ;

    ParameterType p = w->getType();
    ParameterValue v = w->getValue();
    
    pendingChanges_[p] = v ;
    parameterChangedTimer_->start();
}

void ComponentParameters::flushPendingChanges(){
    if ( pendingChanges_.empty() ) return ;

    for ( auto [p, val] : pendingChanges_ ){
        emit parameterEdited(model_->getId(),p,val) ;
    }
    pendingChanges_.clear();
}