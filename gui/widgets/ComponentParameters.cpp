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
#include "widgets/AudioWaveform.hpp"
#include "widgets/PianoRollWidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QTimer>

ComponentParameters::ComponentParameters(ComponentModel* model, QWidget* parent):
    QWidget(parent),
    model_(model),
    mainLayout_(new QVBoxLayout(this)),
    paramLayout_(new QGridLayout()),
    detailedEditor_(nullptr),
    fileSelector_(nullptr)
{
    auto d = model_->getDescriptor();
    detailedEditor_ = createDetailedEditor(d.type);

    if ( d.hasFile ){
        fileSelector_ = new FileSelectorWidget();
        connect(
            fileSelector_, &FileSelectorWidget::fileSelected,
            this, [this](std::string path){
                emit fileSelected(model_->getId(), path);
            }
        );
    }

    int maxSpan = 1 ;
    for ( auto p: d.controllableParameters ){
        auto w = createParameterWidget(p);
        parameterWidgets_[p] = w ;
        maxSpan = std::max(maxSpan, w->gridColumnSpan());
    }

    // layout 
    setMinimumWidth((Theme::PARAMETER_WIDGET_WIDTH + Theme::PARAMETER_WIDGET_SPACING) * maxSpan);
    paramLayout_->setSpacing(Theme::PARAMETER_WIDGET_SPACING);
    if ( detailedEditor_ ){
        mainLayout_->addWidget(detailedEditor_, 1);
    }
    if ( fileSelector_ ){
        mainLayout_->addWidget(fileSelector_, 1);
    }

    mainLayout_->addLayout(paramLayout_);
    rebuildLayout();

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

QWidget* ComponentParameters::getDetailedEditor() const {
    return detailedEditor_ ; 
}

bool ComponentParameters::hasDetailedEditor() const {
    return detailedEditor_ != nullptr ;
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

QWidget* ComponentParameters::createDetailedEditor(ComponentType t){
    switch(t){
    case ComponentType::Sequencer:
    {
        auto* scroll = new QScrollArea() ;
        PianoRollWidget* pianoRoll = new PianoRollWidget(model_);
        scroll->setWidget(pianoRoll);
        scroll->setWidgetResizable(true);
        scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(
            model_, &ComponentModel::parameterValueChanged, 
            pianoRoll, &PianoRollWidget::onParameterChanged
        );
        return scroll ;
    }
    case ComponentType::Chopper:
    {
        // auto* scroll = new QScrollArea();
        AudioWaveformWidget* waveform = new AudioWaveformWidget(model_, 0);
        waveform->setUpstream(true);
        // scroll->setWidget(waveform);
        // scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        model_->requestBufferData(model_->getId(), 0);
        waveform->resize(800,200);

        return waveform ;
        // return scroll ;
    }
    default:
        return nullptr ;
        
    }
}

void ComponentParameters::resizeEvent(QResizeEvent* event){
    QWidget::resizeEvent(event);
    if ( hasDetailedEditor() ){
        rebuildLayout();
        updateGeometry();
    }
}

void ComponentParameters::rebuildLayout(){
    // clear out existing parameters
    while ( paramLayout_->count() ){
        paramLayout_->takeAt(0);   
    }

    // build placement list
    std::vector<ParameterWidget*> remaining ; 
    for ( auto [_, w] : parameterWidgets_ ){
        remaining.push_back(w);
    }

    // place widgets, look ahead to avoid gaps
    // TODO: this could be more robust, (e.g., find optimal placement)
    // but we might just let users position in future, so maybe not worth
    const int COLS = computeColumnCount() ;
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

        paramLayout_->addWidget(widget, row, col, 1, span, Qt::AlignTop);
        col += span ;

        if (col >= COLS){
            ++row ;
            col = 0 ;
        }
    }

    paramLayout_->setRowStretch(paramLayout_->rowCount(), 1);
    paramLayout_->setColumnStretch(COLS, 1);
}

int ComponentParameters::computeColumnCount() const {
    if ( hasDetailedEditor() ){
        return std::max(1, static_cast<int>(width() / Theme::PARAMETER_WIDGET_WIDTH) );
    } else {
        return Theme::PARAMETER_GRID_N_COLS ;
    }
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