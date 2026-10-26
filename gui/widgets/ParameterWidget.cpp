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
#include "units/decibel.hpp"
#include "widgets/MidiLearnWidget.hpp"
#include "widgets/RangeSlider.hpp"
#include "widgets/WheelGuard.hpp"

#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QShortcut>
#include <QLineEdit>
#include <QMenu>
#include <QWidgetAction>
#include <spdlog/spdlog.h>


ParameterWidget::ParameterWidget(QWidget* parent):
    QWidget(parent)
{}

int ParameterWidget::gridColumnSpan() const {
    return 1 ;
}

std::pair<double, double> ParameterWidget::getRange() const {
    return {
        GET_PARAMETER_TRAIT_MEMBER(getType(), minimum),
        GET_PARAMETER_TRAIT_MEMBER(getType(), maximum)
    };
}

void ParameterWidget::setRange([[maybe_unused]] const ParameterValue& min, [[maybe_unused]] const ParameterValue& max, [[maybe_unused]] bool block){
}

void ParameterWidget::onModelParameterChanged(ParameterType p, ParameterValue v){
    if ( p != getType() ) return ;

    setValue(v, true);
}

void ParameterWidget::onModelRangeChanged(ParameterType p, ParameterValue min, ParameterValue max){
    if ( p != getType() ) return ;

    setRange(min,max, true);
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

void ParameterWidget::contextMenuEvent(QContextMenuEvent *event){
    QMenu menu ;

    // adjust min/max parameter range
    if ( GET_PARAMETER_TRAIT_MEMBER(getType(), supportRangeUpdate) ){
        QMenu* editRange = menu.addMenu("Edit Parameter Range");
        QWidget* rangeWidget = createRangeEditor(editRange);
        QWidgetAction* rangeAction = new QWidgetAction(editRange);
        rangeAction->setDefaultWidget(rangeWidget);
        editRange->addAction(rangeAction);
    }
    
    QMenu* editMidi = menu.addMenu("Edit MIDI Control");
    QWidget* midiWidget = createMidiLearn(editMidi);
    QWidgetAction* midiAction = new QWidgetAction(editMidi);
    midiAction->setDefaultWidget(midiWidget);
    editMidi->addAction(midiAction);

    connect(editMidi, &QMenu::aboutToHide, &menu, &QMenu::close);
    
    menu.exec(event->globalPos());
    event->accept();
}

QWidget* ParameterWidget::createRangeEditor(QWidget* parent){
    auto* popup = new QWidget(parent);
    popup->setAttribute(Qt::WA_DeleteOnClose);

    auto* layout = new QVBoxLayout(popup);
    layout->setContentsMargins(8,8,8,8);

    double min = GET_PARAMETER_TRAIT_MEMBER(getType(), minimum);
    double max = GET_PARAMETER_TRAIT_MEMBER(getType(), maximum);
    if ( getType() == ParameterType::FREQUENCY ){
        max = Config::get<double>("audio.sample_rate").value() / 2.0 ;
    }

    const auto [curMin, curMax] = getRange();
    
    RangeSlider* slider = new RangeSlider(
        min, max, curMin, curMax,
        GET_PARAMETER_TRAIT_MEMBER(getType(), uiPrecision),
        popup
    );

    connect(slider, &RangeSlider::rangeUpdated, this, [this](double min, double max){
        switch(getType()){
            #define X(name)                                                       \
                case ParameterType::name:                                         \
                    emit rangeChanged(                                            \
                        ParameterType::name,                                      \
                        ParameterValue{static_cast<                               \
                            GET_PARAMETER_VALUE_TYPE(ParameterType::name)>(min)}, \
                        ParameterValue{static_cast<                               \
                            GET_PARAMETER_VALUE_TYPE(ParameterType::name)>(max)}  \
                    );                                                            \
                    break ;
            PARAMETER_TYPE_LIST
            #undef X
            default:
                SPDLOG_WARN("invalid parameter specified: {}.", 
                    GET_PARAMETER_TRAIT_MEMBER(getType(), name));
                break ;
        }
    });
    
    layout->addWidget(slider);
    popup->adjustSize();
    
    return popup ;
}

QWidget* ParameterWidget::createMidiLearn(QMenu* parent){
    auto popup = new MidiLearnWidget(parent);

    connect(
        popup, &MidiLearnWidget::midiControlSelected,
        this, [this](uint8_t ctrl){
            emit midiControlSelected(getType(), ctrl);
        }
    );

    connect(
        popup, &MidiLearnWidget::midiControlSelected,
        parent, &QMenu::close
    );

    return popup ;
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
    sampleRate_(Config::get<double>("audio.sample_rate").value()),
    minSamples_(0),
    minMs_(0),
    maxSamples_(0),
    maxMs_(0)
{
    setupUI();
    connectSignals();
    updateDisplay();
}

ParameterType DelayWidget::getType() const {
    return ParameterType::DELAY ;
}

ParameterValue DelayWidget::getValue() const {
    if ( unitToggle_->isChecked() ){ 
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
    QSignalBlocker blocker(knob_);
    if ( !block ) blocker.unblock();
    
    if ( unitToggle_->isChecked() ){ 
        knob_->setValue((samples / sampleRate_) * 1000.0f);
    } else {
        knob_->setValue(samples);
    }

    updateDisplay();
}

std::pair<double, double> DelayWidget::getRange() const {
    return { minSamples_, maxSamples_ };
}

void DelayWidget::setRange(const ParameterValue& min, const ParameterValue& max, bool block){
    minSamples_ = std::get<int>(min) ;
    minMs_ = minSamples_ / sampleRate_ * 1000 ;

    maxSamples_ = std::get<int>(max) ;
    maxMs_ = maxSamples_ / sampleRate_ * 1000 ;

    QSignalBlocker blocker(knob_);
    if ( !block ) blocker.unblock();

    if ( unitToggle_->isChecked() ){
        knob_->setMinimum(minMs_);
        knob_->setMaximum(maxMs_);
    } else {
        knob_->setMinimum(minSamples_);
        knob_->setMaximum(maxSamples_);
    }
}

void DelayWidget::mouseDoubleClickEvent([[maybe_unused]] QMouseEvent* event){
    auto* edit = new QLineEdit(this);
    edit->setAlignment(Qt::AlignCenter);
    edit->setText(QString::number(knob_->value()));
    edit->setGeometry(valueLabel_->geometry());

    // Enforce min/max based on current unit
    if ( unitToggle_->isChecked() ){
        edit->setValidator(new QDoubleValidator(minMs_, maxMs_, 1, edit));
    } else {
        edit->setValidator(new QIntValidator(minSamples_, maxSamples_, edit));
    }

    edit->show();
    edit->setFocus();
    edit->selectAll();

    connect(
        edit, &QLineEdit::editingFinished,
        this, [this, edit](){
        if ( !edit->hasAcceptableInput() ){
            edit->deleteLater();
            return ;
        }
        
        double val = edit->text().toDouble();
        edit->deleteLater();

        if ( unitToggle_->isChecked() ){
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
    valueLabel_->setMinimumWidth(fm.horizontalAdvance("00000000"));
    valueLabel_->setAlignment(Qt::AlignRight);

    layout->addWidget(valueLabel_);
    
    // unit selector (unchecked = samples, checked = ms)
    unitToggle_->setAttribute(Qt::WA_Hover, true);
    unitToggle_->setText("samples");
    unitToggle_->setCheckable(true);
    unitToggle_->setChecked(false);
    unitToggle_->setAutoRaise(true);
    unitToggle_->setToolTip("Toggle between samples and milliseconds");
    unitToggle_->setProperty("textToggleBtn", true);

    QHBoxLayout* valueRow = new QHBoxLayout();
    valueRow->setContentsMargins(0,0,0,0);
    valueRow->addStretch();
    valueRow->addWidget(valueLabel_);
    valueRow->addWidget(unitToggle_);
    valueRow->addStretch();
    layout->addLayout(valueRow);

    // knob
    minSamples_ = GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, minimum);
    minMs_ = minSamples_ / sampleRate_ * 1000 ;

    maxSamples_ = GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, maximum);
    maxMs_ = maxSamples_ / sampleRate_ * 1000 ;

    knob_ = new KnobWidget(Qt::Horizontal);
    knob_->setPrecision(precisionSample_);
    knob_->setMinimum(minSamples_);
    knob_->setMaximum(maxSamples_);  
    knob_->setValue(GET_PARAMETER_TRAIT_MEMBER(ParameterType::DELAY, defaultValue));
    knob_->setEnabled(true);

    QHBoxLayout* knobRow = new QHBoxLayout();
    knobRow->setContentsMargins(0,0,0,0);
    knobRow->addStretch();
    knobRow->addWidget(knob_);
    knobRow->addStretch();
    layout->addLayout(knobRow);
}

void DelayWidget::connectSignals(){
    connect(knob_, &KnobWidget::valueChanged, this, [this](){
        updateDisplay();
        emit valueChanged();
    });

    // changing units updates the slider but doesn't send updates
    connect(unitToggle_, &QToolButton::toggled,
        this, [this](bool ms){
        const double sampleRate = Config::get<double>("audio.sample_rate").value();        
        
        const int min = ms ? minMs_ : minSamples_ ;
        const int max = ms ? maxMs_ : maxSamples_ ;
        
        const int precision = ms ? precisionMs_ : precisionSample_ ;
        const double val = ms 
            ? (knob_->value() / sampleRate * 1000.0)
            : (knob_->value() / 1000.0 * sampleRate);

        unitToggle_->setText(ms ? "ms" : "samples");
        knob_->setPrecision(precision);
        knob_->setRange(min, max);
        knob_->setValue(val, true);
        
        updateDisplay();
    });
}

void DelayWidget::updateDisplay(){
    const double val = knob_->value();
    const int precision = unitToggle_->isChecked() ? precisionMs_ : precisionSample_ ;
    valueLabel_->setText(QString::number(val, 'f', precision));
}

/*
=========================================
================ WAVEFORM ===============
=========================================
*/
WaveformWidget::WaveformWidget(QWidget* parent):
    ParameterWidget(parent),
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
    for ( auto wf : Waveform::getNames()){
        Waveform w = Waveform::fromString(wf);
        waveforms_->addItem(QString::fromStdString(std::string(wf)), w.to_uint8());
    }
    int idx = waveforms_->findData(GET_PARAMETER_TRAIT_MEMBER(ParameterType::WAVEFORM, defaultValue));
    if ( idx != -1 ){
        waveforms_->setCurrentIndex(idx);
    } 
    layout->addWidget(waveforms_);

    // connections
    connect(
        waveforms_, &QComboBox::currentIndexChanged, 
        this, &ParameterWidget::valueChanged
    );

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
        SPDLOG_WARN("could not set waveform value, enum not found in data");
        return ;
    }
}

/*
=========================================
=============== FILTER TYPE =============
=========================================
*/
FilterTypeWidget::FilterTypeWidget(QWidget* parent):
    ParameterWidget(parent),
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
    for ( auto ft : FilterType::getNames()){
        FilterType f = FilterType::fromString(ft);
        type_->addItem(QString::fromStdString(std::string(ft)),f.to_uint8());
    }

    // set default type
    int idx = type_->findData(GET_PARAMETER_TRAIT_MEMBER(ParameterType::FILTER_TYPE, defaultValue));
    if ( idx != -1 ){
        type_->setCurrentIndex(idx);
    } 

    layout->addWidget(type_);

    // connections
    connect(
        type_, &QComboBox::currentIndexChanged, 
        this, &ParameterWidget::valueChanged
        );
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
        SPDLOG_WARN("could not set filter type value, enum not found in data");
        return ;
    }
}

