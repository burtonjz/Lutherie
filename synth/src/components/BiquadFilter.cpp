#include "BiquadFilter.hpp"
#include "core/BaseComponent.hpp"
#include "params/ParameterMap.hpp"
#include "types/FilterType.hpp"
#include "types/ParameterType.hpp"
#include <cmath>

BiquadFilter::BiquadFilter(ComponentId id, BiquadFilterConfig cfg):
    BaseComponent(id, ComponentType::BiquadFilter),
    BaseModule(1,1),
    BaseModulator(),
    state1_(0.0),
    state2_(0.0),
    dirty_(false)
{
    parameters_->add<ParameterType::FILTER_TYPE>(cfg.filterType, false);
    parameters_->add<ParameterType::FREQUENCY>(cfg.frequency, true, 
        20, sampleRate_ * 0.495); // limit to nyquist
    parameters_->add<ParameterType::DBGAIN>(cfg.gain, true);
    parameters_->add<ParameterType::Q_FACTOR>(cfg.qFactor,true);
    parameters_->add<ParameterType::BANDWIDTH>(cfg.bandwidth,true);
    parameters_->add<ParameterType::SHELF>(cfg.shelfSlope,true);

    parameters_->getParameter(ParameterType::FILTER_TYPE)->addListener(this);
    parameters_->getParameter(ParameterType::FREQUENCY)->addListener(this);
    parameters_->getParameter(ParameterType::DBGAIN)->addListener(this);
    parameters_->getParameter(ParameterType::Q_FACTOR)->addListener(this);
    parameters_->getParameter(ParameterType::BANDWIDTH)->addListener(this);
    parameters_->getParameter(ParameterType::SHELF)->addListener(this);

    calculateCoefficients(); // prime the filter
}

