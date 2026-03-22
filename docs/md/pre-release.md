# Pre-Release Information

## Upcoming Release: v1.1.0 (Planned)

### Status
**Currently in Development** - Target release: Q1 2026

### Planned Features
- **Enhanced Format Support**
  - Support for additional image formats (GIF, HDR, etc.)
  - Custom output templates for pixel data
  - Support for 16-bit and 32-bit float images

- **Batch Processing & Export**
  - Process entire directories of images in a single command
  - Export pixel data to JSON, CSV, or XML formats
  - Recursive directory scanning

- **Advanced Querying & Filtering**
  - Filter pixels by color range or brightness
  - Define specific regions (rectangles/circles) for extraction
  - Sample every Nth pixel for downsampled data extraction

- **User Experience Improvements**
  - Progress bars for large image processing
  - Colored terminal output for color previews
  - Verbose logging and diagnostic modes
  - Support for reading from stdin

- **Platform-Specific Enhancements**
  - **Windows**: Improved resource handling and native terminal support
  - **Linux**: Optimized SIMD processing for faster extraction on supported architectures

### Testing & Feedback
This release is currently in development and testing phase. Early adopters and testers are welcome to try the development builds and provide feedback.

### Development Builds
Development builds are available through:
- **GitHub Actions**: Automated builds from the `develop` branch
- **Artifacts**: Downloadable from workflow runs

### Known Issues
- Very large images (>10k resolution) may consume significant memory
- Some exotic color profiles might not be perfectly mapped to RGB
- Transparency handling in certain legacy formats is still being refined

### Testing Priority Areas
We're particularly looking for feedback on:
1. Performance with high-resolution batch processing
2. Accuracy of BGR/RGB mapping in various formats
3. Memory usage during long-running operations
4. Cross-distro Linux compatibility

### How to Test Development Builds
```bash
# Clone the repository
git clone https://github.com/Lateralus138/imggetpixels.git
cd imggetpixels

# Switch to develop branch
git checkout develop

# Build from source (requires CMake and a C++20 compiler)
mkdir build && cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Test the new features
./imggetpixels --help
./imggetpixels --resolution my_image.png
./imggetpixels --rgb --coordinates my_image.png
```

### Contributing
We welcome contributions! See our [Contributing Guidelines](../../CONTRIBUTING.md) for details on:
- Code submission process
- Testing requirements
- Documentation standards
- Issue reporting guidelines

### Release Timeline
- **Alpha**: Current - Core feature development
- **Beta**: Late January 2026 - Feature-complete, public testing
- **RC**: Early February 2026 - Release candidate, bug fixes only
- **Release**: Mid-February 2026 - Stable v1.1.0 release

## Changelog

See [Changelog](./docs/md/reference/changelog.md)

---

## Current Stable Release

While v1.1.0 is in development, the current stable release is **v1.0.0**, available for download from the [Releases](https://github.com/Lateralus138/imggetpixels/releases) page.

### Upgrade Path
- Upgrading from v1.0.0 to v1.1.0 will be seamless
- Command-line arguments will remain backward compatible
- Performance improvements will be applied automatically

## Feedback Channels

### For Development Testing
- **GitHub Issues**: Report bugs and feature requests
- **GitHub Discussions**: General discussion and feedback

### Security Concerns
For security-related issues, please email: security@lateralus138.com

## Roadmap Beyond v1.1.0

### v1.2.0 (Planned Q2 2026)
- Dominant color extraction (palette generation)
- Basic image manipulation (resize/crop before extraction)
- Integrated CSS/SASS variable generator

### v1.3.0 (Planned Q3 2026)
- Python and Node.js bindings
- Plugin system for custom extraction logic
- WebAssembly version for browser-based toolsets

---

**Note**: Pre-release information is subject to change. Features and timelines may be adjusted based on development progress, testing feedback, and community needs.
