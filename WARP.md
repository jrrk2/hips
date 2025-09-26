# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

Project summary
- C++17 project using Qt6 (Core, Network, Widgets) and HEALPix C++ for astronomical HiPS tiles and mosaics.
- Built with CMake; produces multiple executables for testing HiPS connectivity and creating mosaics (CLI and GUI variants).
- Outputs images and reports into build-time subfolders (m51_mosaic_tiles, messier_mosaics, enhanced_mosaics, hips_test_results).

Prerequisites (macOS)
- Qt6 via Homebrew: installed at /opt/homebrew/opt/qt6 (CMAKE_PREFIX_PATH is set in CMakeLists.txt)
- HEALPix C++: brew install healpix

Common commands
- Configure and build (Debug)
```sh path=null start=null
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```
- Configure and build (Release)
```sh path=null start=null
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
- List available targets/help from CMake-defined helper
```sh path=null start=null
cmake --build build --target help_build
```
- Run executables directly (after build)
```sh path=null start=null
./build/ProperHipsClient
./build/M51MosaicCreator
./build/MessierMosaicCreator
./build/EnhancedMosaicCreator
./build/SimpleHipsTest
```
- Run convenience targets that both build and run
```sh path=null start=null
cmake --build build --target test_hips        # Runs ProperHipsClient
cmake --build build --target create_m51       # Runs M51MosaicCreator
cmake --build build --target create_messier   # Runs MessierMosaicCreator
cmake --build build --target create_enhanced  # Runs EnhancedMosaicCreator
cmake --build build --target simple_test      # Runs SimpleHipsTest
```
- Single “test” run
  - This codebase doesn’t use a unit test framework; use the SimpleHipsTest target to validate URL generation and a tile request.
```sh path=null start=null
cmake --build build --target simple_test
# or run the executable directly
./build/SimpleHipsTest
```
- Generate and open an Xcode project (optional)
```sh path=null start=null
# Generate an Xcode project into xcode_build/
cmake -S . -B xcode_build -G Xcode
# Or via the helper targets
cmake --build build --target generate_xcode
cmake --build build --target open_xcode
```

Notes on configuration
- CMake config sets CMAKE_PREFIX_PATH=/opt/homebrew/opt/qt6 on macOS. If Qt is elsewhere, pass your own prefix path at configure time:
```sh path=null start=null
cmake -S . -B build -DCMAKE_PREFIX_PATH="/custom/qt/prefix"
```
- If HEALPix headers/libs aren’t found, CMake warns and falls back to including headers from the source dir. Prefer installing HEALPix via Homebrew.
- Compiler warnings are enabled (-Wall -Wextra). Build type flags: Debug (-g -O0), Release (-g -O3 -DNDEBUG).

High-level architecture
- Build system: CMake defines five executables and convenience targets in CMakeLists.txt
  - ProperHipsClient
  - M51MosaicCreator
  - MessierMosaicCreator
  - EnhancedMosaicCreator
  - SimpleHipsTest
  - Convenience targets: test_hips, create_m51, create_messier, create_enhanced, simple_test, and Xcode helpers.

- Core client (networking + HEALPix): ProperHipsClient.h/.cpp
  - Responsibilities
    - Defines HipsSurveyInfo, SkyPosition, TileResult types and maintains a survey registry.
    - Uses HEALPix (Healpix_Base) to compute HEALPix pixel indices from sky coordinates (ang2pix) at a given order.
    - Builds HiPS tile URLs per survey family (DSS, 2MASS, Rubin, generic) using the Norder/Dir/Npix pattern.
    - Performs HTTP GETs via QNetworkAccessManager, records timing/HTTP status/bytes, and appends results.
    - Provides helpers to find neighboring pixels and construct a proper 3x3 grid around a center pixel.
  - Key APIs for other components
    - calculateHealPixel(SkyPosition, order)
    - buildTileUrl(surveyName, position, order)
    - createProper3x3Grid(centerPixel, order)
    - testSurveyAtPosition(surveyName, position) for simple download checks

- Simple mosaic (CLI): main_m51_mosaic.cpp
  - Minimal QObject-based workflow creating a fixed 3×3 grid around a target (default is M51).
  - Downloads tiles sequentially, writes JPEGs under m51_mosaic_tiles, assembles a 1536×1536 PNG mosaic with crosshairs, and saves a progress report.
  - Accepts optional CLI args (RA, Dec, name, description) to override the default position.

- Messier mosaic (GUI): main_messier_mosaic.cpp (+ MessierCatalog.h)
  - QWidget-based UI to select any Messier object; shows metadata and builds a 3×3 tile grid centered on that object.
  - Caches and validates existing tile JPEGs, downloads missing ones, assembles a mosaic, and updates a preview.
  - Provides an optional “zoom to object size” view derived from catalog dimensions.

- Enhanced mosaic (GUI, coordinate-centered): main_enhanced_mosaic.cpp (+ MessierCatalog.h)
  - Tabbed UI for Messier-based or custom coordinates with inputs and live preview.
  - Robust coordinate parsing (sexagesimal and degrees), keyboard nudging (arrow keys with step sizes), and coordinate-centric cropping.
  - Produces output under enhanced_mosaics with reports and previews.

- M51 mosaic client (GUI scaffold): M51MosaicClient.h/.cpp
  - Widget and orchestration scaffolding for building an M51 mosaic with configurable orders, target resolution, and survey priority.
  - Demonstrates grid calculation, URL construction, and staged downloading; delegates network operations to ProperHipsClient.

- Data/catalog: MessierCatalog.h
  - Provides MessierObject data and helpers such as object type/constellation names. Used by Messier and Enhanced creators.

Outputs and folders
- At configure time CMake ensures these build output folders exist:
  - build/m51_mosaic_tiles
  - build/messier_mosaics
  - build/enhanced_mosaics
  - build/hips_test_results
- Mosaic creators save tiles, final mosaics (PNG/JPG), previews, and text reports into these folders.

Linting and tests
- No dedicated linter or unit test framework is configured in the repository.
- Use the SimpleHipsTest target and the program-specific convenience targets to validate behavior end-to-end.

Important bits from README
- The quick start aligns with the commands above:
```sh path=null start=null
mkdir build
cd build
cmake ..
make
./MessierMosaicCreator
```
