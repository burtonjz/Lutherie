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

#ifndef PARAMETER_WIDGET_HPP_
#define PARAMETER_WIDGET_HPP_

#include "types/ParameterType.hpp"
#include "widgets/SwitchWidget.hpp"
#include "widgets/KnobWidget.hpp"

#include <QWidget>
#include <QLabel>
#include <QComboBox>

// base class
class ParameterWidget : public QWidget {
    Q_OBJECT

public:
    explicit ParameterWidget(QWidget* parent = nullptr);

    virtual ~ParameterWidget() = default ;

    virtual ParameterType getType() const = 0 ;
    virtual ParameterValue getValue() const = 0 ;
    virtual void setValue(const ParameterValue& value, bool block = false) = 0 ;

protected:
    void childEvent(QChildEvent* event) override ;
    
public slots:
    void onModelParameterChanged(ParameterType p, ParameterValue v);

signals:
    void valueChanged();
};

class DelayWidget : public ParameterWidget {
private:
    QLabel* label_ ;
    KnobWidget* knob_ ;
    QComboBox* unitCombo_ ;
    QLabel* valueLabel_ ;
    double sampleRate_ ;
    int minSamples_ ;
    int minMs_ ;
    int maxSamples_ ;
    int maxMs_ ;

public:
    explicit DelayWidget(QWidget* parent = nullptr);

    ParameterType getType() const override ;
    ParameterValue getValue() const override ;
    void setValue(const ParameterValue& value, bool block = false) override ;
    void setValue(size_t samples, bool block = true);

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override ;
    
private:
    void setupUI();
    void connectSignals();
    void updateDisplay();
    size_t value2sample(int val) const ;
};

class WaveformWidget : public ParameterWidget {
private:
    QLabel* label_ ;
    QComboBox* waveforms_ ;

public:
    explicit WaveformWidget(QWidget* parent = nullptr);

    ParameterType getType() const override ;
    ParameterValue getValue() const override ;
    void setValue(const ParameterValue& value, bool block = false) override ;

};

class FilterTypeWidget : public ParameterWidget {
private:
    QLabel* label_ ;
    QComboBox* type_ ;

public:
    explicit FilterTypeWidget(QWidget* parent = nullptr);

    ParameterType getType() const override ;
    ParameterValue getValue() const override ;
    void setValue(const ParameterValue& value, bool block = false) override ;

};

class MonophonicTriggerBehaviorWidget : public ParameterWidget {
private:
    QLabel* label_ ;
    QComboBox* type_ ;

public:
    explicit MonophonicTriggerBehaviorWidget(QWidget* parent = nullptr);

    ParameterType getType() const override ;
    ParameterValue getValue() const override ;
    void setValue(const ParameterValue& value, bool block = false) override ;
};

class StatusWidget : public ParameterWidget {
private:
    QLabel* label_ ;
    SwitchWidget* toggle_ ;

public:
    explicit StatusWidget(QWidget* parent = nullptr);

    ParameterType getType() const override ;
    ParameterValue getValue() const override ;
    void setValue(const ParameterValue& value, bool block = false) override ;

};

class SliderWidget : public ParameterWidget {
private:
    ParameterType param_ ;
    size_t precision_ ; // number of decimals
    QLabel* label_ ;
    KnobWidget* knob_ ;
    QLabel* valueLabel_ ;

public:
    explicit SliderWidget(ParameterType p, QWidget* parent = nullptr);

    ParameterType getType() const override ;
    ParameterValue getValue() const override ;
    void setValue(const ParameterValue& value, bool block = false) override ;

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override ;

private:
    void setupUI();
    void connectSignals();
    void updateDisplay();
    int scaleByPrecision(double v) const ;
};

class DetuneWidget : public ParameterWidget {
private:
    QLabel* harmonicLabel_ ;
    KnobWidget* harmonicKnob_ ;
    QLabel* harmonicValueLabel_ ;
    QLabel* detuneLabel_ ;
    KnobWidget* detuneKnob_ ;
    QLabel* detuneValueLabel_ ;

    int harmonicPrecision_ ;
    int detunePrecision_ ;

public:
    explicit DetuneWidget(QWidget* parent = nullptr);
    ParameterType getType() const override ;
    ParameterValue getValue() const override ;
    void setValue(const ParameterValue& value, bool block = false) override ;

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override ;
    
private:
    void setupUI();
    void connectSignals();
    void updateDisplays();
    void showEditor(QLabel* valueLabel, KnobWidget* knob, int precision);
    double harmonicValue() const;
    double detuneValue() const;
    double combinedValue() const;
    void setFromCombined(double combined, bool block = false);
    int scaleByPrecision(double v, int precision) const;
};

#endif // PARAMETER_WIDGET_HPP_