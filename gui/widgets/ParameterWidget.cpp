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
#include "widgets/WheelGuard.hpp"

#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QShortcut>
#include <QLineEdit>


ParameterWidget::ParameterWidget(QWidget* parent):
    QWidget(parent)
{}

int ParameterWidget::gridColumnSpan() const {
    return 1 ;
}

void ParameterWidget::onModelParameterChanged(ParameterType p, ParameterValue v){
    if ( p != getType() ) return ;

    setValue(v, true);
}

void ParameterWidget::childEvent(QChildEvent* event){
    if ( event->added() ){
        auto* w = dynamic_cast<QWidget*>(event->child());
        if ( w ){
            w->installEventFilter(new WheelGuard(this));
            w->setFocusPolicy(Qt::ClickFocus);
        }
    }
    QWidget::childEvent(event);
}

/*
=========================================
================= DELAY =================
=========================================
*/
DelayWidget::DelayWidget(QWidget* parent): 
    ParameterWidget(parent),
    label_(nullptr),
    knob_(nullptr),
    unitToggle_(new QToolButton()),
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
    QString unit = unitToggle_->text();

    if ( unitToggle_->text() == "ms") { 
            return static_cast<int>(knob_->value() / 1000.0f * sampleRate_);
    } else {
        return static_cast<int>(knob_->value());
    }
}

void DelayWidget::setValue(const ParameterValue& value, bool block){
    using ValueType = GET_PARAMETER_VALUE_TYPE(ParameterType::DELAY);

    size_t samples = std::get<ValueType>(value);

    setValue(samples, block);
}

void DelayWidget::setValue(size_t samples, bool block){
    QString unit = unitToggle_->text();
    
    QSignalBlocker blocker(knob_);
    if ( !block ) blocker.unblock();
    
    
    if (unit == "ms") { 
        knob_->setValue((samples / sampleRate_) * 1000.0f) ;
    } else {
        knob_->setValue(samples);
    }

    updateDisplay();
}

void DelayWidget::mouseDoubleClickEvent(QMouseEvent* event){
    auto* edit = new QLineEdit(this);
    edit->setAlignment(Qt::AlignCenter);
    edit->setText(QString::number(knob_->value()));
    edit->setGeometry(valueLabel_->geometry());

    // Enforce min/max based on current unit
    QString unit = unitToggle_->text();
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

        QString unit = unitToggle_->text();
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
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);
    setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);

    // widget label
    label_ = new QLabel(QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, name))));
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);

    // value label    
    valueLabel_ = new QLabel("0");
    QFontMetrics fm(valueLabel_->fontMetrics());
    valueLabel_->setMinimumWidth(fm.horizontalAdvance("0000"));
    valueLabel_->setAlignment(Qt::AlignRight);

    layout->addWidget(valueLabel_);
    
    // unit selector
    unitToggle_->setAttribute(Qt::WA_Hover, true);
    unitToggle_->setText("samples");
    unitToggle_->setCheckable(true);
    unitToggle_->setChecked(false);
    unitToggle_->setAutoRaise(true);
    unitToggle_->setToolTip("Toggle between samples and milliseconds");
    unitToggle_->setObjectName("unitToggle");

    QHBoxLayout* valueRow = new QHBoxLayout();
    valueRow->setContentsMargins(0,0,0,0);
    valueRow->addStretch();
    valueRow->addWidget(valueLabel_);
    valueRow->addWidget(unitToggle_);
    valueRow->addStretch();
    layout->addLayout(valueRow);

    // value slider
    minSamples_ = GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, minimum);
    minMs_ = minSamples_ / sampleRate_ * 1000 ;

    maxSamples_ = GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, maximum);
    maxMs_ = maxSamples_ / sampleRate_ * 1000 ;

    knob_ = new KnobWidget(Qt::Horizontal);
    knob_->setMinimum(minSamples_);
    knob_->setMaximum(maxSamples_);  
    knob_->setValue(0);
    knob_->setEnabled(true);

    QHBoxLayout* knobRow = new QHBoxLayout();
    knobRow->setContentsMargins(0,0,0,0);
    knobRow->addStretch();
    knobRow->addWidget(knob_);
    knobRow->addStretch();
    layout->addLayout(knobRow);
    
}

