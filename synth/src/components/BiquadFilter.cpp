#include "components/BiquadFilter.hpp"

#include "capabilities/AudioIO.hpp"    // IWYU pragma: keep
#include "capabilities/Modulator.hpp"  // IWYU pragma: keep
#include "capabilities/Parameters.hpp" // IWYU pragma: keep

#include "interfaces/ParameterAccessor.hpp"

#include "units/decibel.hpp"
#include <cmath>

BiquadFilter::BiquadFilter(int id, BiquadFilterConfig):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::BiquadFilter),
    AudioProcessor(),
    ModulationSource(ModulatorRange::UNKNOWN, {}),
    ParameterObserver(),
    state1_(0.0),
    state2_(0.0),
    coefficients_(),
    dirty_(false),
    filterType_(createSlotAccessor<ParameterType::FILTER_TYPE>()),
    cutoff_(createSlotAccessor<ParameterType::FREQUENCY>()),
    gain_(createSlotAccessor<ParameterType::GAIN>()),
    qFactor_(createSlotAccessor<ParameterType::Q_FACTOR>()),
    bandwidth_(createSlotAccessor<ParameterType::BANDWIDTH>()),
    shelf_(createSlotAccessor<ParameterType::SHELF>())
{
    addCapability<Capability::AudioIO>(*this, 1, 1);
    addCapability<Capability::Modulator>(*this);

    auto& params = addCapability<Capability::Parameters>(this);
    
    params.addUnitParameter(ParameterType::FILTER_TYPE);
    params.addUnitParameter(ParameterType::FREQUENCY);
    params.addUnitParameter(ParameterType::GAIN);
    params.addUnitParameter(ParameterType::Q_FACTOR);
    params.addUnitParameter(ParameterType::BANDWIDTH);
    params.addUnitParameter(ParameterType::SHELF);

    params.bindSlot<ParameterType::FILTER_TYPE>(filterType_, 0);
    params.bindSlot<ParameterType::FREQUENCY>(cutoff_, 0);
    params.bindSlot<ParameterType::GAIN>(gain_, 0);
    params.bindSlot<ParameterType::Q_FACTOR>(qFactor_, 0);
    params.bindSlot<ParameterType::BANDWIDTH>(bandwidth_, 0);
    params.bindSlot<ParameterType::SHELF>(shelf_, 0);

    params.observe(ParameterType::FILTER_TYPE);
    params.observe(ParameterType::FREQUENCY);
    params.observe(ParameterType::GAIN);
    params.observe(ParameterType::Q_FACTOR);
    params.observe(ParameterType::BANDWIDTH);
    params.observe(ParameterType::SHELF);
    
    params.freezeParameterCreation();
    
    calculateCoefficients(); // prime the filter
}

void BiquadFilter::onPrepare(AudioContext ctx){
    sampleRate_ = ctx.sampleRate ;
}

// see https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
void BiquadFilter::calculateCoefficients(){
    double b0, b1, b2, a0, a1, a2 ;
    double w0, alpha, sin_w0, cos_w0, A, S, Q ;

    w0 = 2.0 * M_PI * cutoff_.modulatedValue() / sampleRate_ ;
    cos_w0 = std::cos(w0);
    sin_w0 = std::sin(w0);

    switch(FilterType::from_uint8(filterType_.value())){
    case FilterType::LowPass:
        Q = qFactor_.modulatedValue();
        alpha = sin_w0 / ( 2.0 * Q );
        b0 = (1.0 - cos_w0) / 2.0 ; // b0
        b1 = 1.0 - cos_w0 ; // b1
        b2 = b0 ; // b2
        a0 = 1.0 + alpha ; // a0
        a1 = -2.0 * cos_w0 ; // a1
        a2 = 1.0 - alpha ; // a2
        break ;
    case FilterType::HighPass:
        Q = qFactor_.modulatedValue();
        alpha = sin_w0 / ( 2.0 * Q);
        b0 = (1.0 + cos_w0) / 2.0 ;
        b1 = -1.0 - cos_w0 ;
        b2 = b0 ;
        a0 = 1.0 + alpha ;
        a1 = -2.0 * cos_w0 ;
        a2 = 1.0 - alpha ;
        break ;
    case FilterType::BandPass:
        Q = qFactor_.modulatedValue();
        alpha = sin_w0 / ( 2.0 * Q);
        b0 = alpha ;
        b1 = 0.0 ;
        b2 = -b0 ;
        a0 = 1.0 + alpha ;
        a1 = -2.0 * cos_w0 ;
        a2 = 1.0 - alpha ;
        break ;
    case FilterType::BandStop:
        Q = qFactor_.modulatedValue();
        alpha = sin_w0 / (2.0 * Q);
        b0 = 1.0;
        b1 = -2.0 * cos_w0;
        b2 = 1.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;
    case FilterType::PeakingBell:
        Q = qFactor_.modulatedValue();
        A = biquadA(gain_.modulatedValue());
        alpha = sin_w0 / (2.0 * Q);
        b0 = 1.0 + alpha * A;
        b1 = -2.0 * cos_w0;
        b2 = 1.0 - alpha * A;
        a0 = 1.0 + alpha / A;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha / A;
        break;
    case FilterType::LowShelf:
        S = shelf_.modulatedValue();
        A = biquadA(gain_.modulatedValue());
        alpha = (sin_w0 / 2.0) * std::sqrt((A + 1.0 / A) * ( 1.0 / S - 1.0 ) + 2.0 );
        b0 = A * ((A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha );
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0);
        b2 = A * ((A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha );
        a0 = (A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha ;
        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_w0 );
        a2 = (A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha ;
        break ;
    case FilterType::HighShelf:
        S = shelf_.modulatedValue();
        A = biquadA(gain_.modulatedValue());
        alpha = (sin_w0 / 2.0) * std::sqrt((A + 1.0 / A) * ( 1.0 / S - 1.0 ) + 2.0 );
        b0 = A * ((A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha );
        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w0);
        b2 = A * ((A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha );
        a0 = (A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * std::sqrt(A) * alpha ;
        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_w0 );
        a2 = (A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * std::sqrt(A) * alpha ;
        break ;
    case FilterType::AllPass:
        Q = qFactor_.modulatedValue();
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

inline double BiquadFilter::currentOutput(double input){
    return currentOutput(input, state1_, state2_);
}

inline double BiquadFilter::currentOutput(double input, double& s1, double& s2) const {
    // Direct Form II transposed
    double output = coefficients_[0] * input + s1 ;
    s1 = coefficients_[1] * input - coefficients_[3] * output + s2 ;
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

    output = currentOutput(value,s1, s2);

    // tick modulation data
    mData->set(ModulationParameter::FILTER_STATE_1, s1);
    mData->set(ModulationParameter::FILTER_STATE_2, s2);
    
    return output ;
}

void BiquadFilter::processFrame(const AudioAccessor& accessor, size_t frame) noexcept {
    if ( dirty_ ){
        calculateCoefficients();
        dirty_ = false ;
    }


    double input = accessor.input(0)[frame];

    accessor.output(0)[frame] = currentOutput(input, state1_, state2_);
}

void BiquadFilter::onParameterChanged(ParameterType, uint16_t){
    dirty_ = true ;
} 

double BiquadFilter::biquadA(double g) const {
    return std::sqrt(std::max(g, decibel::silenceFloor));
}
