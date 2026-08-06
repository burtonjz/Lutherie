# Lutherie API Reference

The `lutherie-backend` is headless and controllable independently of the Qt6 GUI through two plain TCP sockets: a **Control API** for graph/state mutation and queries, and a **Data API** for one-way streaming of buffer data. The GUI (`gui/api/ControlApiClient.{hpp,cpp}`, `gui/api/DataApiClient.{hpp,cpp}`) is just one client of this API — anything that can open a TCP socket can drive the engine, which is what `debug/control-client.py` demonstrates.

This document describes the wire protocol as implemented in `synth/src/api/ControlApiHandler.{hpp,cpp}` and `synth/src/api/DataApiHandler.{hpp,cpp}`.

---

## 1. Connecting

Both servers bind to the host/ports configured in `shared/resources/config/config.json`, which is copied into the user config directory as specified in `./shared/platform/AppPaths.{hpp,cpp}` during first use:

```json
{
    "server": {
        "address": "127.0.0.1",
        "control_port": 12345,
        "data_port": 12346
    }
}
```

| Server      | Default port | Purpose                                   |
|-------------|--------------|--------------------------------------------|
| Control API | `12345`      | Request/response — build & control the graph |
| Data API    | `12346`      | Server → client push of raw buffer data     |

Both servers accept multiple simultaneous TCP clients. **Every response and every data message is broadcast to all currently-connected clients** on the respective socket — there is no per-client addressing or subscription model. If two clients are connected to the Control API, both see every response, regardless of which one sent the request.

Connections are plain, unauthenticated, unencrypted TCP on localhost. There is no auth layer — treat the ports as trusted-local-machine only.

---
## 2. Control API

### 2.1 Framing

The Control API communicates using **newline-delimited JSON** in both directions. The server buffers partial TCP reads and only parses a message once a complete `\n`-terminated line has been received, so requests can be sent in separate `send()` calls or pipelined back-to-back in one call.

```python
# sample python3 TCP client call
sock.sendall(b'{"action":"set_state","state":"run"}\n')
```

### 2.2 Request / response shape

Every request is a JSON object with an `"action"` field naming the handler, plus whatever parameters that action needs:

```json
{ "action": "set_parameter", "componentId": 0, "parameter": "frequency", "value": 220.0 }
```

The response is **the request object, echoed back with additional fields merged in**, plus:

- `"status"`: `"success"` or `"failed"` (added automatically if not already present)
- `"error"`: present only on failure, containing the exception message

```json
{ "action": "set_parameter", "componentId": 0, "parameter": "frequency", "value": 220.0, "status": "success" }
```

```json
{ "action": "set_parameter", "componentId": 99, "parameter": "frequency", "value": 220.0, "status": "failed", "error": "Component with id = 99 not found for parameter request" }
```

An unrecognized `"action"` value, or a request that fails to parse as JSON, also produces a `"status": "failed"` response with a descriptive `"error"`.

### 2.3 Action reference

Actions are grouped below by subsystem. Unless noted, `componentId` refers to the integer ID returned from `add_component`.

#### Engine / hardware

| Action | Request fields | Response fields |
|---|---|---|
| `get_audio_devices` | — | `data`: array of `{id, name}` |
| `set_audio_device` | `device_id` | `output_channels` |
| `get_audio_configuration` | — | `device_id`, `output_channels` |
| `get_midi_devices` | — | `data`: array of `{id, name}` |
| `set_midi_device` | `device_id` | — |
| `set_state` | `state`: `"run"` \| `"stop"` | — |
| `get_configuration` | — | `data`: full serialized engine state (see `Engine::serialize()`) |

#### Patch save/load

| Action | Request fields | Notes |
|---|---|---|
| `load_patch` | `components` (array, required), `connections` (array, optional), `midi_controls` (array, optional) | See §2.4 |

#### Component lifecycle

| Action | Request fields | Response fields |
|---|---|---|
| `add_component` | `name` (component type display name, e.g. `"Oscillator"`) | `componentId` |
| `remove_component` | `componentId` | — (also tears down all of that component's connections first; fails if any can't be removed) |
| `sync_component` | `componentId` | `data`: full serialized state of that component |

#### Connections

| Action | Request fields |
|---|---|
| `create_connection` | `inbound`, `outbound` (see §2.5) |
| `remove_connection` | `inbound`, `outbound` |
| `create_depth_connection` | `inbound`, `outbound` |
| `remove_depth_connection` | `inbound`, `outbound` |

#### Parameters