void DelayWidget::connectSignals(){
    connect(knob_, &QSlider::valueChanged, this, [this](){
        updateDisplay();
        emit valueChanged();
    });

    // changing units updates the slider but doesn't send updates
    connect(unitToggle_, &QToolButton::toggled,
        this, [this](bool ms){
        unitToggle_->setText(ms ? "ms" : "samples");
        
        // convert current knob value to samples regardless of old unit
        double sampleRate = Config::get<double>("audio.sample_rate").value();
        size_t samples = ms 
            ? static_cast<size_t>(knob_->value())  
            : static_cast<size_t>(knob_->value() / 1000.0 * sampleRate);                      

        // update knob range and restore value in new unit
        if ( ms ){
            knob_->setMinimum(minMs_);
            knob_->setMaximum(maxMs_);
            knob_->setValue(samples / sampleRate * 1000.0);
        } else {
            knob_->setMinimum(minSamples_);
            knob_->setMaximum(maxSamples_);
            knob_->setValue(samples);
        }
        updateDisplay();
    });
}

void DelayWidget::updateDisplay(){
    QString unit = unitToggle_->text();
    int val = knob_->value();

    if (unit == "ms") {
        valueLabel_->setText(QString::number(val, 'f', 1));
    } else {
        valueLabel_->setText(QString::number(val));
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
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);
    setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);

    // label
    label_ = new QLabel(QString::fromStdString(
        std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::WAVEFORM, name))
    ));
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);

    // waveforms
    waveforms_ = new QComboBox(this);
    for ( auto wf : Waveform::getWaveforms()){
        Waveform wave = Waveform(wf);
        waveforms_->addItem(QString::fromStdString(wave.toString()),wave.to_uint8());
    }
    int idx = waveforms_->findData(GET_PARAMETER_TRAIT_MEMBER(ParameterType::WAVEFORM, defaultValue));
    if ( idx != -1 ){
        waveforms_->setCurrentIndex(idx);
    } 
    layout->addWidget(waveforms_);

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
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);
    setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);

    // label
    label_ = new QLabel(QString::fromStdString(
        std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::FILTER_TYPE, name))
    ));
    label_->setAlignment(Qt::AlignCenter);
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
===== MONOPHONIC TRIGGER BEHAVIOR =======
=========================================
*/
MonophonicTriggerBehaviorWidget::MonophonicTriggerBehaviorWidget(QWidget* parent):
    label_(nullptr),
    type_(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);
    setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);

    // label
    label_ = new QLabel(QString::fromStdString(
        std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::TRIGGER, name))
    ));
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);

    // populate types
    type_ = new QComboBox(this);
    for ( auto b : MonophonicTriggerBehavior::getBehaviors()){
        MonophonicTriggerBehavior behavior = MonophonicTriggerBehavior(b);
        type_->addItem(QString::fromStdString(behavior.toString()),behavior.to_uint8());
    }

    // set default type
    int idx = type_->findData(GET_PARAMETER_TRAIT_MEMBER(ParameterType::TRIGGER, defaultValue));
    if ( idx != -1 ){
        type_->setCurrentIndex(idx);
    } 

    layout->addWidget(type_);

    // connections
    connect(type_, &QComboBox::currentIndexChanged, this, &ParameterWidget::valueChanged);
}

ParameterType MonophonicTriggerBehaviorWidget::getType() const {
    return ParameterType::TRIGGER ;
}

ParameterValue MonophonicTriggerBehaviorWidget::getValue() const {
    return type_->itemData(type_->currentIndex()).value<uint8_t>();
}