/*
=========================================
===== MONOPHONIC TRIGGER BEHAVIOR =======
=========================================
*/
MonophonicTriggerTypeWidget::MonophonicTriggerTypeWidget(QWidget* parent):
    ParameterWidget(parent),
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
    for ( auto b : MonophonicTriggerType::getNames()){
        MonophonicTriggerType typ = MonophonicTriggerType::fromString(b);
        type_->addItem(QString::fromStdString(std::string(b)),typ.to_uint8());
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

ParameterType MonophonicTriggerTypeWidget::getType() const {
    return ParameterType::TRIGGER ;
}

ParameterValue MonophonicTriggerTypeWidget::getValue() const {
    return type_->itemData(type_->currentIndex()).value<uint8_t>();
}

void MonophonicTriggerTypeWidget::setValue(const ParameterValue& value, bool block){
    using ValueType = GET_PARAMETER_VALUE_TYPE(ParameterType::TRIGGER);

    QSignalBlocker blocker(type_);
    if ( !block ) blocker.unblock();

    uint8_t behavior = std::get<ValueType>(value);

    int idx = type_->findData(behavior);

    if ( idx != -1 ){
        type_->setCurrentIndex(idx);
    } else {
        SPDLOG_WARN("could not set filter type value, enum not found in data");
        return ;
    }
}

/*
=========================================
================== STATUS ===============
=========================================
*/
BoolWidget::BoolWidget(ParameterType p, QWidget* parent):
    ParameterWidget(parent),
    label_(nullptr),
    toggle_(nullptr),
    param_(p)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);
    setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);

    // label
    label_ = new QLabel(QString::fromStdString(
        std::string(GET_PARAMETER_TRAIT_MEMBER(p, name))
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

ParameterType BoolWidget::getType() const {
    return param_ ;
}

ParameterValue BoolWidget::getValue() const {
    return toggle_->isChecked();
}

void BoolWidget::setValue(const ParameterValue& value, bool block){
    bool status = std::get<bool>(value);

    QSignalBlocker blocker(toggle_);
    if ( !block ) blocker.unblock();

    toggle_->setChecked(status);
}

/*
=========================================
================= RECORD ================
=========================================
*/
RecordWidget::RecordWidget(QWidget* parent):
    ParameterWidget(parent),
    btn_(new QPushButton(this)),
    recordIcon_(makeRecordStopIcon(true)),
    stopIcon_(makeRecordStopIcon(false))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // create status toggle
    btn_->setFixedSize(
        Theme::RECORD_BUTTON_ICON_WIDTH * 1.5, 
        Theme::RECORD_BUTTON_ICON_HEIGHT * 1.5
    );
    btn_->setCheckable(true);
    btn_->setChecked(false);
    btn_->setIcon(recordIcon_);
    btn_->setIconSize(QSize(
        Theme::RECORD_BUTTON_ICON_WIDTH,
        Theme::RECORD_BUTTON_ICON_HEIGHT
    ));

    layout->addWidget(btn_);
 
    // connections
    connect(btn_, &QPushButton::toggled, this, [this](bool checked){
        btn_->setIcon(checked ? stopIcon_ : recordIcon_);
        btn_->setToolTip(checked ? "Stop Recording" : "Start Recording");
        emit ParameterWidget::valueChanged();
    });
}

ParameterType RecordWidget::getType() const {
    return ParameterType::RECORD ;
}

ParameterValue RecordWidget::getValue() const {
    return btn_->isChecked();
}

void RecordWidget::setValue(const ParameterValue& value, bool block){
    bool checked = std::get<bool>(value);

    QSignalBlocker blocker(btn_);
    if ( !block ) blocker.unblock();

    btn_->setChecked(checked);

    // just sanity check to keep icon in sync even if signal blocked.
    if ( block ){
        btn_->setIcon(checked ? stopIcon_ : recordIcon_);
        btn_->setToolTip(checked ? "Stop Recording" : "Start Recording");
    }
}

QIcon RecordWidget::makeRecordStopIcon(bool record){
    QPixmap pixmap(
        Theme::RECORD_BUTTON_ICON_WIDTH,
        Theme::RECORD_BUTTON_ICON_HEIGHT
    );
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    float marginX = Theme::RECORD_BUTTON_ICON_WIDTH  / 6.0 ;
    float marginY = Theme::RECORD_BUTTON_ICON_HEIGHT / 6.0 ;
    QRectF rect(
        marginX, marginY,
        Theme::RECORD_BUTTON_ICON_WIDTH - 2 * marginX,
        Theme::RECORD_BUTTON_ICON_HEIGHT - 2 * marginY
    );

    if ( record ){
        painter.setBrush(Theme::RECORD_ICON_RECORDING_COLOR);
        painter.drawEllipse(rect);
    } else {
        painter.setBrush(Theme::RECORD_ICON_STOP_COLOR);
        painter.drawRoundedRect(rect, 2, 2);
    }

    return QIcon(pixmap);
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
    setupUI();
    connectSignals();
}

ParameterType SliderWidget::getType() const {
    return param_ ;
}

ParameterValue SliderWidget::getValue() const {
    const double v = knob_->value();
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
                knob_->setValue(std::get<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>(value)); \
                break ; 
            PARAMETER_TYPE_LIST
        #undef X
        default:
            SPDLOG_WARN("Parameter of type {} is not in enum. This is a programming bug",
                GET_PARAMETER_TRAIT_MEMBER(param_, name));
            break ;
    }

    if ( block ) updateDisplay();
}

