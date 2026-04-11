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
    label_ = new QLabel(QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, name))));
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
    label_ = new QLabel(QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::WAVEFORM, name))));
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
    label_ = new QLabel(QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::FILTER_TYPE, name))));
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
===== MONOPHONIC TRIGGER BEHAVIOR =======
=========================================
*/
MonophonicTriggerBehaviorWidget::MonophonicTriggerBehaviorWidget(QWidget* parent):
    label_(nullptr),
    type_(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::COMPONENT_DETAIL_WIDGET_SPACING);

    // label
    label_ = new QLabel(QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::TRIGGER, name))));
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
    layout->addStretch();

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
    layout->setSpacing(Theme::COMPONENT_DETAIL_WIDGET_SPACING);

    // label
    label_ = new QLabel(QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::STATUS, name))));
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
    label_ = new QLabel(QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(param_, name))));
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

/*
=========================================
================= DETUNE ================
=========================================
*/

DetuneWidget::DetuneWidget(QWidget* parent):
    ParameterWidget(parent),
    harmonicLabel_(nullptr),
    harmonicSlider_(nullptr),
    harmonicValueLabel_(nullptr),
    detuneLabel_(nullptr),
    detuneSlider_(nullptr),
    detuneValueLabel_(nullptr),
    harmonicPrecision_(ParameterTraits<ParameterType::DETUNE>::harmonicPrecision),
    detunePrecision_(ParameterTraits<ParameterType::DETUNE>::detunePrecision)
{
    setupUI();
    connectSignals();
}

ParameterType DetuneWidget::getType() const {
    return ParameterType::DETUNE;
}

double DetuneWidget::harmonicValue() const {
    return harmonicSlider_->value() * std::pow(10, -1.0 * harmonicPrecision_);
}

double DetuneWidget::detuneValue() const {
    return detuneSlider_->value() * std::pow(10, -1.0 * detunePrecision_);
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
    double harmonicMin = harmonicSlider_->minimum() * std::pow(10, -1.0 * harmonicPrecision_);
    double harmonicMax = harmonicSlider_->maximum() * std::pow(10, -1.0 * harmonicPrecision_);
    harmonicRatio = std::clamp(harmonicRatio, harmonicMin, harmonicMax);

    // remainder goes to detune
    double harmonicCents = 1200.0 * std::log2(harmonicRatio);
    double detuneCents = combined - harmonicCents ;

    // clamp detune
    double detuneMin = detuneSlider_->minimum() * std::pow(10, -1.0 * detunePrecision_);
    double detuneMax = detuneSlider_->maximum() * std::pow(10, -1.0 * detunePrecision_);
    detuneCents = std::clamp(detuneCents, detuneMin, detuneMax);
    
    QSignalBlocker hBlocker(harmonicSlider_);
    QSignalBlocker dBlocker(detuneSlider_);
    if ( !block ){
        hBlocker.unblock();
        dBlocker.unblock();
    }
    
    harmonicSlider_->setValue(scaleByPrecision(harmonicRatio, harmonicPrecision_));
    detuneSlider_->setValue(scaleByPrecision(detuneCents, detunePrecision_));

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
    layout->setSpacing(Theme::COMPONENT_DETAIL_WIDGET_SPACING);

    // harmonic ratio
    auto hLayout = new QVBoxLayout();
    
    harmonicLabel_ = new QLabel("harmonic");
    harmonicLabel_->setAlignment(Qt::AlignCenter);
    hLayout->addWidget(harmonicLabel_);

    harmonicValueLabel_ = new QLabel();
    harmonicValueLabel_->setAlignment(Qt::AlignCenter);
    hLayout->addWidget(harmonicValueLabel_);

    harmonicSlider_ = new QSlider(Qt::Horizontal);
    auto hMin = ParameterTraits<ParameterType::DETUNE>::harmonicMin ;
    harmonicSlider_->setMinimum(scaleByPrecision(hMin, harmonicPrecision_));
    auto hMax = ParameterTraits<ParameterType::DETUNE>::harmonicMax ;
    harmonicSlider_->setMaximum(scaleByPrecision(hMax, harmonicPrecision_));
    harmonicSlider_->setSingleStep(scaleByPrecision(1.0, harmonicPrecision_)); 

    hLayout->addWidget(harmonicSlider_);

    layout->addLayout(hLayout);

    // fine detune
    auto dLayout = new QVBoxLayout();

    detuneLabel_ = new QLabel("detune (cents)");
    detuneLabel_->setAlignment(Qt::AlignCenter);
    dLayout->addWidget(detuneLabel_);

    detuneValueLabel_ = new QLabel();
    detuneValueLabel_->setAlignment(Qt::AlignCenter);
    dLayout->addWidget(detuneValueLabel_);

    detuneSlider_ = new QSlider(Qt::Horizontal);
    auto dMin = ParameterTraits<ParameterType::DETUNE>::detuneCentsMin ;
    detuneSlider_->setMinimum(scaleByPrecision(dMin, detunePrecision_));
    auto dMax = ParameterTraits<ParameterType::DETUNE>::detuneCentsMax ;
    detuneSlider_->setMaximum(scaleByPrecision(dMax, detunePrecision_));
    dLayout->addWidget(detuneSlider_);

    layout->addLayout(dLayout);
    layout->addStretch();

    setFromCombined(GET_PARAMETER_TRAIT_MEMBER(ParameterType::DETUNE, defaultValue), true);
    updateDisplays();
}


void DetuneWidget::connectSignals(){
    connect(harmonicSlider_, &QSlider::valueChanged, this, [this](){
        updateDisplays();
        emit valueChanged();
    });
    connect(detuneSlider_, &QSlider::valueChanged, this, [this](){
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

void DetuneWidget::showEditor(QLabel* valueLabel, QSlider* slider, int precision){
    auto s2v = [precision](int v){ return v * std::pow(10, -1.0 * precision); };
    auto* edit = new QLineEdit(this);
    edit->setText(QString::number(s2v(slider->value())));
    edit->setGeometry(valueLabel->geometry());
    double min = s2v(slider->minimum());
    double max = s2v(slider->maximum());
    edit->setValidator(new QDoubleValidator(min, max, precision, edit));
    edit->show();
    edit->setFocus();
    edit->selectAll();
    connect(edit, &QLineEdit::editingFinished, this, [this, edit, slider, precision](){
        if ( !edit->hasAcceptableInput() ){
            edit->deleteLater();
            return;
        }
        double val = edit->text().toDouble();
        edit->deleteLater();
        slider->setValue(scaleByPrecision(val, precision));
    });
}

void DetuneWidget::mouseDoubleClickEvent(QMouseEvent* event){
    if ( harmonicValueLabel_->geometry().contains(event->pos()) ){
        showEditor(harmonicValueLabel_, harmonicSlider_, harmonicPrecision_);
    } else if ( detuneValueLabel_->geometry().contains(event->pos()) ){
        showEditor(detuneValueLabel_, detuneSlider_, detunePrecision_);
    }
}

int DetuneWidget::scaleByPrecision(double v, int precision) const {
    return static_cast<int>(v * std::pow(10, precision));
}