void MonophonicTriggerBehaviorWidget::setValue(const ParameterValue& value, bool block){
    using ValueType = GET_PARAMETER_VALUE_TYPE(ParameterType::TRIGGER);

    QSignalBlocker blocker(type_);
    if ( !block ) blocker.unblock();

    uint8_t behavior = std::get<ValueType>(value);

    int idx = type_->findData(behavior);

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
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);
    setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);

    // label
    label_ = new QLabel(QString::fromStdString(
        std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::STATUS, name))
    ));
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);

    // create status toggle
    toggle_ = new SwitchWidget(this);
    toggle_->setChecked(true);
    QHBoxLayout* toggleRow = new QHBoxLayout();
    toggleRow->addStretch();
    toggleRow->addWidget(toggle_);
    toggleRow->addStretch();
    layout->addLayout(toggleRow);
 
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
    knob_(nullptr),
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
    double v = knob_->value() * std::pow(10, -1.0 * precision_);
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
    QSignalBlocker blocker(knob_);
    if ( !block ) blocker.unblock();
    
    switch(param_){
        #define X(name) \
            case ParameterType::name: \
                knob_->setValue( \
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
    edit->setText(QString::number(s2v(knob_->value())));
    edit->setGeometry(valueLabel_->geometry());

    double min = s2v(knob_->minimum());
    double max = s2v(knob_->maximum());
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
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);
    setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);
    
    // widget label
    label_ = new QLabel(QString::fromStdString(
        std::string(GET_PARAMETER_TRAIT_MEMBER(param_, name))
    ));
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);

    // value label
    valueLabel_ = new QLabel();
    valueLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(valueLabel_);
    
    // value knob
    knob_ = new KnobWidget();
    knob_->setMinimum(scaleByPrecision(GET_PARAMETER_TRAIT_MEMBER(param_, minimum)));

    if ( param_ == ParameterType::FREQUENCY ){ // nyquist override
        knob_->setMaximum(scaleByPrecision(Config::get<double>("audio.sample_rate").value() / 2.0)); 
    } else {
        knob_->setMaximum(scaleByPrecision(GET_PARAMETER_TRAIT_MEMBER(param_,maximum)));  
    }
    
    knob_->setEnabled(true);
    QHBoxLayout* knobRow = new QHBoxLayout();
    knobRow->setContentsMargins(0,0,0,0);
    knobRow->addStretch();
    knobRow->addWidget(knob_);
    knobRow->addStretch();
    layout->addLayout(knobRow);

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
            knob_->setValue(0);
    }

    updateDisplay();
}

void SliderWidget::connectSignals(){
    connect(knob_, &QSlider::valueChanged, this, [this](){
        updateDisplay();
        emit valueChanged();
    });
}

void SliderWidget::updateDisplay(){
    valueLabel_->setText(QString::number(knob_->value() * std::pow(10, -1.0 * precision_)));
}

int SliderWidget::scaleByPrecision(double v) const {
    return v * std::pow(10, precision_);
}

/*
=========================================
================= DETUNE ================
=========================================
*/

DetuneWidget::DetuneWidget(QWidget* parent):
    ParameterWidget(parent),
    harmonicLabel_(nullptr),
    harmonicKnob_(nullptr),
    harmonicValueLabel_(nullptr),
    detuneLabel_(nullptr),
    detuneKnob_(nullptr),
    detuneValueLabel_(nullptr),
    harmonicPrecision_(ParameterTraits<ParameterType::DETUNE>::harmonicPrecision),
    detunePrecision_(ParameterTraits<ParameterType::DETUNE>::detunePrecision)
{
    setupUI();
    connectSignals();
}

int DetuneWidget::gridColumnSpan() const {
    return 2 ;
}

ParameterType DetuneWidget::getType() const {
    return ParameterType::DETUNE;
}

