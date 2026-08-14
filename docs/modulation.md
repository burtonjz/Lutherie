# Modulation System

This document explains how modulation works internally in Lutherie: what a modulator is, how a modulation connection gets from an API request to a per-sample value change, the different modulation strategies and why a parameter picks one, and the depth/parent-child mechanics that build on top
of it.

---

## 1. Overview

Modulation in Lutherie is the interaction of three things:

1. **A modulator** — any component that inherits `ModulatorComponent`. Examples: `Oscillator` (as an LFO/audio-rate source), `ADSREnvelope`, `LinearFader`.
2. **A target parameter** — a modulatable `Parameter` living inside some component's `ParameterMap`. The Parameter itself holds a pointer to its current modulator.
3. **A modulation strategy** — the arithmetic rule (e.g., additive, multiplicative, exponential, replace) used each tick to fold the modulator's output into the parameter's *instantaneous* value.

Critically: **a parameter has exactly one modulator slot, not a sum of inputs.** setting a new modulator connection when one is already defined is unavailable in the GUI, and, if done directly through the control API, would simply replace the existing modulator.

---

## 2. Two values per parameter: `value_` vs `instantaneousValue_`

Every [Parameter](../synth/src/params/Parameter.hpp) stores two numbers:

- `value_` — the "base" value: whatever was last set via `set_parameter`, clamped to `[minValue_, maxValue_]`. This is what the API reports back from `get_parameter` and what persists across modulation.
- `instantaneousValue_` — the value **after** modulation has been applied for the current tick. This is what the DSP code actually reads when it wants "the current value of this knob."

`modulate()` recomputes `instantaneousValue_` from `value_` and the modulator's output; it never overwrites `value_`. This means turning off modulation (`remove_connection`) instantly reverts the parameter to exactly what it was set to, with no residual drift.

`instantaneousValue_` cannot be externally set as it is only designed to be managed through modulation. However, if a component ever needs to respond to a modulation update, then it can subscribe as a listener to the Parameter, and each time the `instantaneousValue_` is set, the listener will be notified. A good reference example for this is in the [BiquadFilter.hpp](../synth/src/components/BiquadFilter.hpp), which uses this as a simple performance optimization to only recompute when necessary.

---

## 3. Modulation Strategies

When a `Parameter` is set to be modulated, it needs a `ModulationStrategy` (`shared/types/ParameterType.hpp`) in order to determine the proper way to mathematically adjust the `instantaneousValue_`:

| Strategy | Formula (`v` = base value, `d` = instantaneous depth, `m` = `modulator->modulate(v, data)`) | Typical use |
|---|---|---|
| `ADDITIVE` | `v + d·m` | Linear offsets: phase, pan, gain (dB), BPM |
| `MULTIPLICATIVE_ZERO` | `v · d·m` | Scaling toward zero: amplitude/gain from a **unipolar** (0..1) source |
| `MULTIPLICATIVE_UNITY` | `v · (1 + d·m)` | Scaling around the current value: amplitude/gain from a **bipolar** (±1) source, so `m = 0` is a no-op |
| `EXPONENTIAL` | `v · 2^(d·m)` | Musically-linear pitch/time modulation: frequency, detune, cutoff, bandwidth, Q, attack/decay/release |
| `REPLACE` | `d·m` | Modulator fully overrides the base value |
| `NONE` | `v` (unchanged) | Non-modulatable parameters (waveform, filter type, MIDI value, trigger, etc.) |

This is computed in `Parameter<typ>::modulate()` template function.

Note the result is always passed back through `limitToRange()`, so modulation can never push a parameter outside its configured min/max, however extreme the modulator output or depth.

### 3.1 Strategy Selection

In order to implement sensible strategy defaults, while permitting an appropriate level of isolation between parameter and modulator, a default strategy is determined by a combination of the `ModulatorRange` and the `ParameterType` definition. First, a modulator is required to report its output range via the override on `ModulatorComponent::getModulatorRange()` (`ModulatorRange::UNIPOLAR`, `BIPOLAR`, or `UNKNOWN`). Second, the `ParameterType` itself (see [Adding Parameters](adding-parameters.md)) defines a default lookup to provide what would be the most traditional modulation method for that parameter:

