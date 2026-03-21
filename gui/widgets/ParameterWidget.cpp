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

#include "widgets/ParameterWidget.hpp"
#include "config/Config.hpp"
#include "app/Theme.hpp"
#include "types/ParameterType.hpp"
#include "types/Waveform.hpp"
#include "types/FilterType.hpp"

#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QShortcut>
#include <QLineEdit>

void ParameterWidget::onModelParameterChanged(ParameterType p, ParameterValue v){
    if ( p != getType() ) return ;

    setValue(v, true);
}

/*
=========================================
================= DELAY =================
=========================================
*/
DelayWidget::DelayWidget(QWidget* parent): 
    ParameterWidget(parent),
    label_(nullptr),
    slider_(nullptr),
    unitCombo_(nullptr),
    valueLabel_(nullptr),
    minSamples_(0),
    maxSamples_(0),
    minMs_(0),
    maxMs_(0)
{
    sampleRate_ = Config::get<double>("audio.sample_rate").value();
    setupUI();
    connectSignals();
    updateDisplay();
}

ParameterType DelayWidget::getType() const {
    return ParameterType::DELAY ;
}

ParameterValue DelayWidget::getValue() const {
    QString unit = unitCombo_->currentText();

    if ( unitCombo_->currentText() == "ms") { 
            return static_cast<int>(slider_->value() / 1000.0f * sampleRate_);
    } else {
        return static_cast<int>(slider_->value());
    }
}

void DelayWidget::setValue(const ParameterValue& value, bool block){
    using ValueType = GET_PARAMETER_VALUE_TYPE(ParameterType::DELAY);

    size_t samples = std::get<ValueType>(value);

    setValue(samples, block);
}

void DelayWidget::setValue(size_t samples, bool block){
    QString unit = unitCombo_->currentText();
    
    QSignalBlocker blocker(slider_);
    if ( !block ) blocker.unblock();
    
    
    if (unit == "ms") { 
        slider_->setValue((samples / sampleRate_) * 1000.0f) ;
    } else {
        slider_->setValue(samples);
    }

    updateDisplay();
}

void DelayWidget::mouseDoubleClickEvent(QMouseEvent* event){
    auto* edit = new QLineEdit(this);
    edit->setText(QString::number(slider_->value()));
    edit->setGeometry(valueLabel_->geometry());

    // Enforce min/max based on current unit
    QString unit = unitCombo_->currentText();
    if (unit == "ms") {
        edit->setValidator(new QDoubleValidator(minMs_, maxMs_, 1, edit));
    } else {
        edit->setValidator(new QIntValidator(minSamples_, maxSamples_, edit));
    }

    edit->show();
    edit->setFocus();
    edit->selectAll();

    connect(
        edit, &QLineEdit::editingFinished,
        this, [this, edit]()
    {
        if ( !edit->hasAcceptableInput()){
            edit->deleteLater();
            return ;
        }

        QString unit = unitCombo_->currentText();
        double val = edit->text().toDouble();
        edit->deleteLater();

        if ( unit == "ms" ){
            setValue(val / 1000.0 * sampleRate_, false);
        } else {
            setValue(val, false);
        }
    });
}

void DelayWidget::setupUI(){
    // default setup is in samples
    // can toggle to time in ms
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::COMPONENT_DETAIL_WIDGET_SPACING);
    
    // widget label
    label_ = new QLabel(QString::fromStdString(GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, name)));
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);

    // value label
    valueLabel_ = new QLabel("0 samples");
    valueLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(valueLabel_);
    
    // value slider
    minSamples_ = GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, minimum);
    minMs_ = minSamples_ / sampleRate_ * 1000 ;

    maxSamples_ = GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, maximum);
    maxMs_ = maxSamples_ / sampleRate_ * 1000 ;

    slider_ = new QSlider(Qt::Horizontal);
    slider_->setMinimum(minSamples_);
    slider_->setMaximum(maxSamples_);  
    slider_->setValue(0);
    slider_->setEnabled(true);
    layout->addWidget(slider_);
    
    // unit selector
    QWidget* unitContainer = new QWidget();
    QHBoxLayout* unitLayout = new QHBoxLayout(unitContainer);
    unitLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* unitLabel = new QLabel("Unit:");
    unitCombo_ = new QComboBox();
    unitCombo_->addItem("samples");
    unitCombo_->addItem("ms");
    
    unitLayout->addWidget(unitLabel);
    unitLayout->addWidget(unitCombo_);
    unitLayout->addStretch();
    
    layout->addWidget(unitContainer);
    layout->addStretch();
}