| Action | Request fields | Response fields |
|---|---|---|
| `get_parameter` | `componentId`, `parameter` | `value` |
| `set_parameter` | `componentId`, `parameter`, `value` | `value` (echoes the *actual* stored value, which may differ from the request if it was clamped) |
| `get_parameter_default` | `componentId`, `parameter` | `value` |
| `set_parameter_default` | `componentId`, `parameter`, `value` | — |
| `get_parameter_range` | `componentId`, `parameter` | `minimum`, `maximum` |
| `set_parameter_range` | `componentId`, `parameter`, `minimum`, `maximum` | — |
| `reset_parameter` | `componentId`, `parameter` | — (resets to default) |

`parameter` is the lowercase string name of a `ParameterType` (e.g. `"frequency"`, `"amplitude"`, `"filter type"`, `"midi value"`) — see `./shared/types/ParameterType.hpp` for the full list of parameters. Requesting a parameter a component doesn't have, or an out-of-range component ID, fails with an error.

#### Collections

Collections are used when a component (e.g. `Sequencer`, `MidiFilter`) needs specific parameters to be updated in sync with each other, or multiple instances of a particular `ParameterType`, based on definitions in the `CollectionDescriptor`. Implementation details can be further explored in `./shared/requests/CollectionRequest.hpp` and `./shared/meta/CollectionDescriptor.hpp`. A high level explanation is available in §2.6

| Action | Request fields |
|---|---|
| `add_collection_value` | `componentId`, `value` |
| `remove_collection_value` | `componentId`, `index` |
| `get_collection_value` | `componentId`, `index` |
| `get_collection_values` | `componentId` |
| `set_collection_value` | `componentId`, `index`, `value` |
| `add_collection_values` | `componentId`, `value` (bulk add) |
| `reset_collection` | `componentId` |

#### Modulation

This application allows users to manage a parameter's `ModulationStrategy`, which defines how the parameter responds to a modulation value. The below requests are only valid for `modulatableParameters`, which are defined in the `ComponentDescriptor`:

| Action | Request fields | Response fields |
|---|---|---|
| `get_modulation_strategy` | `componentId`, `parameter` | `strategy` |
| `set_modulation_strategy` | `componentId`, `parameter`, `strategy` | — |
| `get_modulation_depth` | `componentId`, `parameter` | `depth` |
| `set_modulation_depth` | `componentId`, `parameter`, `depth` | — |

#### Files / buffers

| Action | Request fields | Response fields |
|---|---|---|
| `get_file_path` | `componentId` | `path` |
| `set_file_path` | `componentId`, `path` | — |
| `save_buffer` | `componentId`, `path` | — |
| `get_buffer_data` | `componentId`, `channel` | — (data is pushed asynchronously over the **Data API**, not returned inline — see §3) |

#### MIDI control mapping

| Action | Request fields | Response fields |
|---|---|---|
| `midi_learn` | — | Starts a MIDI-learn session: the next `NUM_LEARN_EVENTS_NEEDED` (3) CC events from the connected MIDI device are captured and auto-registered. Response comes back with `"status": "pending"` immediately, then the API pushes a follow-up `set_midi_control`-shaped response once learning completes. |
| `get_midi_control` | `componentId`, `parameter` | `value`: the MIDI CC number (0–127) routed to that parameter |
| `set_midi_control` | `componentId`, `parameter`, `value` (CC 0–127), `control_type` (optional: `"continuous"` \| `"discrete"`) | — |

### 2.4 Loading a patch (`load_patch`)

`load_patch` recreates an entire graph from a serialized description in one request, e.g.,:

```json
{
  "action": "load_patch",
  "components": [
    { "id": 0, "name": "Oscillator", "parameters": { "frequency": { "currentValue": 220.0 } } }
  ],
  "connections": [ /* same shape as create_connection requests */ ],
  "midi_controls": [ /* same shape as set_midi_control requests */ ]
}
```

Because a fresh graph reassigns component IDs, `load_patch` builds an old-ID → new-ID map (`IdMap`) as it creates each component, then rewrites every `componentId` reference present in the incoming json before applying them. Component IDs in the file you're loading do **not** need to match IDs already active in the running engine.

Per-component `parameters` entries may include `currentValue`, and/or a `minimumValue`/`maximumValue` pair to restore custom parameter ranges. `defaultValue` is also available for convenient parameter reset awareness.

### 2.5 Connections

A connection request has an `inbound` and `outbound` side, each with a `socketType`. More details on this implementation are available in `./shared/requests/ConnectionRequest.hpp`:

```json
{
  "action": "create_connection",
  "inbound":  { "componentId": 2, "index": 0, "socketType": "Signal Inbound" },
  "outbound": { "componentId": 1, "index": 0, "socketType": "Signal Outbound" }
}
```

Socket types must pair up correctly (`Signal Inbound` ↔ `Signal Outbound`, etc.) — the server routes the request based on the matched pair:

