# How to Add a New Component

Components in Lutherie are designed to follow a "Unix-like" philosophy. This means that generally speaking, a component should be considered the smallest piece of standalone processing possible. A new component should only be created if it represents a building block that does not already exist, or involves an algorithmic improvement that would otherwise not be possible as separate components.

For example, a `BiquadFilter` can technically be created through delays and other mathematical operations. However, it requires its own standalone class to implement the "Direct Form II Transposed" implementation, which provides notable speed increases. For this reason, it is justifiably its own, standalone component.

Prior to creating a new component, the developer must thoughtfully determine whether it is necessary, or if it can be reasonably implemented using the already available "building blocks". 

Below are the steps for developing a new component:

---

## 1. Specify Component Type

Update the [ComponentType](../shared/types/ComponentType.hpp) X Macro to create the component enum entry and keep factory methods in sync.

## 2. Create Component Files

In `synth/src/components/`, create `MyComponent.hpp` and `MyComponent.cpp`. The class must have the following construction signature:

```cpp
MyComponent(ComponentId id, const MyComponentConfig& config);
```

Where `MyComponentConfig` is defined in the next step.

---

## 3. Create Component Config

Component Config files are used to create a standard system for our component factory to produce components with all necessary input variables/defaults, and is required for the component constructor.

In `synth/src/configs/`, create `MyComponentConfig.hpp`. Note that this file must match the `ComponentType` defined in §1, followed by `Config`, in order for factory methods to successfully compile with your new component.

```cpp
class MyComponent ; // forward declaration of component class

struct MyComponentConfig {
    // Include all parameters needed for construction
    Waveform waveform = Waveform::SINE ;
    double frequency = 440.0f ;
};

// this template struct is required for factory construction
template <> struct ComponentTypeTraits<ComponentType::MyComponent>{ 
    using type = MyComponent ;
    using config = MyComponentConfig ;
};

// macro to serialize/deserialize json <-> structs
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MyComponentConfig, waveform, frequency) 
```

---

## 4. Define Parameters

By default, all components have access to a `parameters_` object of type [ParameterMap](../synth/src/params/ParameterMap.hpp). Any values that clients are expected to interact with **must** be defined within this map in the constructor of your component class. Other variables only referenced internally may be defined directly in your class. See [Adding Parameters](adding-parameters.md) if you need to add a new parameter type.

Individual controls should be defined using the base parameter system, e.g.,

```cpp
parameters_->add<ParameterType::FREQUENCY>(
    cfg.frequency, // required: default value
    true, // required: modulatable
    minValue, // optional, defaults to minimum defined for parameter
    maxValue, // optional, defaults to maximum defined for parameter
    modulator, // optional, defaults to nullptr but allows a modulator to immediately be connected, not recommended
    modulationData, // optional, defaults to empty object, not recommended
);
```

If your component requires multiple parameters to be updated in association with each other, or repeats of a particular parameter, then the control should be added as a `ParameterCollection`. See [Sequencer](../synth/src/components/Sequencer.hpp) for an example of this. These can be added in your component like:

```cpp
// sequencer uses a CollectionStructure::SYNCHRONIZED to force these
// four parameters to be added together
parameters_->addCollection<ParameterType::MIDI_VALUE>({});
parameters_->addCollection<ParameterType::VELOCITY>({});
parameters_->addCollection<ParameterType::START_POSITION>({});
parameters_->addCollection<ParameterType::DURATION>({});
```

*Notes on Collections*

- `CollectionStructure` (defined in the `ComponentDescriptor`, see §6) specifies how the different Parameter Collections relate to each other. 
- The `ParameterMap` itself does not have any awareness to the structured nature of the data. Instead, the validation operations are enforced at the [Control API](api-reference.md) layer -- see the section on `CollectionRequest`. This allows `ParameterMap` to be fully decoupled from any given component.
- The `ComponentDescriptor` currently only supports one `CollectionDescriptor`.

---

## 5. Implement Base Class(es) Requirements

All Components must inherit from `BaseComponent`. It is generally expected that your component will inherit this indirectly through at least one of several base classes defined below. There are no restrictions on how many / which combination of base classes may be inherited:

| Component | Constructor | Connections | Description |
| --- | --- | --- | --- |
|BaseComponent|BaseComponent(ComponentId id, ComponentType type)| inbound parameter modulation | Direct inheritance unneccessary in header files as other classes virtually inherit in their constructors. However, the constructor must be called during component class construction. |
|AudioSignalComponent|AudioSignalComponent(size_t in, size_t out)| inbound/outbound audio signal | used for real-time audio processing, so no allocation or other blockers should be introduced on hot path|
|AudioBufferComponent|AudioBufferComponent(size_t in, size_t out)| inbound/outbound audio buffer | used to process audio buffers (e.g., read from an audio file or saved from an audio stream via converter)|
|ModulatorComponent|ModulatorComponent()| outbound modulation | specifies a unique path for modulation output to be read by Parameter objects. |
|MidiEventHandler|MidiEventHandler()| outbound MIDI | allows component to manage and manipulate an incoming stream of midi information. Note: all handlers are listeners, as they receive midi states from a centralized object. |
|MidiEventListener|MidiEventListener()| inbound MIDI | allows a component to respond to midi signals. |
|AudioProbe|AudioProbe()|inbound audio signal only| inherits from AudioSignalComponent, audio sink with outbound UDP data sends. |

Most new components will only inherit from one of these. "Converter" components (i.e., components responsible for manipulating data/connection type) are a notable exception -- however, only 1 designated "converter" component should be defined for cases where the input/output combo changes. For example, `BufferStreamer` is responsible for converting an `AudioBufferComponent` to a `AudioSignalComponent`, and so no other component should fulfill that role.  

If a component is inheriting from many of these subtypes, it is probably an indicator that the component is not following our "unix-like" philosophy, and it is probably worth rethinking how the pieces can be appropriately broken. After selecting the appropriate subtypes, reference its section below to see more specific implementation details.
  
### 5.1. BaseComponent Requirements

Generally speaking, [BaseComponent](../synth/src/core/BaseComponent.hpp) function overrides will not be needed. However, in the case of Parent/Child Component Configurations (See §7), any of the following may need to have overrides in order to properly manage child components:

|Function|Description|
|---|---|
|ModulatorComponent* getParameterModulator(ParameterType p) const | return the modulator of a given parameter. |
|ModulatorComponent* getParameterDepthModulator(ParameterType p) const | return the depth modulator of a given parameter. |
|double getParameterDepth(ParameterType p) const | return the depth value of the specified parameter.  |
|void setParameterDepth(ParameterType p, double depth) | set the depth value of the specified parameter. | 
|ModulationStrategy getParameterModulationStrategy(ParameterType p) const | get the modulation strategy for the specified parameter. |
| void setParameterModulationStrategy(ParameterType p, ModulationStrategy strat) | set the modulation strategy for the specified parameter |
| void updateParameters() | by default, runs the modulation for all internal parameters. |
| void onSetParameterModulation(ParameterType p, ModulatorComponent* m, ModulationData d ) | any additional operations required during a parameter modulation event |
| void onRemoveParameterModulation(ParameterType p) | any additional operations required during a parameter modulation removal event |
| void onSetParameterDepthModulation(ParameterType p, ModulatorComponent* m, ModulationData d ) | any additional operations required during a parameter depth modulation event |
| void onRemoveParameterDepthModulation(ParameterType p) | any additional operations required during a parameter depth modulation removal event |
| void onParameterChanged(ParameterType p, bool isCollection) | register itself as a `ParameterListener`, allowing custom behavior whenever the instantaneous (modulated) value changes |

### 5.2. AudioSignalComponent Requirements

All overrideable functions have "do-nothing" defaults, so not defining "required" functions does not cause compilation failure. However, the component will generally not be functional unless they fullfill some of the generally-expected overrides:

|Function|Required|Description|
|---|---|---|
|void calculateSample()| Yes | calculate a sample value and set it for the buffer. This is the value that will get set to the current index of the internal buffer |
|void tick()| No | perform any per-sample operations required by your class. You **must** call the base implementation in order to ensure the parent class buffer is properly ticked. |
|void onInputConnect()| No | update internal state after an input connect event |
|void onInputDisconnect()| No | update internal state after an input disconnect event |

### 5.3. AudioBufferComponent Requirements

Similar to the `AudioSignalComponent`. All overrideable functions have "do-nothing" defaults:

|Function|Required|Description|
|---|---|---|
|void onInputConnect()| Yes | update internal state after an input connect event |
|void onInputDisconnect()| Yes | update internal state after an input disconnect event |
|void onInputUpdated() | Yes | update internal state after an upstream buffer communicates its own update |

in these functions, it is generally expected that the function queries its own parameters, inputs, etc. to surmise state, and make appropriate updates off of that. It is also expected that changes to internal state are communicated downstream via the `notifyDownstream()` function.

### 5.4. ModulatorComponent Requirements

