Matrix Screensaver with Mask Support (WPF Hardware Accelerated)
===============================================================

This screensaver creates a Matrix-style digital rain effect with support for mask images that control character density based on brightness levels.

FEATURES:
- Classic Matrix digital rain effect with Japanese katakana and numbers
- WPF with DirectX hardware acceleration for smooth performance
- Mask image support (PNG or JPG) - place as "mask.png" or "mask.jpg" in the same folder
- White areas in the mask = higher character density
- Black areas in the mask = lower character density
- Multi-monitor support
- Authentic Matrix color scheme (green characters with white heads)
- Smooth, flicker-free animation using GPU acceleration

INSTALLATION:
1. Place your mask image in the root folder as "mask.png" or "mask.jpg"
2. Run build.bat to compile the screensaver
3. Run install.bat as Administrator to install the screensaver
4. Go to Windows Display Settings > Screen Saver to select "Matrix Screensaver"

MASK IMAGE TIPS:
- Use high contrast images for best effect
- Black areas will have maximum character density
- White areas will have minimum character density
- The mask will be automatically scaled to fit your screen resolution

CONTROLS:
- Any mouse movement or key press will exit the screensaver

REQUIREMENTS:
- Windows with .NET 8.0 Runtime
- Administrator privileges for installation

TROUBLESHOOTING:
- If the mask image doesn't load, ensure it's named exactly "mask.png" or "mask.jpg"
- If installation fails, make sure you're running install.bat as Administrator
- The screensaver works without a mask image (uniform density)