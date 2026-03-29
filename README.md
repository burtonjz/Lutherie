# Syndesium

A fully modular synthesizer application that lets you build any synthesizer you want through a flexible, connection-based architecture.

## License

This project is licensed under the GNU Lesser General Public License v3.0 - 
see the [LICENSE](LICENSE) file for details.

## Third-Party Libraries

This project uses:
- Qt6 (LGPL v3)
- KDDockWidgets (LGPL v3)
- RtMidi (MIT-style)
- RtAudio (MIT-style)
- KissFFT (BSD 3-Clause)
- spdlog (MIT-style)
 
See [THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt) for complete license information.

## Overview

Syndesium is a standalone modular synthesis environment where you can create, connect, and configure components to design custom synthesizers.

![User Interface](assets/images/readme-gui.png)

## Features

- **Fully Modular Architecture**: Create and connect individual synthesis modules (oscillators, envelopes, filters, etc.) in any configuration
- **Polyphonic Synthesis**: Built-in support for polyphonic oscillators and voice management, no bounds on polyphony.
- **Flexible Modulation System**: Every parameter can be modulated by any source through the ParameterMap design
- **Visual Patch Bay**: Draw connections between modules using an intuitive Qt6 interface
- **MIDI Support**: Connect and use any MIDI device for performance control via RtMidi
- **Audio**: Select your desired audio output device with RtAudio support
- **Extensible Design**: Clean separation between frontend and backend enables easy addition of new modules and potential development onto other platforms

## Current Status

**Platform Support**: Linux (currently)

The codebase uses cross-platform libraries (Qt6, RTAudio, RtMidi) and is designed to be extensible to other platforms.

## Prerequisites

### Build Dependencies
- C++20 or later
- Qt6 development libraries
- RtAudio
- RtMidi
- CMake

### Runtime Requirements
- Linux operating system
- Audio system (ALSA, JACK, Pipewire)
- MIDI device (virtual keyboards are supported)

## Installation

```bash
git clone https://github.com/burtonjz/Syndesium.git
cd Syndesium

cmake -S . -B build
cd build
make
```

## Usage

```bash
# run backend executable
./build/synth/synth

# in another terminal, run front end executable
./build/gui/gui
```

1. **Launch Application**: Start Syndesium
2. **Configure Hardware output/input**: Select your audio output device and midi input device through the setup menu.
3. **Create Modules**: Add audio sources
4. **Create Modulators**: Add modulation sources
5. **Create Midi Components**: Add midi manipulation components
6. **Draw Connections**: Click and drag to connect module outputs to parameter inputs
7. **Play**: Hit play to start the audio loop, and use your MIDI controller or computer keyboard to play your custom synthesizer

## Project Structure

```
Syndesium/
├── gui/          # Qt6 frontend application
├── synth/        # backend synthesis engine
├── shared/       # shared definitions and configurations
```

## Development

### Adding New Components

See [Adding Components](docs/adding-components.md)

## Acknowledgments

Built with:
- [Qt6](https://www.qt.io/) - Cross-platform GUI framework
- [RtAudio](https://github.com/thestk/rtaudio) - Cross-platform audio I/O
- [RtMidi](https://github.com/thestk/rtmidi) - Cross-platform MIDI I/O
- [KissFFT](https://github.com/mborgerding/kissfft) - Fast Fourier Transform Library
- [spdlog](https://github.com/gabime/spdlog) - logging
---