// see https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
void BiquadFilter::calculateCoefficients(){
    double b0, b1, b2, a0, a1, a2 ;
    double w0, alpha, sin_w0, cos_w0, A, S, Q ;

    w0 = 2.0 * M_PI * parameters_->getParameter<ParameterType::FREQUENCY>()->getInstantaneousValue() / sampleRate_ ;
    cos_w0 = std::cos(w0);
    sin_w0 = std::sin(w0);

    switch(FilterType::from_uint8(parameters_->getParameter<ParameterType::FILTER_TYPE>()->getValue())){
    case FilterType::LowPass:
        Q = parameters_->getParameter<ParameterType::Q_FACTOR>()->getInstantaneousValue();
        alpha = sin_w0 / ( 2.0 * Q );
        b0 = (1.0 - cos_w0) / 2.0 ; // b0
        b1 = 1.0 - cos_w0 ; // b1
        b2 = b0 ; // b2
        a0 = 1.0 + alpha ; // a0
        a1 = -2.0 * cos_w0 ; // a1
        a2 = 1.0 - alpha ; // a2
        break ;
    case FilterType::HighPass:
        Q = parameters_->getParameter<ParameterType::Q_FACTOR>()->getInstantaneousValue();
        alpha = sin_w0 / ( 2.0 * Q);
        b0 = (1.0 + cos_w0) / 2.0 ;
        b1 = -1.0 - cos_w0 ;
        b2 = b0 ;
        a0 = 1.0 + alpha ;
        a1 = -2.0 * cos_w0 ;
        a2 = 1.0 - alpha ;
        break ;
    case FilterType::BandPass:
        Q = parameters_->getParameter<ParameterType::Q_FACTOR>()->getInstantaneousValue();
        alpha = sin_w0 / ( 2.0 * Q);
        b0 = alpha ;
        b1 = 0.0 ;
        b2 = -b0 ;
        a0 = 1.0 + alpha ;
        a1 = -2.0 * cos_w0 ;
        a2 = 1.0 - alpha ;
        break ;
    case FilterType::BandStop:
        Q = parameters_->getParameter<ParameterType::Q_FACTOR>()->getInstantaneousValue();
        alpha = sin_w0 / (2.0 * Q);
        b0 = 1.0;
        b1 = -2.0 * cos_w0;
        b2 = 1.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;
    case FilterType::PeakingBell:
        Q = parameters_->getParameter<ParameterType::Q_FACTOR>()->getInstantaneousValue();
        A = std::pow(10.0, parameters_->getParameter<ParameterType::DBGAIN>()->getInstantaneousValue() / 40.0);
        alpha = sin_w0 / (2.0 * Q);
        b0 = 1.0 + alpha * A;
        b1 = -2.0 * cos_w0;
        b2 = 1.0 - alpha * A;
        a0 = 1.0 + alpha / A;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha / A;
        break;
    case FilterType::LowShelf:
        S = parameters_->getParameter<ParameterType::SHELF>()->getInstantaneousValue() ;
        A = std::pow(10.0, parameters_->getParameter<ParameterType::DBGAIN>()->getInstantaneousValue() / 40.0 );
        alpha = (sin_w0 / 2.0) * std::sqrt((A + 1.0 / A) * ( 1.0 / S - 1.0 ) + 2.0 );
        b0 = A * ((A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha );
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0);
        b2 = A * ((A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha );
        a0 = (A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha ;
        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_w0 );
        a2 = (A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha ;
        break ;
    case FilterType::HighShelf:
        S = parameters_->getParameter<ParameterType::SHELF>()->getInstantaneousValue() ;
        A = std::pow(10.0, parameters_->getParameter<ParameterType::DBGAIN>()->getInstantaneousValue() / 40.0 );
        alpha = (sin_w0 / 2.0) * std::sqrt((A + 1.0 / A) * ( 1.0 / S - 1.0 ) + 2.0 );
        b0 = A * ((A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha );
        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w0);
        b2 = A * ((A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha );
        a0 = (A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha ;
        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_w0 );
        a2 = (A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha ;
        break ;
    case FilterType::AllPass:
        Q = parameters_->getParameter<ParameterType::Q_FACTOR>()->getInstantaneousValue();
        alpha = sin_w0 / ( 2.0 * Q );
        b0 = 1.0 - alpha ;
        b1 = -2.0 * cos_w0 ;
        b2 = 1.0 + alpha ;
        a0 = b2 ;
        a1 = b1 ;
        a2 = b0 ;
        break ;
    default:
        throw std::runtime_error("unknown filter type configured");
    }

    // normalize coefficients:
    coefficients_ = {b0/a0, b1/a0, b2/a0, a1/a0, a2/a0};
}

inline double BiquadFilter::getCurrentOutput(double input){
    return getCurrentOutput(input, state1_, state2_);
}

inline double BiquadFilter::getCurrentOutput(double input, double& s1, double& s2) const {
    // Direct Form II transposed
    double output = coefficients_[0] * input + state1_ ;
    s1 = coefficients_[1] * input - coefficients_[3] * output + state2_ ;
    s2 = coefficients_[2] * input - coefficients_[4] * output ;
    return output ;
}

double BiquadFilter::modulate(double value, ModulationData* mData) const {
    double output = 0 ;

    // check required data
    if ( !mData ) return output ; 
    if ( ! mData->has(ModulationParameter::FILTER_STATE_1) ){
        mData->set(ModulationParameter::FILTER_STATE_1, 0.0);
    }
    if ( ! mData->has(ModulationParameter::FILTER_STATE_2) ){
        mData->set(ModulationParameter::FILTER_STATE_2, 0.0);
    }
    
    double s1 = mData->get(ModulationParameter::FILTER_STATE_1); 
    double s2 = mData->get(ModulationParameter::FILTER_STATE_2);

    output = getCurrentOutput(value,s1, s2);

    // tick modulation data
    mData->set(ModulationParameter::FILTER_STATE_1, s1);
    mData->set(ModulationParameter::FILTER_STATE_2, s2);
    
    return output ;
}

void BiquadFilter::calculateSample(){
    if ( dirty_ ){
        calculateCoefficients();
        dirty_ = false ;
    }
    double input = aggregateInputs(0);

    setBufferValue(0, getCurrentOutput(input, state1_, state2_));
}

void BiquadFilter::tick(){
    BaseModule::tick();
}

void BiquadFilter::onParameterChanged([[maybe_unused]] ParameterType p){
    dirty_ = true ;
} 