**This component class has required overrides**, and will fail to compile if not defined. See [Modulation](modulation.md) for more details on modulation behavior.

|Function|Required|Description|
|---|---|---|
|double modulate(double value, ModulationData* mdat) const | Yes | provide an output modulation value. See additional details below on modulation data. |
| ModulatorRange getModulatorRange() | No | Returns "unknown" by default. In order to maintain class separation between modulators and modulated parameters, the modulator needs a way to report what range of values it returns. This allows the parameter to set a reasonable default modulation strategy |


### 5.5. MidiEventHandler Requirements

The following overrides are not strictly necessary, but each override is generally available in order to provide any needed management functionality.

|Function|Required|Description|
|---|---|---|
| void onKeyPressed(const ActiveNote* note, bool rePressed = false) override | No | default behavior is to immediately push the key press into the outbound queue. Overriding allows to add custom component handling behavior. |
| void onKeyReleased(ActiveNote anote) override | No | default behavior pushes a release event for the note into the outbound queue |
| void onKeyOff(ActiveNote anote) override | No | default behavior pushes a note off event for the note into the outbound queue |
| void onPitchbend(uint16_t pitchbend) override | No | default behavior pushes a pitchbend control event into the outbound queue |
| bool shouldKillNote(const ActiveNote& anote) const | No | default behavior determines that a note is killed as soon as an inbound note_off event is received. Overriding this allows for the component to hold on to a note longer than its default lifecycle. |
| void onTick(float dt) | No | By default, this function does nothing. MIDI events tick once per received audio buffer (on hot path), and this allows us to define behavior that needs to trigger on specific timing |
| void onReset() | No | prior to a reset event (clearing out existing midi notes), perform any additional cleanup that may need to occur. Default no-op |
| void onListenerAdded() const | No | response to a connection added event. Default no-op |
| void onListenerRemoved() const | No | respond to a connection removed event. Default no-op |


### 5.6. MidiEventListener Requirements

The following overrides are available to a component class in order to respond to MIDI events. By default, these are no-op:

|Function|Description|
|---|---|
| void onKeyPressed(const ActiveNote* note, bool rePress = false) | respond to a key press event |
| void onKeyReleased(ActiveNote anote) | respond to a key release event |
| void onKeyOff(ActiveNote anote) | respond to a key off event |
| void onPitchbend(uint16_t pitchbend ) | respond to a pitchbend event |
| void onHandlerAdded() const | response to a connection added event |
| void onHandlerRemoved() const | respond to a connection removed event |
---

### 5.7 AudioProbe Requirements

`AudioProbe` components are a specialization of the `AudioSignalComponent`, where it has 1 audio input and 0 audio outputs. Built in is functionality for collecting and processing audio signal data, and flushing it through a UDP stream port to provide real-time visualization to the client application.

All AudioSignalComponent overrides are valid to override here except for `calculateSample`. 

The collection state is defined through the `collecting_` boolean. By default, this is turned off for this class. Generally, this variable can be toggled based on `AudioSignalComponent` overrideable events, such as responding to a parameter change or connection event.

The below are additional overrides available to AudioProbes.

| Function | Description | 
| --- | --- |
| virtual void process(const double* data, size_t size, ComponentId id) | called through the streaming api. Only requirement is the final output must be sent via `StreamingApiHandler::instance()->send(const std::vector<float>& output, int componentId)` |

## 6. Register the Component Descriptor

In `./shared/meta/ComponentRegistry.cpp`, we need to add a new entry to the registry map defined in `ComponentRegistry::getAllComponentDescriptors`. The `ComponentDescriptor` (defined in `./shared/meta/ComponentDescriptor.{hpp,cpp}`) is an object that contains critical details about a component, without needing the full implementation in its code base. Generally speaking, the ComponentDescriptor auto defines each parameter with the empty version, and it is the developers responsibility to initialize the descriptor in that map with the correct static variables. See the header file for full implementation details.

Careful attention should be paid towards the `CollectionDescriptor`, should one have been implemented. One `CollectionStructure` may be defined per component (a limitation that has not produced issues thus far), and it is this descriptor definition which manages the expectations and requirements of `CollectionRequests` through the [Control API](api-reference.md). This must be defined for the collection to be accessible and exhibit proper behavior.

## 7. Create GUI Controls

By default, any basic `ParameterType` has a corresponding, sensible widget for displaying the control in the GUI. However, there are two cases where additional GUI controls may be necessary:

1. A `ParameterCollection` was defined. In this case, the developer **must** implement GUI behavior in [ComponentParameters](../gui/widgets/ComponentParameters.hpp)`::createDetailedEditor(ComponentType t)`. See that function for reference implementations.
2. [TODO] the basic controls do not yield the end user a seamless experience in the context of the component. For example `BiquadFilter` is fully functional working off of dials, but I will eventually want to build a graphic based manager along with the existing controls. This section will get updated when that work is addressed.

---

## 8. Notes on MIDI component usage

1. **MIDI Control Messages**. Currently, we do not provide mapping of MIDI control messages other than pitchbend through the Handler/Listener paradigm. Instead, any given MIDI control message may be mapped to automate a parameter (see [API Reference §2.3 section on MIDI control mapping](api-reference.md)). 

2. **MIDI Channels**. We also do not currently perform any filtering relative to specific MIDI channels. MIDI messages from any given channel are indescriminately treated as if they are coming from one channel.

3. **MIDI Handler Chains**. Because all MIDI Handlers are listeners, this allows us to chain together MIDI operations in an indescriminate order. Raw MIDI specification only has **KEY ON** and **KEY OFF** signals, while our system includes a **KEY RELEASED** signal. 

To allow components to behave predictably, `MidiEventHandler` broadcasts downstream in the following manner:

| Handler Sends | Handler Receives | Listener Receives |
| --- | --- | --- |
| KEY ON | KEY ON | KEY ON |
| KEY RELEASED | N/A | KEY RELEASED |
| KEY OFF | KEY RELEASED | KEY OFF |

Essentially, this means that handlers get to make their own decisions regarding when to formally turn a note off, but that end users should be aware of ordering effects of handler chains

For example, an `ADSREnvelope` extends `shouldKillNote` past the KEY OFF signal, so that the sound can continue to ring out through the release phase. If a user were to chain two `ADSREnvelope`s together, then the output of the second envelope would not key off until `R1 + R2` after the initial KEY_OFF signal. 

The general rule of thumb is that **handlers extending KEY OFF behavior should be positioned at the end of the handler chain**. This ensures that listeners are capable of properly responding to all 3 event signals.

## 9. Notes on Managing Child Components

Generally, components following our "unix"-style philosophy should not have child components. If it needs other components to function, it should generally be handled through component connections. 

However, there are some use cases (like the `PolyphonicOscillator`) that provide end users with sensible functionality through a single component via this "child component" pattern.

Because this is such a rare requirement, there is no defined/centralized pattern for developing this class of component. However, should other use cases arise, at a high level we need to make sure:

1. Child components properly inherit parameters that are synced with the parent (reference parameters)
2. The parent passes down parameter and modulation actions that it receives from the API
3. The parent instructs children to perform real time behaviors (tick)

The following patterns may be helpful for implementing parent/child components:
```cpp

void getParameterModulator(ParameterType p) override {
    /**
    If children are modulatable, we need to be able to retrieve
    the modulator associated with child parameters if the function parameter is not one stored into the parent's map.
    **/
}

// from BaseComponent
void updateParameters() override {
    parameters_->modulate();  // Update this component's params first
    
    // Then update all children (example patterns):
    
    // Pattern A: children stored in an array
    for (auto& child : children_) {
        child.updateParameters();
    }
    
    // Pattern B: Polyphonic Children (using FixedPool)
    childPool_.forEachActive([](ChildType& child) {
        child.updateParameters();
    });
}

// when parameter modulation is set, we need to pass it down to
// children if it is not a parent's referenced parameter
void onSetParameterModulation(ParameterType p, ModulatorComponent* m) override {
    if ( d.empty() && m ){
        auto required = m->getRequiredModulationParameters();
        for ( auto mp : required ){
            d[mp];
        }
    }

    // store in parent arrays for reference
    modulators_[p] = m ;
    modulationData_[p] = d ;

    // apply to each child
    childPool_.forEachActive(&Oscillator::setParameterModulation, p, m, d);
}

void onRemoveParameterModulation(ParameterType param) override {
    // default behavior removes from the parent modulation map
    parameters_->getParameter(p)->removeModulation();
    // Or, remove modulation from children if needed
}

// from AudioSignalComponent
void tick(){
    AudioSignalComponent::tick(); // you must run the parent tick function

    // perform any additional per-sample actions

    // Then, explicitly tick child objects

    // Pattern A: children stored in an array
    for (auto& child : children_) {
        child.tick()
    }
    
    // Pattern B: Polyphonic Children (using FixedPool)
    childPool_.forEachActive([](ChildType& child) {
        child.tick()
    });
}

```

---