std::pair<double, double> SliderWidget::getRange() const {
    return { knob_->minimum(), knob_->maximum() };
}

void SliderWidget::setRange(const ParameterValue& min, const ParameterValue& max, bool block){
    double newMin = std::visit([](auto&& val){
        return static_cast<double>(val);
    }, min) ;
    double newMax = std::visit([](auto&& val){
        return static_cast<double>(val);
    }, max);

    if ( param_ == ParameterType::FREQUENCY ){
        double nyquist = Config::get<double>("audio.sample_rate").value() / 2.0 ;
        newMin = std::clamp(newMin, 0.0, nyquist);
        newMax = std::clamp(newMax, 0.0, nyquist);
    }
    
    QSignalBlocker blocker(knob_);
    if ( !block ) blocker.unblock();

    knob_->setMinimum(newMin);
    knob_->setMaximum(newMax);

    if ( block ) updateDisplay();
}

void SliderWidget::mouseDoubleClickEvent([[maybe_unused]] QMouseEvent* event){
    auto* edit = new QLineEdit(this);
    edit->setText(QString::number(knob_->value()));
    edit->setGeometry(valueLabel_->geometry());

    const double min = knob_->minimum();
    const double max = knob_->maximum();
    edit->setValidator(new QDoubleValidator(min, max, knob_->precision(), edit));
    
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

        const double val = edit->text().toDouble();
        edit->deleteLater();

        switch(param_){
            #define X(name) \
                case ParameterType::name: \
                    setValue(static_cast<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>(val)); \
                    break ;
                PARAMETER_TYPE_LIST
            #undef X
        default:
            SPDLOG_WARN("Parameter of type {} is not in enum. This is a programming bug",
                GET_PARAMETER_TRAIT_MEMBER(param_, name));
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
    knob_->setPrecision(GET_PARAMETER_TRAIT_MEMBER(param_, uiPrecision));
    knob_->setMinimum(GET_PARAMETER_TRAIT_MEMBER(param_, minimum));

    if ( param_ == ParameterType::FREQUENCY ){ // nyquist override
        knob_->setMaximum(Config::get<double>("audio.sample_rate").value() / 2.0); 
    } else {
        knob_->setMaximum(GET_PARAMETER_TRAIT_MEMBER(param_,maximum));  
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
            SPDLOG_WARN("Parameter of type {} is not in enum. This is a programming bug",
                GET_PARAMETER_TRAIT_MEMBER(param_, name));
            knob_->setValue(0);
    }

    updateDisplay();
}

void SliderWidget::connectSignals(){
    connect(knob_, &KnobWidget::valueChanged, this, [this](){
        updateDisplay();
        emit valueChanged();
    });
}

void SliderWidget::updateDisplay(){
    valueLabel_->setText(QString::number(knob_->value() * std::pow(10, -1.0 * knob_->precision())));
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

double DetuneWidget::combinedValue() const {
    double harmonicCents = 1200.0 * std::log2(harmonicKnob_->value());
    return harmonicCents + detuneKnob_->value();
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
    double harmonicMin = harmonicKnob_->minimum();
    double harmonicMax = harmonicKnob_->maximum();
    harmonicRatio = std::clamp(harmonicRatio, harmonicMin, harmonicMax);

    // remainder goes to detune
    double harmonicCents = 1200.0 * std::log2(harmonicRatio);
    double detuneCents = combined - harmonicCents ;

    // clamp detune
    double detuneMin = detuneKnob_->minimum();
    double detuneMax = detuneKnob_->maximum();
    detuneCents = std::clamp(detuneCents, detuneMin, detuneMax);
    
    QSignalBlocker hBlocker(harmonicKnob_);
    QSignalBlocker dBlocker(detuneKnob_);
    if ( !block ){
        hBlocker.unblock();
        dBlocker.unblock();
    }
    
    harmonicKnob_->setValue(harmonicRatio);
    detuneKnob_->setValue(detuneCents);

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

    harmonicKnob_ = new KnobWidget();
    harmonicKnob_->setPrecision(harmonicPrecision_);
    harmonicKnob_->setMinimum(ParameterTraits<ParameterType::DETUNE>::harmonicMin);
    harmonicKnob_->setMaximum(ParameterTraits<ParameterType::DETUNE>::harmonicMax);
    harmonicKnob_->setSingleStep(1.0); 

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

    detuneKnob_ = new KnobWidget();
    detuneKnob_->setPrecision(detunePrecision_);
    detuneKnob_->setMinimum(ParameterTraits<ParameterType::DETUNE>::detuneCentsMin);
    detuneKnob_->setMaximum(ParameterTraits<ParameterType::DETUNE>::detuneCentsMax);
    detuneKnob_->setSingleStep(1.0);

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
    connect(harmonicKnob_, &KnobWidget::valueChanged, this, [this](){
        updateDisplays();
        emit valueChanged();
    });
    connect(detuneKnob_, &KnobWidget::valueChanged, this, [this](){
        updateDisplays();
        emit valueChanged();
    });
}

void DetuneWidget::updateDisplays(){
    harmonicValueLabel_->setText(QString::number(harmonicKnob_->value()));
    detuneValueLabel_->setText(
        QString::number(detuneKnob_->value(), 'f', detunePrecision_) + " ct"
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
    connect(edit, &QLineEdit::editingFinished, this, [edit, knob](){
        if ( !edit->hasAcceptableInput() ){
            edit->deleteLater();
            return ;
        }
        double val = edit->text().toDouble();
        edit->deleteLater();
        knob->setValue(val);
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

GainWidget::GainWidget(QWidget* parent):
    ParameterWidget(parent),
    label_(nullptr),
    knob_(nullptr),
    unitToggle_(new QToolButton()),
    valueLabel_(nullptr),
    linear_(GET_PARAMETER_TRAIT_MEMBER(ParameterType::GAIN, defaultValue)),
    minLinear_(GET_PARAMETER_TRAIT_MEMBER(ParameterType::GAIN, minimum)),
    maxLinear_(GET_PARAMETER_TRAIT_MEMBER(ParameterType::GAIN, maximum)),
    minDb_(0),
    maxDb_(0)
{
    setupUI();
    connectSignals();
    updateDisplay();
}
 
ParameterType GainWidget::getType() const {
    return ParameterType::GAIN ;
}
 
ParameterValue GainWidget::getValue() const {
    return linear_ ;
}
 
void GainWidget::setValue(const ParameterValue& value, bool block){
    using ValueType = GET_PARAMETER_VALUE_TYPE(ParameterType::GAIN);
    ValueType v = std::get<ValueType>(value);
    setValue(v, block);
}

void GainWidget::setValue(double linear, bool block){
    QSignalBlocker blocker(knob_);
    if ( !block ) blocker.unblock();

    linear_ = linear ;
    if ( unitToggle_->isChecked() ){
        knob_->setValue(decibel::fromLinear(linear_));
    } else {
        knob_->setValue(linear_);
    }

    updateDisplay();
}

std::pair<double, double> GainWidget::getRange() const {
    return { minLinear_, maxLinear_ };
}
 
void GainWidget::setRange(const ParameterValue& min, const ParameterValue& max, [[maybe_unused]] bool block){
    using ValueType = GET_PARAMETER_VALUE_TYPE(ParameterType::GAIN);
    minLinear_ = std::get<ValueType>(min);
    maxLinear_ = std::get<ValueType>(max);

    minDb_ = decibel::fromLinear(minLinear_);
    maxDb_ = decibel::fromLinear(maxLinear_);

    SPDLOG_DEBUG("db: min={} max={}. linear: min={} max={}", minDb_, maxDb_, minLinear_, maxLinear_);
    QSignalBlocker blocker(knob_);
    if ( !block ) blocker.unblock();

    if ( unitToggle_->isChecked() ){
        knob_->setMinimum(minDb_);
        knob_->setMaximum(maxDb_);
    } else {
        knob_->setMinimum(minLinear_);
        knob_->setMaximum(maxLinear_);
    }
}
 
void GainWidget::mouseDoubleClickEvent([[maybe_unused]] QMouseEvent* event){
    const bool db = unitToggle_->isChecked();
    const int precision = db ? 1 : 3 ;

    const double min = db ? minDb_ : minLinear_ ;
    const double max = db ? maxDb_ : maxLinear_ ;
    const double val = knob_->value();

    auto* edit = new QLineEdit(this);
    edit->setAlignment(Qt::AlignCenter);
    edit->setText(QString::number(val));
    edit->setGeometry(valueLabel_->geometry());
    edit->setValidator(new QDoubleValidator(min, max, precision, edit));
    
    edit->show();
    edit->setFocus();
    edit->selectAll();

    connect(
        edit, &QLineEdit::editingFinished,
        this, [this, edit](){
        if ( !edit->hasAcceptableInput() ){
            edit->deleteLater();
            return ;
        }

        double val = edit->text().toDouble();
        edit->deleteLater();

        linear_ = unitToggle_->isChecked()
            ? decibel::toLinear(val)
            : val ;
        setValue(linear_, false);
    });
}
 
void GainWidget::setupUI(){
    // default setup is in linear
    // can toggle to decibel
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);
    setFixedWidth(Theme::PARAMETER_WIDGET_WIDTH);
 
    // widget label
    label_ = new QLabel(QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(ParameterType::GAIN, name))));
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);
 
    // value label
    valueLabel_ = new QLabel("0");
    QFontMetrics fm(valueLabel_->fontMetrics());
    valueLabel_->setMinimumWidth(fm.horizontalAdvance("-00.000"));
    valueLabel_->setAlignment(Qt::AlignRight);
    layout->addWidget(valueLabel_);
 
    // unit selector (unchecked = linear, checked = dB)
    unitToggle_->setAttribute(Qt::WA_Hover, true);
    unitToggle_->setText("linear");
    unitToggle_->setCheckable(true);
    unitToggle_->setChecked(false);
    unitToggle_->setAutoRaise(true);
    unitToggle_->setToolTip("Toggle between linear gain and decibels");
    unitToggle_->setProperty("textToggleBtn", true);
 
    QHBoxLayout* valueRow = new QHBoxLayout();
    valueRow->setContentsMargins(0, 0, 0, 0);
    valueRow->addStretch();
    valueRow->addWidget(valueLabel_);
    valueRow->addWidget(unitToggle_);
    valueRow->addStretch();
    layout->addLayout(valueRow);
 
    // knob
    minLinear_ = GET_PARAMETER_TRAIT_MEMBER(ParameterType::GAIN, minimum);
    minDb_ = decibel::fromLinear(minLinear_);

    maxLinear_ = GET_PARAMETER_TRAIT_MEMBER(ParameterType::GAIN, maximum);
    maxDb_ = decibel::fromLinear(maxLinear_);

    knob_ = new KnobWidget();
    knob_->setPrecision(precisionLinear_);
    knob_->setMinimum(minLinear_);
    knob_->setMaximum(maxLinear_);
    knob_->setValue(linear_);
    knob_->setEnabled(true);
 
    QHBoxLayout* knobRow = new QHBoxLayout();
    knobRow->setContentsMargins(0, 0, 0, 0);
    knobRow->addStretch();
    knobRow->addWidget(knob_);
    knobRow->addStretch();
    layout->addLayout(knobRow);
}
 
void GainWidget::connectSignals(){
    connect(knob_, &KnobWidget::valueChanged, this, [this](){
        linear_ = unitToggle_->isChecked() 
            ? decibel::toLinear(knob_->value())
            : knob_->value();
        updateDisplay();
        emit valueChanged();
    });
 
    // changing units re-maps the knob but never changes the value or sends updates
    connect(unitToggle_, &QToolButton::toggled, this, [this](bool db){
        const double min = db ? minDb_ : minLinear_ ;
        const double max = db ? maxDb_ : maxLinear_ ;
        const int precision = db ? precisionDb_ : precisionLinear_ ;
        const double val = db
            ? decibel::fromLinear(linear_)
            : linear_ ;
        
        unitToggle_->setText(db ? "dB" : "linear");
        knob_->setPrecision(precision);
        knob_->setRange(min, max);
        knob_->setValue(val, true);

        updateDisplay();
    });
}

void GainWidget::updateDisplay(){
    const double val = knob_->value();
    const int precision = unitToggle_->isChecked() ? precisionDb_ : precisionLinear_ ;
    valueLabel_->setText(QString::number(val, 'f', precision));
}