void DelayWidget::connectSignals(){
    connect(slider_, &QSlider::valueChanged, this, [this](){
        updateDisplay();
        emit valueChanged();
    });

    // changing units updates the slider but doesn't send updates
    connect(unitCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, [this](int index) {
        size_t samples ; 
        QString oldUnit = (index == 0) ? "ms" : "samples"; 
        QString newUnit = unitCombo_->currentText();
        double sampleRate = Config::get<double>("audio.sample_rate").value();

        if (oldUnit == "ms") { 
            samples = slider_->value() / 1000.0f * sampleRate ;
        } else {
            samples = slider_->value();
        }

        // Update slider for new unit
        if (newUnit == "ms") {
            slider_->setMaximum(maxMs_); 
            slider_->setMinimum(minMs_);
            setValue(samples);
        } else {
            slider_->setMinimum(minSamples_);
            slider_->setMaximum(maxSamples_);
            slider_->setValue(samples);  
        }
    });
}

void DelayWidget::updateDisplay(){
    QString unit = unitCombo_->currentText();
    int val = slider_->value();

    if (unit == "ms") {
        valueLabel_->setText(QString::number(val, 'f', 1) + " ms");
    } else {
        if ( val == 1 ){
            valueLabel_->setText(QString::number(val) + " sample");
        } else {
            valueLabel_->setText(QString::number(val) + " samples");
        }
        
    }
}

/*
=========================================
================ WAVEFORM ===============
=========================================
*/
WaveformWidget::WaveformWidget(QWidget* parent):
    label_(nullptr),
    waveforms_(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::COMPONENT_DETAIL_WIDGET_SPACING);

    // label
    label_ = new QLabel(QString::fromStdString(GET_PARAMETER_TRAIT_MEMBER(ParameterType::WAVEFORM, name)));
    layout->addWidget(label_);

    // populate waveforms
    waveforms_ = new QComboBox(this);
    for ( auto wf : Waveform::getWaveforms()){
        Waveform wave = Waveform(wf);
        waveforms_->addItem(QString::fromStdString(wave.toString()),wave.to_uint8());
    }

    // set default waveform
    int idx = waveforms_->findData(GET_PARAMETER_TRAIT_MEMBER(ParameterType::WAVEFORM, defaultValue));
    if ( idx != -1 ){
        waveforms_->setCurrentIndex(idx);
    } 
    layout->addWidget(waveforms_);
    layout->addStretch();

    // connections
    connect(waveforms_, &QComboBox::currentIndexChanged, this, &ParameterWidget::valueChanged);

}

ParameterType WaveformWidget::getType() const {
    return ParameterType::WAVEFORM ;
}

ParameterValue WaveformWidget::getValue() const {
    return waveforms_->itemData(waveforms_->currentIndex()).value<uint8_t>();
}

void WaveformWidget::setValue(const ParameterValue& value, bool block){
    using ValueType = GET_PARAMETER_VALUE_TYPE(ParameterType::WAVEFORM);
    uint8_t wf = std::get<ValueType>(value);

    QSignalBlocker blocker(waveforms_);
    if ( !block ) blocker.unblock();

    int idx = waveforms_->findData(wf);

    if ( idx != -1 ){
        waveforms_->setCurrentIndex(idx);
    } else {
        qWarning() << "could not set waveform value, enum not found in data" ;
        return ;
    }
}

