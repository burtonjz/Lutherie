# How to Add a New Component

## Prerequisites
- Understand the three base classes: `AudioSignalComponent` (signal processing), `ModulatorComponent` (modulation source), `MidiEventHandler` (MIDI input)
- Your component can inherit from any combination of these (they all inherit from `BaseComponent`)

---

## Step 1: Specify Component Type
**Location:** `shared/types/ComponentType.hpp`

make sure to update the X Macro in that file as well!

## Step 2: Create Component Files
**Location:** `synth/src/components/`

Create `MyComponent.hpp` and `MyComponent.cpp`

**Constructor signature (required):**
```cpp
MyComponent(ComponentId id, const MyComponentConfig& config):
    BaseComponent(id, ComponentType::MyComponent)
```

---

## Step 3: Define Parameters
1. If using new parameter types, create them first (see: "How to Add a New Parameter Type")
2. All parameters must be added to the parameter map in your component's constructor
3. Set initial values using config data

---

## Step 4: Implement Base Class Requirements
All Components must inherit from BaseComponent, but it is generally expected that your component will inherit this indirectly through at least one of the following classes:
- `AudioSignalComponent` - for signal processing 
- `ModulatorComponent` - for modulation sources 
- `MidiEventHandler` - for MIDI input handling

In addition, it may be necessary to inherit from:
- `MidiEventListener` - for MIDI input listening 
  
Check each base class for necessary overrides.

---

## Step 5: Handle Child Components (If Applicable)
If your component will manage any child components, it is responsible for doing so through overrides -- there is currently no centralized pattern for managing child components.

See `PolyphonicOscillator` for an appropriate example example of implementing a component with children. However, at a high level we need to make sure:

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

If NO children, you can skip this step.

---

## Step 6: Create Component Config
**Location:** `synth/src/configs/`

Component Config files are used to create a standard system for our component factory to produce components with all necessary input variables/defaults:
```cpp
class MyComponent ; // forward declaration

struct MyComponentConfig {
    // Include all parameters needed for construction
    Waveform waveform = Waveform::SINE ;
    double frequency = 440.0f ;
};

template <> struct ComponentTypeTraits<ComponentType::MyComponent>{ 
    using type = MyComponent ;
    using config = MyComponentConfig ;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MyComponentConfig, waveform, frequency) // macro to serialize/deserialize json <-> structs
```

---

## Step 7: Register Component Metadata
**Location:** `shared/meta/ComponentRegistry.cpp`

Add an entry containing:
- Component type identifier
- Display name
- Parameter list
- I/O configuration
- Any other metadata needed by frontend/backend

This registry is shared between frontend and backend.
---

## Step 9: Frontend Integration (Optional)
**Location:** `gui/views/ComponentEditor.hpp`

Only needed if you want custom parameter visualizations for a newly created parameter

Update `ComponentEditor::createParameter()`:

---