double DetuneWidget::harmonicValue() const {
    return harmonicKnob_->value() * std::pow(10, -1.0 * harmonicPrecision_);
}

double DetuneWidget::detuneValue() const {
    return detuneKnob_->value() * std::pow(10, -1.0 * detunePrecision_);
}

double DetuneWidget::combinedValue() const {
    double harmonicCents = 1200.0 * std::log2(harmonicValue());
    return harmonicCents + detuneValue();
}

void DetuneWidget::setFromCombined(double combined, bool block){
    // if the sliders already produce this value
    if ( std::abs(combinedValue() - combined) < 0.5 ){
        updateDisplays();
        return ;
    }

    // otherwise, decompose by finding nearest harmonic ratio (remainder in cents)
    double totalRatio = std::pow(2.0, combined / 1200.0);
    double scale = std::pow(10, harmonicPrecision_);
    double harmonicRatio = std::round(totalRatio * scale) / scale ;

    // clamp harmonic
    double harmonicMin = harmonicKnob_->minimum() * std::pow(10, -1.0 * harmonicPrecision_);
    double harmonicMax = harmonicKnob_->maximum() * std::pow(10, -1.0 * harmonicPrecision_);
    harmonicRatio = std::clamp(harmonicRatio, harmonicMin, harmonicMax);

    // remainder goes to detune
    double harmonicCents = 1200.0 * std::log2(harmonicRatio);
    double detuneCents = combined - harmonicCents ;

    // clamp detune
    double detuneMin = detuneKnob_->minimum() * std::pow(10, -1.0 * detunePrecision_);
    double detuneMax = detuneKnob_->maximum() * std::pow(10, -1.0 * detunePrecision_);
    detuneCents = std::clamp(detuneCents, detuneMin, detuneMax);
    
    QSignalBlocker hBlocker(harmonicKnob_);
    QSignalBlocker dBlocker(detuneKnob_);
    if ( !block ){
        hBlocker.unblock();
        dBlocker.unblock();
    }
    
    harmonicKnob_->setValue(scaleByPrecision(harmonicRatio, harmonicPrecision_));
    detuneKnob_->setValue(scaleByPrecision(detuneCents, detunePrecision_));

    updateDisplays();
}

ParameterValue DetuneWidget::getValue() const {
    return static_cast<double>(combinedValue());
}

void DetuneWidget::setValue(const ParameterValue& value, bool block){
    double combined = std::get<double>(value);
    setFromCombined(combined, block);
}

void DetuneWidget::setupUI(){
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);

    // harmonic ratio
    auto* hWidget = new QWidget();
    hWidget->setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);
    auto hLayout = new QVBoxLayout(hWidget);

    harmonicLabel_ = new QLabel("harmonic");
    harmonicLabel_->setAlignment(Qt::AlignCenter);
    hLayout->addWidget(harmonicLabel_);

    harmonicValueLabel_ = new QLabel();
    harmonicValueLabel_->setAlignment(Qt::AlignCenter);
    hLayout->addWidget(harmonicValueLabel_);

    harmonicKnob_ = new KnobWidget(Qt::Horizontal);
    auto hMin = ParameterTraits<ParameterType::DETUNE>::harmonicMin ;
    harmonicKnob_->setMinimum(scaleByPrecision(hMin, harmonicPrecision_));
    auto hMax = ParameterTraits<ParameterType::DETUNE>::harmonicMax ;
    harmonicKnob_->setMaximum(scaleByPrecision(hMax, harmonicPrecision_));
    harmonicKnob_->setSingleStep(scaleByPrecision(1.0, harmonicPrecision_)); 

    QHBoxLayout* hKnobRow = new QHBoxLayout();
    hKnobRow->setContentsMargins(0,0,0,0);
    hKnobRow->addStretch();
    hKnobRow->addWidget(harmonicKnob_);
    hKnobRow->addStretch();
    hLayout->addLayout(hKnobRow);
    layout->addWidget(hWidget);

    // fine detune
    auto dWidget = new QWidget();
    dWidget->setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);
    auto dLayout = new QVBoxLayout(dWidget);

    detuneLabel_ = new QLabel("detune (cents)");
    detuneLabel_->setAlignment(Qt::AlignCenter);
    dLayout->addWidget(detuneLabel_);

    detuneValueLabel_ = new QLabel();
    detuneValueLabel_->setAlignment(Qt::AlignCenter);
    dLayout->addWidget(detuneValueLabel_);

    detuneKnob_ = new KnobWidget(Qt::Horizontal);
    auto dMin = ParameterTraits<ParameterType::DETUNE>::detuneCentsMin ;
    detuneKnob_->setMinimum(scaleByPrecision(dMin, detunePrecision_));
    auto dMax = ParameterTraits<ParameterType::DETUNE>::detuneCentsMax ;
    detuneKnob_->setMaximum(scaleByPrecision(dMax, detunePrecision_));
    QHBoxLayout* dKnobRow = new QHBoxLayout();
    dKnobRow->setContentsMargins(0,0,0,0);
    dKnobRow->addStretch();
    dKnobRow->addWidget(detuneKnob_);
    dKnobRow->addStretch();
    dLayout->addLayout(dKnobRow);
    layout->addWidget(dWidget);

    setFromCombined(GET_PARAMETER_TRAIT_MEMBER(ParameterType::DETUNE, defaultValue), true);
    updateDisplays();
}


