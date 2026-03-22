# Release Information

## Current Release: v1.0.0

### Release Date
January 9, 2026

### Overview
This is the initial stable release of Image Get Pixels, providing a cross-platform command-line utility for extracting pixel colors and resolution data from images on Windows and Linux.

### Downloads
- **Windows**: `imggetpixels-windows.exe` - Native Windows x64 executable
- **Linux**: `imggetpixels-linux` - Native Linux x64 binary

### System Requirements
- **Windows**: Windows 10 or later (x64)
- **Linux**: Any modern distribution (x64) with standard C++ libraries

### Installation
1. Download the appropriate binary for your operating system.
2. Place the executable in your preferred directory.
3. Add the directory to your system PATH (optional, for global access).
4. Run `imggetpixels --help` to confirm it is working correctly.

### Quick Start
```bash
# Show image resolution (WIDTHxHEIGHT)
imggetpixels --resolution my_image.png

# Get all pixel colors in hex format (default)
imggetpixels my_image.png

# Output colors in RGB with transparency
imggetpixels --rgb icon.png

# Get colors with coordinates (x,y: COLOR)
imggetpixels --coordinates screenshot.bmp

# Get opaque hex values with a "0x" prefix
imggetpixels -o -p "0x" logo.png

# Get only image width
imggetpixels -W texture.tga

# Get only image height
imggetpixels -H texture.tga
```

### Key Features
- **Fast Extraction**: Optimized for quick pixel querying using the `stb_image` library.
- **Flexible Formats**: Supports RGB, BGR, and hex formats with optional transparency.
- **Lightweight**: Small binary size with no heavy external library dependencies.
- **Coordinate Mapping**: Easily associate pixel data with image locations.
- **CLI-Focused**: Designed for piping and integration into larger script workflows.

### Security & Verification
All binaries are cryptographically signed and verified. SHA256 hashes are provided for integrity verification in the main README.

## Changelog

See [Changelog](./docs/md/reference/changelog.md)

---

## Previous Releases

No previous releases available - this is the initial release.

## Support

For issues, feature requests, or questions:
- [GitHub Issues](https://github.com/Lateralus138/imggetpixels/issues)
- [GitHub Discussions](https://github.com/Lateralus138/imggetpixels/discussions)

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](../../LICENSE) for details.
