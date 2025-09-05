# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a modern C++20 Matrix screensaver for Windows with DirectX 11/Direct2D hardware acceleration. The project uses CMake for building and includes support for mask images to control character density.

## Architecture

The codebase follows a modular design pattern:

- **MatrixScreensaver**: Main coordinator class that orchestrates rendering and settings
- **MatrixRenderer**: DirectX 11 + Direct2D rendering engine for hardware-accelerated graphics
- **SettingsManager**: Registry-based persistence for configuration options
- **ConfigDialog**: Windows configuration dialog with sliders and options
- **MaskLoader**: WIC image loading for PNG/JPG mask support
- **Common types**: Shared structures like `MatrixSettings`, `MatrixColumn`, and utilities

Key files:
- `src/main.cpp`: Entry point handling Windows screensaver protocol (`/c` config, `/p` preview, default screensaver mode)
- `src/common.h`: Shared types, Matrix character constants, and utility functions
- `CMakeLists.txt`: Modern CMake configuration targeting C++20 with DirectX dependencies

## Development Commands

### Building
```batch
build.bat
```
- Builds using Visual Studio 2022 x64 toolchain
- Requires Visual Studio Developer Command Prompt
- Outputs `MatrixScreensaver.scr` in `build/Release/`
- Automatically copies mask images to output directory

### Installation
```batch
install.bat
```
- Must be run as Administrator
- Copies `.scr` file and mask images to `System32`
- Automatically builds if screensaver doesn't exist

### Development Environment
- Requires Visual Studio 2022 with C++20 support
- Uses Windows SDK for DirectX 11/Direct2D/WIC APIs
- CMake 3.20+ for build configuration

## Key Technical Details

### DirectX Integration
The project uses modern DirectX APIs with COM smart pointers:
- DirectX 11 for hardware acceleration
- Direct2D for 2D rendering and text
- DirectWrite for font handling
- WIC for image loading

### Screensaver Protocol
Follows Windows screensaver conventions:
- `/c`: Configuration dialog mode
- `/p`: Preview mode (not implemented)
- Default: Full screensaver mode with multi-monitor support

### Settings System
Registry-based configuration stored in `HKCU\Software\MatrixScreensaver`:
- Speed, density, colors, fonts configurable
- Mask image path and usage settings
- Custom message support

### Current Status
The architecture is complete but has compilation issues with DirectX/WIC COM interfaces that need debugging. The foundation demonstrates professional C++ practices and modern design patterns.

## File Organization

```
src/
├── main.cpp              # Entry point and Windows message handling
├── matrix_screensaver.*  # Main coordinator class
├── matrix_renderer.*     # DirectX rendering engine  
├── settings_manager.*    # Registry persistence
├── config_dialog.*       # Configuration UI
├── mask_loader.*         # Image loading via WIC
├── common.h              # Shared types and constants
└── *.rc, resource.h      # Windows resources
```

## Build Artifacts

- Primary output: `build/Release/MatrixScreensaver.scr`
- Mask images: `mask.png` or `mask.jpg` (optional)
- Installation target: `%SystemRoot%\System32\`