| Inbound socket | Outbound socket | Requires |
|---|---|---|
| `Signal Inbound` | `Signal Outbound` | `index` on both sides (audio channel indices) |
| `Buffer Inbound` | `Buffer Outbound` | `index` on both sides |
| `MIDI Inbound` | `MIDI Outbound` | — |
| `Modulation Inbound` | `Modulation Outbound` | `parameter` on the inbound side |

Notes from the examples in `debug/`:

- The **audio output device itself** is an implicit inbound endpoint — an `inbound` block with `index` but no `componentId` targets a physical output channel directly (see `test-midi-control.json`). 
- Modulation connections omit `index` but require `inbound.parameter`, naming which parameter on the inbound component is being modulated.
- `create_depth_connection` / `remove_depth_connection` use the same `inbound`/`outbound` shape but mark the connection as a "depth" connection (`depthConnection: true` server-side) — used for 2nd order modulation.

### 2.6 Collections

Components with a collection expose it in one of three structures (`shared/meta/CollectionDescriptor.hpp`):

| Structure | Shape of `value` | Example component |
|---|---|---|
| `INDEPENDENT` | a single number | — |
| `GROUPED` | a fixed-size array, N values per entry | `Midi Filter` (`groupSize = 2`, min/max MIDI note pass-through range) |
| `SYNCHRONIZED` | an object keyed by parameter name, all collections advance in lock-step | `Sequencer` (`midi value`, `midi velocity`, `start`, `duration`) |

Example — adding one note to a `Sequencer`'s event list:

```json
{
  "action": "add_collection_value",
  "componentId": 0,
  "value": { "midi value": 48, "midi velocity": 100, "start": 0.0, "duration": 1.0 }
}
```

`add_collection_value` returns the assigned `index` for later
`get`/`set`/`remove` calls against that entry.

### 2.7 Errors

Any handler that throws (`std::runtime_error` or similar) is caught centrally and converted into a `"status": "failed"` response carrying the exception message in `"error"` — the socket is never closed because of a bad request.

A malformed connection to the socket only closes if the peer disconnects or a real socket-level error occurs.

---

## 3. Data API

The Data API is **push-only**: clients connect and receive a stream of binary messages whenever the engine has buffer data to report -- currently triggered by a) `get_buffer_data` on the Control API, or b) an `AudioBufferComponent` publishing state. Anything a client sends on this socket is read and discarded (logged as unexpected).

### 3.1 Wire format

Each message is a fixed header immediately followed by a raw `double[]` payload, with no delimiter — length is derived from the header. See `./shared/requests/DataDescriptor.hpp` for the header definition.

Because messages have no boundary marker beyond the declared `size`, clients **must** read exactly `HEADER_SIZE` bytes, then exactly `size` bytes, looping on partial `recv()` (TCP gives no message-boundary guarantee) — see `recv_exact()` in `./debug/control-client.py` for a reference implementation.

### 3.2 Triggering a data push

```json
{ "action": "get_buffer_data", "componentId": 1, "channel": 0 }
```

This Control API request doesn't return the data inline — it responds with a normal success/fail status on the *control* socket, and the actual sample data is broadcast to all connected clients on the *data* socket.

---

## 4. Worked example: patching an oscillator to output

```jsonc
// 1. query available audio output devices
{ "action": "get_audio_devices"}

// 2. select an available audio output
{ "action": "set_audio_device", "device_id": 129 }

// 3. create component and set parameters
{ "action": "add_component", "name": "Oscillator" } // ->componentId 0

// 4. set parameters (optional, defaults are generally sensible)
{ "action": "set_parameter", "componentId": 0, "parameter": "frequency", "value": 220.0 }

// 5. connect components / to peripheral devices
{
  "action": "create_connection",
  "inbound":  { "index": 0, "socketType": "Signal Inbound" },
  "outbound": { "componentId": 0, "index": 0, "socketType": "Signal Outbound" }
}

// 4. Start audio
{ "action": "set_state", "state": "run" }

// 5. stop audio
{ "action": "set_state", "state": "stop" }
```

Sent line-by-line over the control socket (see `debug/control-client.py` and accompanying `json` files in that directory for complete runnable examples.

---

## 5. Quirks to be aware of

- **Broadcast-to-all-clients, no scoping.** Multiple simultaneous control clients receive every response to every request, not just their own. This is done by design as it is currently only used in a debugging context. However, should a multi-client environment (e.g., secondary controller) ever be supported, this will likely be re-addressed.

- **No authentication or TLS.** API sockets are unauthenticated   plaintext TCP, and the assumption is trusted localhost only. There are no plans to develop additional security protocols to run the backend or client remotely at this time.