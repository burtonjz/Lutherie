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

bool ComponentParameters::hasSpecializedWidget() const {
    return specializedWidget_ != nullptr ;
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
    case ParameterType::DETUNE:
        w = new DetuneWidget(this);
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
        scroll->setWidgetResizable(true);
        scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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

    if ( specializedWidget_ ){
        layout->addWidget(specializedWidget_, 1);
    }

    QGridLayout* parameterLayout = new QGridLayout();
    parameterLayout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);

    // fix column widths
    const int COLS = Theme::PARAMETER_GRID_N_COLS ;
    for ( int c = 0 ; c < COLS ; ++c ){
        parameterLayout->setColumnMinimumWidth(c, Theme::PARAMETER_WIDGET_WIDTH);
        parameterLayout->setColumnStretch(c, 0);
    }

    // build placement list
    std::vector<ParameterWidget*> remaining ; 
    for ( auto p : model_->getDescriptor().controllableParameters ){
        remaining.push_back(parameterWidgets_[p]);
    }

    // fix widget width to span
    for ( auto* widget : remaining ){
        int span = widget->gridColumnSpan();
        widget->setFixedWidth(
            Theme::PARAMETER_WIDGET_WIDTH * span +
            (span - 1) * Theme::PARAMETER_WIDGET_SPACING
        );
    }

    // place widgets, look ahead to avoid gaps
    // TODO: this could be more robust, (e.g., find optimal placement)
    // but we might just let users position in future, so maybe not worth
    int col = 0, row = 0;
    while ( !remaining.empty() ){
        int slotsLeft = COLS - col;
        
        auto it = std::find_if(remaining.begin(), remaining.end(),
            [slotsLeft](ParameterWidget* w){
                return w->gridColumnSpan() <= slotsLeft;
            });

        if ( it == remaining.end() ){
            // nothing fits — advance to next row
            ++row;
            col = 0;
            continue;
        }

        auto* widget = *it ;
        remaining.erase(it);
        const int span = widget->gridColumnSpan();

        parameterLayout->addWidget(widget, row, col, 1, span, Qt::AlignTop);
        col += span ;

        if (col >= COLS){
            ++row ;
            col = 0 ;
        }
    }

    parameterLayout->setRowStretch(parameterLayout->rowCount(), 1);
    parameterLayout->setColumnStretch(COLS, 1);
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