void DetuneWidget::connectSignals(){
    connect(harmonicKnob_, &QSlider::valueChanged, this, [this](){
        updateDisplays();
        emit valueChanged();
    });
    connect(detuneKnob_, &QSlider::valueChanged, this, [this](){
        updateDisplays();
        emit valueChanged();
    });
}

void DetuneWidget::updateDisplays(){
    harmonicValueLabel_->setText(
        QString::number(harmonicValue(), 'f', harmonicPrecision_)
    );
    detuneValueLabel_->setText(
        QString::number(detuneValue(), 'f', detunePrecision_) + " ct"
    );
}

void DetuneWidget::showEditor(QLabel* valueLabel, KnobWidget* knob, int precision){
    auto s2v = [precision](int v){ return v * std::pow(10, -1.0 * precision); };
    
    auto* edit = new QLineEdit(this);
    edit->setAlignment(Qt::AlignCenter);
    edit->setText(QString::number(s2v(knob->value())));

    QRect labelRect(valueLabel->mapTo(this, QPoint(0, 0)), valueLabel->size());
    edit->setGeometry(labelRect);

    double min = s2v(knob->minimum());
    double max = s2v(knob->maximum());
    edit->setValidator(new QDoubleValidator(min, max, precision, edit));
    edit->show();
    edit->setFocus();
    edit->selectAll();
    connect(edit, &QLineEdit::editingFinished, this, [this, edit, knob, precision](){
        if ( !edit->hasAcceptableInput() ){
            edit->deleteLater();
            return;
        }
        double val = edit->text().toDouble();
        edit->deleteLater();
        knob->setValue(scaleByPrecision(val, precision));
    });
}

void DetuneWidget::mouseDoubleClickEvent(QMouseEvent* event){
    auto toLocal = [this](QWidget* w) -> QRect {
        return QRect(w->mapTo(this, QPoint(0,0)), w->size());
    };

    QRect harmonic = toLocal(harmonicValueLabel_);
    QRect detune   = toLocal(detuneValueLabel_);

    if ( harmonic.contains(event->pos()) ){
        showEditor(harmonicValueLabel_, harmonicKnob_, harmonicPrecision_);
    } else if ( detune.contains(event->pos()) ){
        showEditor(detuneValueLabel_, detuneKnob_, detunePrecision_);
    }
}

int DetuneWidget::scaleByPrecision(double v, int precision) const {
    return static_cast<int>(v * std::pow(10, precision));
}