/*
=========================================
=============== FILTER TYPE =============
=========================================
*/
FilterTypeWidget::FilterTypeWidget(QWidget* parent):
    label_(nullptr),
    type_(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::COMPONENT_DETAIL_WIDGET_SPACING);

    // label
    label_ = new QLabel(QString::fromStdString(GET_PARAMETER_TRAIT_MEMBER(ParameterType::FILTER_TYPE, name)));
    layout->addWidget(label_);

    // populate types
    type_ = new QComboBox(this);
    for ( auto ft : FilterType::getFilterTypes()){
        FilterType f = FilterType(ft);
        type_->addItem(QString::fromStdString(f.toString()),f.to_uint8());
    }

    // set default type
    int idx = type_->findData(GET_PARAMETER_TRAIT_MEMBER(ParameterType::FILTER_TYPE, defaultValue));
    if ( idx != -1 ){
        type_->setCurrentIndex(idx);
    } 

    layout->addWidget(type_);
    layout->addStretch();

    // connections
    connect(type_, &QComboBox::currentIndexChanged, this, &ParameterWidget::valueChanged);
}

ParameterType FilterTypeWidget::getType() const {
    return ParameterType::FILTER_TYPE ;
}

ParameterValue FilterTypeWidget::getValue() const {
    return type_->itemData(type_->currentIndex()).value<uint8_t>();
}

void FilterTypeWidget::setValue(const ParameterValue& value, bool block){
    using ValueType = GET_PARAMETER_VALUE_TYPE(ParameterType::FILTER_TYPE);

    QSignalBlocker blocker(type_);
    if ( !block ) blocker.unblock();

    uint8_t ft = std::get<ValueType>(value);

    int idx = type_->findData(ft);

    if ( idx != -1 ){
        type_->setCurrentIndex(idx);
    } else {
        qWarning() << "could not set filter type value, enum not found in data" ;
        return ;
    }
}

/*
=========================================
================== STATUS ===============
=========================================
*/
StatusWidget::StatusWidget(QWidget* parent):
    label_(nullptr),
    toggle_(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::COMPONENT_DETAIL_WIDGET_SPACING);

    // label
    label_ = new QLabel(QString::fromStdString(GET_PARAMETER_TRAIT_MEMBER(ParameterType::STATUS, name)));
    layout->addWidget(label_);
    layout->addStretch();

    // create status toggle
    toggle_ = new SwitchWidget(this);
    toggle_->setChecked(true);
    layout->addWidget(toggle_);

    // connections
    connect(toggle_, &QAbstractButton::toggled, this, &ParameterWidget::valueChanged);
}

ParameterType StatusWidget::getType() const {
    return ParameterType::STATUS ;
}

ParameterValue StatusWidget::getValue() const {
    return toggle_->isChecked();
}

void StatusWidget::setValue(const ParameterValue& value, bool block){
    using ValueType = GET_PARAMETER_VALUE_TYPE(ParameterType::STATUS);
    auto status = std::get<ValueType>(value);

    QSignalBlocker blocker(toggle_);
    if ( !block ) blocker.unblock();

    toggle_->setChecked(status);
}

/*
=========================================
================= SLIDER =================
=========================================
*/
SliderWidget::SliderWidget(ParameterType p, QWidget* parent): 
    ParameterWidget(parent),
    param_(p),
    label_(nullptr),
    slider_(nullptr),
    valueLabel_(nullptr)
{
    precision_ = GET_PARAMETER_TRAIT_MEMBER(p, uiPrecision);
    setupUI();
    connectSignals();
}

ParameterType SliderWidget::getType() const {
    return param_ ;
}

ParameterValue SliderWidget::getValue() const {
    double v = slider_->value() * std::pow(10, -1.0 * precision_);
    switch(param_){
        #define X(name) \
            case ParameterType::name: \
                return ParameterValue{ \
                    static_cast<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>(v) \
                };
            PARAMETER_TYPE_LIST
        #undef X
        default:
            throw std::runtime_error("Parameter of type " +
                GET_PARAMETER_TRAIT_MEMBER(param_, name) +
                " is not in enum. This shouldn't happen.") ;
    }
}