```cpp
template <> struct ParameterTraits<ParameterType::AMPLITUDE>{
    ...
    static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> defaultStrategy = {{
        {ModulatorRange::UNIPOLAR, ModulationStrategy::MULTIPLICATIVE_ZERO},
        {ModulatorRange::BIPOLAR,  ModulationStrategy::MULTIPLICATIVE_UNITY},
        {ModulatorRange::UNKNOWN,  ModulationStrategy::MULTIPLICATIVE_ZERO},
    }};
};
```

When `setModulation()` is called on a `Parameter`, it sets itself up with the default strategy:

```cpp
void setModulation(ModulatorComponent* modulator, ModulationData modData){
    if ( modulator){
        modData_ = modData ;
        modulator_ = modulator ;
        modStrategy_ = GET_PARAMETER_MODULATION_STRATEGY(type_, modulator_->getModulatorRange());
    } ...
}
```

So the same `AMPLITUDE` parameter behaves differently depending on what's plugged into it: an `Oscillator` (bipolar) as an amplitude modulator gets `MULTIPLICATIVE_UNITY` (tremolo around the current level), while an `ADSREnvelope` (unipolar) gets `MULTIPLICATIVE_ZERO` (the envelope directly scales the level from 0 up to the base value). 

These defaults can be overriden via the GUI modulation menu for the component, which calls the `"set_modulation_strategy"` control API action under the hood.

## 4. `ModulationData`: passing context into a modulator

A modulator's `modulate()` call receives a `ModulationData*` — a small fixed-size struct (`shared/params/ModulationParameter.hpp`) carrying context the modulator needs beyond the raw parameter value, without needing additional parsing capabilities of the modulated parameter. Each `ModulatorComponent` declares which of these keys it desires needs via `expectedParams_` (a `std::set<ModulationParameter>`). For example:

```cpp
/* ADSREnvelope generally needs to know which note triggered them, 
as well as an initial value should the key have been re-pressed during release
*/
expectedParams_ = { ModulationParameter::MIDI_NOTE, ModulationParameter::INITIAL_VALUE };
```

Modulators are expected to work without context data initially defined. In fact, in the case of `ADSREnvelope`, most components are not capable of supplying a `MIDI_NOTE`, a notable exception being the `PolyphonicOscillator`, which implements a parent/child polyphony system capable of attributing specific child components to specific midi notes. In most cases, however, the ADSR falls back to "global mode": using the last key press and a `retrigger` strategy to send out a modulation signal.

To this end, when a connection is made, `BaseComponent::onSetParameterModulation` seeds any of the modulator's required keys that weren't already present in the supplied `ModulationData`, defaulting them to `0.0`.

```cpp
// Note, this function generally is not overridden, except in the case of child/parent components. See §7 for a deeper explanation
void BaseComponent::onSetParameterModulation(ParameterType p, ModulatorComponent* m, ModulationData d){
    if ( d.isEmpty() && m ){
        auto required = m->getRequiredModulationParameters();
        for ( auto mp : required ){
            d.set(mp,0.0f);
        }
    }
    parameters_->getParameter(p)->setModulation(m,d);
}
```

`ModulationData` is per-*target*, stored alongside the modulator pointer on the `Parameter` itself (`modData_`), not within the modulator. This allows a modulator to be "semi-stateless" -- a modulator has its own internal state, but it can modulate multiple parameters without any threat of interference between them. The modulator doesn't care what is asking to be modulated, it just manages necessary context in this struct. A common pattern is for the modulator to put historical data (e.g., `LAST_OUTPUT`) into the map so that those state variables can be used on the next tick. Other modulators don't require any context, and simply report out a modulation value based on their internal state (e.g., `Oscillator`)

---

## 5. Depth: modulating the modulation

Every non-`DEPTH` parameter carries a companion `Parameter<ParameterType::DEPTH>` parameter.

```cpp
if constexpr (typ != ParameterType::DEPTH){
    depth_ = new (depthStorage_.buf) Parameter<ParameterType::DEPTH>(1.0f, true);
}
```

