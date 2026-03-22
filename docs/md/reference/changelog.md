# Changelog

All notable changes to Image Get Pixels will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-09

### Added
- **Initial release of Image Get Pixels** - A cross-platform command-line utility for extracting pixel data and resolution information from images.
- **Cross-platform support** - Built and distributed for:
  - **Windows** (x64) - Native Windows executable with optimized resource handling.
  - **Linux** (x64) - Native Linux binary compatible with modern distributions.
- **Command-line interface** - Simple and intuitive CLI for image querying operations.
- **Pixel Color Extraction** - Ability to retrieve colors in various formats:
  - **Hexadecimal**: Default format (e.g., FFFFFF or FFFFFFFF).
  - **RGB/RGBA**: Comma-separated decimal values.
  - **BGR**: Comma-separated decimal values in BGR order.
- **Resolution Querying** - Quickly retrieve image dimensions:
  - **Resolution**: Returns `<WIDTH>x<HEIGHT>`.
  - **Width Only**: Returns just the image width.
  - **Height Only**: Returns just the image height.
- **Formatting Options**:
  - **Opaque Mode**: Option to strip transparency values from output.
  - **Custom Prefixes**: Prepend strings (like "0x") to hexadecimal values.
- **Coordinate Mapping**: Optional format to associate colors with their `x,y` coordinates.
- **Lightweight design** - Minimal resource footprint and fast execution using the `stb_image` library.
- **No external dependencies** - Self-contained executables for each platform.

### Technical Details
- **Language**: C++20
- **Library**: `stb_image` for robust image decoding support.
- **Build System**: CMake with Ninja generator.
- **CI/CD**: Automated builds via GitHub Actions.

### Features
- Get image dimensions (width and height).
- Extract all pixel colors from an image.
- Map pixel colors to their coordinates.
- Support for multiple output formats (Hex, RGB, BGR).
- Custom prefixing for hexadecimal color values.
- Opaque output support for non-alpha transparency needs.

---

## [Unreleased]

### Planned - Definite
- Get the closest color of each pixel from list of colors (-C, --closest-list).
  - Listed values would a string delimited by a ';' so that both hexadecimal and rgb/bgr values could be passed (E.g. "55,127,255;255,0,127;...")
  - This could be used to generate copies of an image using a palette.
- Output (-O, --ouput) an image from the the retrieved colors.
  - E.g. if --opaque is provided the new file would 
  - E.g. if -C, --closest-list is used it would generate the new image using the palette.
  - Otherwise it just makes a copy.

### Planned - Considerations
- Batch processing for entire directories.
- Export functionality to JSON, CSV, and XML.
- Dominant color extraction and palette generation.
- Support for additional formats (GIF, HDR).
- Sub-region extraction (rectangles/circles).
- SIMD optimization for high-resolution images.
