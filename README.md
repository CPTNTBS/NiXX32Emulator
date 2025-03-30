# NiXX-32 Emulator

An emulator for the Starsick NiXX-32 arcade system board from the early 1990s.

## Overview

The NiXX-32 Emulator recreates the hardware environment of the NiXX-32 and NiXX-32+ arcade system boards, allowing you to play arcade games made on the boards on modern hardware. This emulator aims to provide accurate emulation of:

- Motorola 68000 CPU (main processor)
- Zilog Z80 CPU (sound co-processor)
- Custom graphics hardware with sprite scaling and multiple background layers
- FM synthesis and PCM audio systems
- Original arcade controls and timing

## Features

- Full emulation of NiXX-32 (1989) and NiXX-32+ (1992) hardware variants
- Support for the various arcade games made on the board:
  - Telefunk (1989)
  - Telefunk 2 (1992)
  - Telefunk 3 (1993)
- Configurable video options (scaling, fullscreen, vsync)
- Customizable input mapping for keyboard and controllers
- Save states
- Debug visualization tools for graphics layers, sprites, and memory
- Frame rate limiting with speed control

## Requirements

- 64-bit Windows, macOS, or Linux
- OpenGL 3.3 compatible graphics card
- Controller recommended (but keyboard controls supported)

## Building from Source

### Dependencies

- CMake 3.10 or higher
- C++17 compatible compiler (MSVC, GCC, or Clang)
- SDL2 (2.30.11)

### Windows Build Instructions

1. Install Visual Studio 2019 or newer with C++ desktop development tools
2. Install CMake
3. Download SDL2 development libraries for Visual C++ from [SDL's website](https://github.com/libsdl-org/SDL/releases)
4. Extract SDL2 to `extern/SDL2-2.30.11` (or adjust CMakeLists.txt paths)
5. Create a build directory:
   ```
   mkdir build
   cd build
   ```
6. Generate project files:
   ```
   cmake ..
   ```
7. Build the project:
   ```
   cmake --build . --config Release
   ```
8. The executable will be in the `build/Release` directory

### macOS/Linux Build Instructions

1. Install required development tools:
   - macOS: `brew install cmake sdl2`
   - Ubuntu/Debian: `sudo apt install cmake build-essential libsdl2-dev`
   - Fedora: `sudo dnf install cmake gcc-c++ sdl2-devel`

2. Create a build directory:
   ```
   mkdir build
   cd build
   ```

3. Configure and build:
   ```
   cmake ..
   make -j$(nproc)
   ```

4. The executable will be in the `build` directory

## Usage

```
nixx32 [options] rompath
```

Options:
- `--fullscreen`: Start in fullscreen mode
- `--scale=N`: Set window scale factor (default: 3)
- `--novsync`: Disable vertical sync
- `--debug`: Enable debug visualizations
- `--help`: Display help information

Example:
```
nixx32 roms/telefunk2.rom
```

## Project Structure

```
NiXX32Emulator/
├── include/             # Header files
├── src/                 # Source code
├── roms/                # ROM files (not included)
├── docs/                # Documentation
├── tests/               # Unit tests
├── extern/              # External dependencies (not included in repo)
│   └── SDL2-2.30.11/    # SDL2 development files (to be added by user)
└── build/               # Build output (generated)
```

## ROM Files

This emulator requires original ROM files which are not included. Place your legally obtained ROM files in the `roms` directory.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- The SDL2 development team
- MAME project for hardware documentation references
- The arcade preservation community