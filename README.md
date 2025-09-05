# Matrix Screensaver

[![Continuous Integration](https://github.com/SOELexicon/MatrixScreen/actions/workflows/ci.yml/badge.svg)](https://github.com/SOELexicon/MatrixScreen/actions/workflows/ci.yml)
[![Build and Release](https://github.com/SOELexicon/MatrixScreen/actions/workflows/release.yml/badge.svg)](https://github.com/SOELexicon/MatrixScreen/actions/workflows/release.yml)

A high-performance Matrix digital rain screensaver written in modern C++20 with DirectX 11 hardware acceleration.

## 🚀 Features

### Visual Effects
- **Authentic Matrix digital rain** with Japanese katakana characters
- **Hardware-accelerated rendering** using DirectX 11 + Direct2D
- **Mask image support** for 3D depth effects and custom shapes
- **Sequential character trails** that leave persistent messages
- **Variable font sizes** and color customization
- **Multi-monitor support** with independent settings

### Customization Options
- **Speed control** - Adjust falling character speed (1-20)
- **Density control** - Configure character density (10-300%)
- **Font selection** - Choose from Consolas, Courier New, Lucida Console, Cascadia Code, Terminal
- **Color hue adjustment** - Full spectrum color control (0-360°)
- **Message speed** - Control text display speed in mask areas (1-10)
- **Fade rate** - Adjust character fade speed (1-10)
- **Trail effects** - Enable/disable persistent character trails
- **3D depth mapping** - Variable font sizes based on mask brightness
- **Custom words** - Display custom messages in mask areas

### Technical Features
- **Registry-based settings** - Persistent configuration storage
- **Professional Windows integration** - Proper screensaver behavior
- **Mask background display** - Show mask images as translucent backgrounds
- **White head characters** - Bright leading characters option
- **Sequential vs random modes** - Control character selection behavior

## 📦 Installation

### Automatic Installation
1. Download the latest release from [Releases](https://github.com/yourusername/MatrixScreen/releases)
2. Extract the ZIP file
3. Run `install.bat` as Administrator
4. Configure via Windows Screen Saver settings

### Manual Installation
1. Copy `MatrixScreensaver.scr` to `C:\Windows\System32`
2. Right-click desktop → Personalize → Screen Saver
3. Select "Matrix Screensaver" from dropdown

## 🛠️ Building from Source

### Prerequisites
- **Visual Studio 2022** with C++ development tools
- **Windows 10/11 SDK**
- **CMake 3.20+**
- **Git**

### Build Steps
```batch
git clone https://github.com/yourusername/MatrixScreen.git
cd MatrixScreen
build.bat
```

The build script will:
1. Configure CMake with Visual Studio 2022
2. Build the Release configuration
3. Generate `MatrixScreensaver.scr` in `build\Release\`

### Development Build
```batch
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug
```

## 🎮 Usage

### Basic Configuration
1. Right-click desktop → Personalize → Screen Saver → Settings
2. Adjust speed, density, and color settings
3. Select font and enable desired effects
4. Click Preview to test changes

### Mask Images
1. Click "Browse..." to select PNG/JPG mask images
2. Enable "Show mask as background" for translucent overlay
3. Adjust opacity slider for background visibility
4. Use "Enable 3D depth mapping" for size-based depth effects

### Advanced Settings
- **Message Speed**: Controls text display speed in bright mask areas
- **Fade Rate**: How quickly characters fade out
- **Depth Range**: Intensity of 3D depth effects (1-20)
- **Variable Font Sizes**: Enable size variation based on depth
- **Sequential Characters**: Create persistent trailing messages

## 🔧 Technical Architecture

### Core Components
- **`MatrixRenderer`** - DirectX 11/Direct2D rendering engine
- **`SettingsManager`** - Registry-based configuration persistence  
- **`ConfigDialog`** - Windows settings dialog interface
- **`MaskLoader`** - WIC-based image loading and processing
- **`MatrixScreensaver`** - Main coordinator and Windows integration

### Key Technologies
- **C++20** with modern language features
- **DirectX 11** for hardware-accelerated graphics
- **Direct2D** for high-quality text rendering
- **WIC (Windows Imaging Component)** for image processing
- **CMake** for cross-platform build configuration
- **Windows Registry** for settings persistence

## 🚀 Releases & CI/CD

### Automated Builds
- **Continuous Integration** - Builds on every push/PR
- **Pre-releases** - Development builds from `develop` branch
- **Tagged Releases** - Stable releases from version tags

### Release Process
1. Create version tag: `git tag v1.2.3`
2. Push tag: `git push origin v1.2.3`  
3. GitHub Actions automatically builds and creates release

### Development Builds
- Available from [Actions](https://github.com/yourusername/MatrixScreen/actions) tab
- Pre-releases created automatically from `develop` branch
- Includes build info and commit details

## 📂 Project Structure

```
MatrixScreen/
├── .github/workflows/     # GitHub Actions CI/CD
├── src/                   # Source code
│   ├── main.cpp          # Entry point
│   ├── matrix_screensaver.*
│   ├── matrix_renderer.*
│   ├── config_dialog.*
│   ├── settings_manager.*
│   ├── mask_loader.*
│   ├── common.h          # Shared types
│   ├── resource.h        # Windows resources
│   └── *.rc             # Resource files
├── CMakeLists.txt        # Build configuration
├── build.bat            # Windows build script
├── install.bat          # Installation script
└── README.md            # This file
```

## 🐛 Known Issues

- Requires Windows 10/11 with DirectX 11 support
- Some antivirus software may flag screensaver files
- Mask images should be reasonable size for best performance

## 🤝 Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/amazing-feature`
3. Commit changes: `git commit -m 'Add amazing feature'`
4. Push to branch: `git push origin feature/amazing-feature`
5. Open Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by the Matrix movie trilogy
- Uses Japanese katakana characters for authentic Matrix feel
- Built with modern C++ and DirectX for optimal performance