void SliderWidget::setValue(const ParameterValue& value, bool block){
    // dispatch for value type, multiplying by precision to properly convert to int slider
    QSignalBlocker blocker(slider_);
    if ( !block ) blocker.unblock();
    
    switch(param_){
        #define X(name) \
            case ParameterType::name: \
                slider_->setValue( \
                    scaleByPrecision(std::get<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>(value)) \
                ); \
                break ; 
            PARAMETER_TYPE_LIST
        #undef X
        default:
            qWarning() << "Parameter of type " << 
                GET_PARAMETER_TRAIT_MEMBER(param_, name) 
                << " is not in enum. This shouldn't happen." ;
            break ;
    }

    if ( block ) updateDisplay();
}

void SliderWidget::mouseDoubleClickEvent(QMouseEvent* event){
    auto s2v = [this](int v){ return v * std::pow(10, -1.0 * precision_);};

    auto* edit = new QLineEdit(this);
    edit->setText(QString::number(s2v(slider_->value())));
    edit->setGeometry(valueLabel_->geometry());

    double min = s2v(slider_->minimum());
    double max = s2v(slider_->maximum());
    edit->setValidator(new QDoubleValidator(min, max, precision_, edit));
    
    edit->show();
    edit->setFocus();
    edit->selectAll();

    connect(
        edit, &QLineEdit::editingFinished,
        this, [this, edit]()
    {
        if ( !edit->hasAcceptableInput()){
            edit->deleteLater();
            return ;
        }

        double val = edit->text().toDouble();
        edit->deleteLater();

        switch(param_){
            #define X(name) \
                case ParameterType::name: \
                    setValue(static_cast<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>(val)); \
                    break ;
                PARAMETER_TYPE_LIST
            #undef X
        default:
            qWarning() << "Parameter of type " <<
                GET_PARAMETER_TRAIT_MEMBER(param_, name) <<
                " is not in enum. This shouldn't happen." ;
            break ;
        }
    });
}

void SliderWidget::setupUI(){
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::COMPONENT_DETAIL_WIDGET_SPACING);
    
    // widget label
    label_ = new QLabel(QString::fromStdString(GET_PARAMETER_TRAIT_MEMBER(param_, name)));
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);

    // value label
    valueLabel_ = new QLabel();
    valueLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(valueLabel_);
    
    // value slider
    slider_ = new QSlider(Qt::Horizontal);
    slider_->setMinimum(scaleByPrecision(GET_PARAMETER_TRAIT_MEMBER(param_, minimum)));

    if ( param_ == ParameterType::FREQUENCY ){
        // nyquist override
        slider_->setMaximum(scaleByPrecision(Config::get<double>("audio.sample_rate").value() / 2.0)); 
    } else {
        slider_->setMaximum(scaleByPrecision(GET_PARAMETER_TRAIT_MEMBER(param_,maximum)));  
    }
    
    slider_->setEnabled(true);
    layout->addWidget(slider_);
    layout->addStretch();

    // initialize to default value via dispatch
    switch(param_){
        #define X(name) \
            case ParameterType::name: \
                setValue(static_cast<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>( \
                    GET_PARAMETER_TRAIT_MEMBER(ParameterType::name, defaultValue))); \
                break ;
        PARAMETER_TYPE_LIST
        #undef X
        default:
            qWarning() << "Parameter of type " << 
                GET_PARAMETER_TRAIT_MEMBER(param_, name) 
                << " is not in enum. This shouldn't happen." ;
            slider_->setValue(0);
    }

    updateDisplay();
}

void SliderWidget::connectSignals(){
    connect(slider_, &QSlider::valueChanged, this, [this](){
        updateDisplay();
        emit valueChanged();
    });
}

void SliderWidget::updateDisplay(){
    valueLabel_->setText(QString::number(slider_->value() * std::pow(10, -1.0 * precision_)));
}

int SliderWidget::scaleByPrecision(double v) const {
    return v * std::pow(10, precision_);
}