`DEPTH` parameters intentionally do **not** get their own depth child — this caps the recursion at one level (you can modulate a parameter's depth, but not the depth of that depth). Depth defaults to `1.0` (unity — full modulator effect) and its own default strategy is `ADDITIVE`, ranging `[-10, 10]`.

Each tick, `Parameter::modulate()` first modulates its own depth (if the depth itself has a modulator attached), then uses the depth's resulting `instantaneousValue_` as the multiplier on the main modulator's output. Depth modulation is wired up exactly like parameter modulation, just through the `*_depth_*` API actions and `create_depth_connection` (see [API Reference](./api-reference.md) §2.5). In the GUI, modulation depth connections become available through the modulation socket of a component node, once the parameter already has a defined modulator.

---

## 6. Reciprocal tracking & lifecycle

`ModulatorComponent` keeps a `std::set<ModulationTarget>` of everything it currently modulates:

```cpp
struct ModulationTarget {
    BaseComponent* component ;
    ParameterType param ;
    bool depth = false ;
};
```

Every `BaseComponent::setParameterModulation` / `setParameterDepthModulation` call updates both sides of the relationship: the target `Parameter` stores the modulator pointer, and the modulator stores the target in its `modulated_` set. This reciprocal bookkeeping is not used for the actual act of modulation on the hot path, but rather fulfills the following roles:

- cleaning up modulation on a `remove_component` event, which needs to clean up all connections prior to removal.
- GUI indicators (e.g., highlighting that a particular modulation is active)
- if the modulator is an `AudioSignalComponent` (i.e., stateful, audio-rate dependant modulator, like `Oscillator`), we also track the modulator so it can be injected into the signal graph, which allows it to stay up-to-date even though its not in the normal audio signal chain. See [SignalController::updateProcessingGraph()](../synth/src/signal/SignalController::instance()->hpp) for implementation details.
 
---

## 7. Parent/child propagation (polyphonic components)

Components with children — currently `PolyOscillator`, wrapping a pool of per-voice `Oscillator`s — don't modulate a single parameter; they need every
active voice modulated identically. Because this is such an edge case, this is not currently a centralized pattern (see [Adding Components](adding-components.md) §5 and `PolyOscillator.cpp`):

1. If the requested parameter exists directly on the parent's own `ParameterMap`, handle it normally via the base class.
2. Otherwise, it's a *per-voice* parameter (e.g. `frequency`, `amplitude`): record the modulator/strategy/data in a parent-side override array (`modulators_`, `strategyOverrides_`, `modulationData_`, all sized `N_PARAMETER_TYPES`), **then** fan the same call out to every active child in the voice pool. When new voices are activated, it assigns the modulation at that same time via the parent. Similar overrides exist for other necessary propogation overrides.
3. 
---

## 8. End-to-end example

Route an `Oscillator`'s raw output as vibrato on a `Polyphonic Oscillator`'s
frequency, with a depth envelope from an `ADSR Envelope`:

```jsonc
{ "action": "add_component", "name": "Oscillator" }              // id 0 — LFO
{ "action": "set_parameter", "componentId": 0, "parameter": "frequency", "value": 5.0 }
{ "action": "set_parameter", "componentId": 0, "parameter": "amplitude", "value": 1.0 }
{ "action": "add_component", "name": "Polyphonic Oscillator" }   // id 1
{ "action": "add_component", "name": "ADSR Envelope" }           // id 2

// LFO -> frequency modulation (Oscillator is BIPOLAR -> EXPONENTIAL strategy, since
// frequency's UNKNOWN/BIPOLAR/UNIPOLAR default is EXPONENTIAL either way)
{
  "action": "create_connection",
  "inbound":  { "componentId": 1, "parameter": "frequency", "socketType": "Modulation Inbound" },
  "outbound": { "componentId": 0, "socketType": "Modulation Outbound" }
}

// ADSR -> depth of that same frequency modulation, so vibrato fades in
{
  "action": "create_depth_connection",
  "inbound":  { "componentId": 1, "parameter": "frequency", "socketType": "Modulation Inbound" },
  "outbound": { "componentId": 2, "socketType": "Modulation Outbound" }
}
```

Each tick: 
    the `ADSREnvelope` updates its own envelope value based on incoming MIDI data → 
    becomes `instDepth` for the frequency parameter → 
    the `Oscillator` LFO's current sample is read as `m` → 
    `PolyOscillator`'s per-voice frequency parameters apply `EXPONENTIAL`: `newFreq = baseFreq * 2^(instDepth * lfoSample)`, fanned out to every active voice.

The ultimate effect is we get a nice vibrato from the LFO, with the envelope controlling the intensity of that vibrato, allowing it to fade in and out.
---