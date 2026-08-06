# Adding a New Parameter Type

This walks through everything required to add a new `ParameterType` end to end — the backend trait definition, wiring it into a component, and getting the GUI to render something sensible for it instead of falling through to a generic slider.

A new parameter should only be created if an existing parameter does not conceptually represent the desired functionality.

---

## Step 1 — Add it to the `PARAMETER_TYPE_LIST` X-macro

Update the [ParameterType](../shared/types/ParameterType.hpp) X Macro to create the component enum entry and keep factory methods in sync.

This is the single enum-value source of truth, while also providing built in functionality into parameter-specific dispatch functions used throughout the codebase, string/enum lookups, and several other utilities.

---

## Step 2 — Define the `ParameterTraits` specialization

In `shared/types/ParameterType.hpp`, alongside the other `template <> struct ParameterTraits<ParameterType::...>` blocks, introduce your new parameter specialized struct:

```cpp
template <> struct ParameterTraits<ParameterType::MY_PARAMETER>{
    using ValueType = float ;                          // bool | uint8_t | int | float | double
    static constexpr std::string_view name = "my parameter" ;
    static constexpr float minimum = 0.0 ;
    static constexpr float maximum = 1.0 ;
    static constexpr float defaultValue = 0.5 ;
    static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> defaultStrategy = {{
        {ModulatorRange::UNIPOLAR, ModulationStrategy::ADDITIVE},
        {ModulatorRange::BIPOLAR,  ModulationStrategy::ADDITIVE},
        {ModulatorRange::UNKNOWN,  ModulationStrategy::ADDITIVE},
    }};
    static constexpr size_t uiPrecision = 2 ; // num decimals shown/used by SliderWidget
    static constexpr bool supportRangeUpdate = true ; // can min/max be changed at runtime via the API?
};
```

Notes on each field:

- **`ValueType`** must be one of the five types in `ParameterValue = std::variant<bool, uint8_t, int, float, double>` — that variant is what actually flows through the API layer (`ParameterValueToJson`, `SliderWidget`'s dispatch, etc.), so anything else won't compile against the existing dispatch macros.
- **`name`** is the exact lowercase wire string used in Control API requests (`"parameter": "my parameter"`) — see `api.md` §4. This is also the only place the string is defined; `stringToParameter` builds its lookup table directly from every trait's `name`.
- **`defaultStrategy`** picks the modulation math based on the connected modulator's declared range — see `modulator.md` §3 for how to choose the right one. If the parameter shouldn't be modulatable at all (e.g. an enum-like selector), use `NONE` for all three entries.
- **`uiPrecision`** and the `minimum`/`maximum` pair are what `SliderWidget` reads via `GET_PARAMETER_TRAIT_MEMBER` if you don't build a custom widget (step 4) — worth setting sensibly even if you *do* plan a custom widget, since the range is still used for clamping (`Parameter::limitToRange`) and for `get_parameter_range` API responses.
- **`supportRangeUpdate`** gates whether `set_parameter_range` is allowed to rewrite `minimum`/`maximum` at runtime — set `false` for parameters whose range is structurally fixed (e.g. a 0–127 MIDI value).

At this point the parameter type exists, is addressable over the API by name, and has sane defaults.

---

## Step 3 — Map it to a GUI widget

In `gui/widgets/ComponentParameters.cpp`, `ComponentParameters::createParameterWidget(ParameterType p)` determines what GUI widget will be presented for the parameter. By default, it will select a continuous knob widget. However, this may not be an appropriate representation for the new parameter. If that is the case, you must add your `ParameterType` case into the switch statement and define your new output widget. The new output widget **must** inherit from [ParameterWidget](../gui/widgets/ParameterWidget.hpp).

Several existing parameter widgets may be helpful to reference / borrow from when implementing new types, in order to keep UX consistency.

| Widget | Use Case |
|---|---|
| `WaveformWidget` / `FilterTypeWidget` | Enum-like selection rendered as a dropdown (`QComboBox`) rather than a range |
| `StatusWidget` | Boolean on/off, rendered as a toggle switch instead of a 0/1 slider |
| `DelayWidget` | A numeric value with an alternate display unit (samples ↔ ms) and its own conversion UI |
| `DetuneWidget` | A single logical value that's actually edited as two combined sub-controls (harmonic + fine detune) |

The main things to look out for on custom widgets:

- Emit `valueChanged()` on edits and `rangeChanged()` on range edits — these drive `ComponentParameters::onValueChange` / `paramRangeEdited`, which are what actually push the new value back to the engine over the [Control API](api-reference.md). A widget that doesn't emit these will look interactive but do nothing.
- Implement `onModelParameterChanged` handling correctly (inherited, but make sure `setValue()` doesn't re-emit `valueChanged()` when called from a model update, or you'll get feedback loops — this is why `ParameterWidget::setValue` takes a `block` flag, and why widgets use `QSignalBlocker` internally; see `SliderWidget::setValue` for the pattern).

---

## Step 4 -- Finish your Component

Presumably, the new parameter was defined in order to be used in a newly defined [Component](adding-components.md). At this point, the parameter is properly defined, and so adding the component will be necessary in order to fully test out the new parameter.

## Notes on Frequency

the `FREQUENCY` parameter slightly breaks convention in order to safely operate during runtime. Rather than having a static "max" on the parameter, in order to avoid exceeding the [Nyquist Frequency](https://en.wikipedia.org/wiki/Nyquist_frequency), we have runtime overrides to check against the currently configured sample rate (which is based off the available sampling rates of the peripheral audio output devices). So, when defining `FREQUENCY` within a component, the default max is simply a placeholder, and it is instead set to be gauranteed below the nyquist.

This should generally be considered an exception to the rule, but if another parameter were to be introduced with similar concerns related to requiring necessary runtime variables, then it is recommended to follow the pattern that frequency uses.