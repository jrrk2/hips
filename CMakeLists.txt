cmake_minimum_required(VERSION 3.16)
project(ProperHipsM51 VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS specific settings for your iMac
if(APPLE)
    # Choose a modern default to match Homebrew Qt/HEALPix builds, but allow override
    if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET OR "${CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "macOS deployment target")
    endif()
    # Homebrew Qt6 path - updated for your system
    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt6" ${CMAKE_PREFIX_PATH})
    # Homebrew include paths
    include_directories(/opt/homebrew/include)
    link_directories(/opt/homebrew/lib)
    
    # Xcode-specific settings
    if(CMAKE_GENERATOR STREQUAL "Xcode")
        set(CMAKE_XCODE_GENERATE_SCHEME ON)
        set(CMAKE_XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
        set(CMAKE_XCODE_SCHEME_ENABLE_GPU_API_VALIDATION ON)
        # Set proper bundle identifier for Xcode
        set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.astronomical.properhips")
    endif()
endif()

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)

# Auto-generate MOC files for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# HEALPix library detection
find_path(HEALPIX_INCLUDE_DIR 
    NAMES healpix_base.h pointing.h
    PATHS 
        /opt/homebrew/include/healpix_cxx
        /opt/homebrew/include
        /usr/local/include/healpix_cxx
        /usr/local/include
    DOC "HEALPix include directory"
)

find_library(HEALPIX_LIBRARY 
    NAMES healpix_cxx
    PATHS 
        /opt/homebrew/lib
        /usr/local/lib
    DOC "HEALPix library"
)

if(NOT HEALPIX_INCLUDE_DIR OR NOT HEALPIX_LIBRARY)
    message(WARNING "HEALPix library not found. Please install with: brew install healpix")
    # Fallback - assume headers are in current directory
    set(HEALPIX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Include directories
include_directories(${HEALPIX_INCLUDE_DIR})

# Source files
set(PROPER_HIPS_SOURCES
    ProperHipsClient.cpp
    ProperHipsClient.h
)

# Create the original ProperHipsClient executable
add_executable(ProperHipsClient
    main.cpp
    ${PROPER_HIPS_SOURCES}
)

target_link_libraries(ProperHipsClient
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
)

if(HEALPIX_LIBRARY)
    target_link_libraries(ProperHipsClient ${HEALPIX_LIBRARY})
endif()

# Create the M51 Mosaic Creator executable
add_executable(M51MosaicCreator
    main_m51_mosaic.cpp
    ${PROPER_HIPS_SOURCES}
    M51MosaicClient.cpp
    M51MosaicClient.h
)

target_link_libraries(M51MosaicCreator
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
)

if(HEALPIX_LIBRARY)
    target_link_libraries(M51MosaicCreator ${HEALPIX_LIBRARY})
endif()

# Create the Messier Mosaic Creator executable
add_executable(MessierMosaicCreator
    main_messier_mosaic.cpp
    ${PROPER_HIPS_SOURCES}
    M51MosaicClient.cpp
    M51MosaicClient.h
)

target_link_libraries(MessierMosaicCreator
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
)

if(HEALPIX_LIBRARY)
    target_link_libraries(MessierMosaicCreator ${HEALPIX_LIBRARY})
endif()

# Create the Enhanced Mosaic Creator executable (with arbitrary coordinates)
add_executable(EnhancedMosaicCreator
    main_enhanced_mosaic.cpp
    ${PROPER_HIPS_SOURCES}
    M51MosaicClient.cpp
    M51MosaicClient.h
)

target_link_libraries(EnhancedMosaicCreator
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
)

if(HEALPIX_LIBRARY)
    target_link_libraries(EnhancedMosaicCreator ${HEALPIX_LIBRARY})
endif()

# Create a simple test executable (minimal HiPS test)
add_executable(SimpleHipsTest
    simple_hips_test.cpp
    ${PROPER_HIPS_SOURCES}
)

target_link_libraries(SimpleHipsTest
    Qt6::Core
    Qt6::Network
)

if(HEALPIX_LIBRARY)
    target_link_libraries(SimpleHipsTest ${HEALPIX_LIBRARY})
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(ProperHipsClient PRIVATE -Wall -Wextra)
    target_compile_options(M51MosaicCreator PRIVATE -Wall -Wextra)
    target_compile_options(MessierMosaicCreator PRIVATE -Wall -Wextra)
    target_compile_options(EnhancedMosaicCreator PRIVATE -Wall -Wextra)
    target_compile_options(SimpleHipsTest PRIVATE -Wall -Wextra)
endif()

# Debug/Release configurations
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DNDEBUG")

# macOS specific linking
if(APPLE)
    # Link against system frameworks if needed
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(COCOA_FRAMEWORK Cocoa)
    
    if(FOUNDATION_FRAMEWORK AND COCOA_FRAMEWORK)
        target_link_libraries(ProperHipsClient ${FOUNDATION_FRAMEWORK} ${COCOA_FRAMEWORK})
        target_link_libraries(M51MosaicCreator ${FOUNDATION_FRAMEWORK} ${COCOA_FRAMEWORK})
        target_link_libraries(MessierMosaicCreator ${FOUNDATION_FRAMEWORK} ${COCOA_FRAMEWORK})
        target_link_libraries(EnhancedMosaicCreator ${FOUNDATION_FRAMEWORK} ${COCOA_FRAMEWORK})
    endif()
    
    # Set Xcode-specific properties for each target
    if(CMAKE_GENERATOR STREQUAL "Xcode")
        set_target_properties(ProperHipsClient PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        
        set_target_properties(M51MosaicCreator PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        
        set_target_properties(MessierMosaicCreator PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        
        set_target_properties(EnhancedMosaicCreator PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        
        set_target_properties(SimpleHipsTest PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()
endif()

# Installation targets
install(TARGETS ProperHipsClient M51MosaicCreator MessierMosaicCreator EnhancedMosaicCreator SimpleHipsTest
    RUNTIME DESTINATION bin
)

# Create output directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/m51_mosaic_tiles)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/messier_mosaics)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/enhanced_mosaics)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/hips_test_results)

# Print configuration summary
message(STATUS "")
message(STATUS "ProperHiPS M51 Mosaic Configuration Summary:")
message(STATUS "============================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "Qt6 Core: ${Qt6Core_VERSION}")
message(STATUS "Qt6 Network: ${Qt6Network_VERSION}")
message(STATUS "Qt6 Widgets: ${Qt6Widgets_VERSION}")
message(STATUS "HEALPix include: ${HEALPIX_INCLUDE_DIR}")
message(STATUS "HEALPix library: ${HEALPIX_LIBRARY}")
message(STATUS "")
message(STATUS "Executables to build:")
message(STATUS "  ProperHipsClient       - Original HiPS tester")
message(STATUS "  M51MosaicCreator       - M51 mosaic generator")
message(STATUS "  MessierMosaicCreator   - Messier object mosaics")
message(STATUS "  EnhancedMosaicCreator  - Custom coordinate mosaics")
message(STATUS "  SimpleHipsTest         - Minimal test program")
message(STATUS "")

# Custom targets for convenience
add_custom_target(test_hips
    COMMAND ${CMAKE_BINARY_DIR}/ProperHipsClient
    DEPENDS ProperHipsClient
    COMMENT "Running HiPS connectivity test"
)

add_custom_target(create_m51
    COMMAND ${CMAKE_BINARY_DIR}/M51MosaicCreator
    DEPENDS M51MosaicCreator
    COMMENT "Creating M51 mosaic"
)

add_custom_target(create_messier
    COMMAND ${CMAKE_BINARY_DIR}/MessierMosaicCreator
    DEPENDS MessierMosaicCreator
    COMMENT "Creating Messier object mosaics"
)

add_custom_target(create_enhanced
    COMMAND ${CMAKE_BINARY_DIR}/EnhancedMosaicCreator
    DEPENDS EnhancedMosaicCreator
    COMMENT "Creating enhanced coordinate mosaics"
)

add_custom_target(simple_test
    COMMAND ${CMAKE_BINARY_DIR}/SimpleHipsTest
    DEPENDS SimpleHipsTest
    COMMENT "Running simple HiPS test"
)

# Xcode project generation target
add_custom_target(generate_xcode
    COMMAND ${CMAKE_COMMAND} -G Xcode -B xcode_build -S ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating Xcode project in xcode_build/ directory"
    VERBATIM
)

add_custom_target(open_xcode
    COMMAND open xcode_build/ProperHipsM51.xcodeproj
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS generate_xcode
    COMMENT "Opening Xcode project"
    VERBATIM
)

# Help target
add_custom_target(help_build
    COMMAND echo "Available targets:"
    COMMAND echo "  make ProperHipsClient      - Build original HiPS client"
    COMMAND echo "  make M51MosaicCreator      - Build M51 mosaic creator"
    COMMAND echo "  make MessierMosaicCreator  - Build Messier mosaic creator"
    COMMAND echo "  make EnhancedMosaicCreator - Build enhanced mosaic creator"
    COMMAND echo "  make SimpleHipsTest        - Build simple test"
    COMMAND echo ""
    COMMAND echo "Run targets:"
    COMMAND echo "  make test_hips             - Build and run HiPS test"
    COMMAND echo "  make create_m51            - Build and run M51 creator"
    COMMAND echo "  make create_messier        - Build and run Messier creator"
    COMMAND echo "  make create_enhanced       - Build and run enhanced creator"
    COMMAND echo "  make simple_test           - Build and run simple test"
    COMMAND echo ""
    COMMAND echo "Xcode targets:"
    COMMAND echo "  make generate_xcode        - Generate Xcode project"
    COMMAND echo "  make open_xcode            - Generate and open Xcode project"
    COMMAND echo ""
    COMMAND echo "Other targets:"
    COMMAND echo "  make install               - Install all executables"
    COMMENT "Showing available build targets"
)

# Optional: Create a simple test source file if it doesn't exist
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/simple_hips_test.cpp)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/simple_hips_test.cpp
"// simple_hips_test.cpp - Minimal HiPS connectivity test
#include <QCoreApplication>
#include <QDebug>
#include \"ProperHipsClient.h\"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << \"Simple HiPS Test - Testing M51 position\";
    
    ProperHipsClient client;
    
    // Test M51 position with working survey
    SkyPosition m51 = {202.4695833, 47.1951667, \"M51\", \"Whirlpool Galaxy\"};
    QString url = client.buildTileUrl(\"DSS2_Color\", m51, 6);
    
    qDebug() << \"M51 test URL:\" << url;
    
    if (!url.isEmpty()) {
        qDebug() << \"✅ URL generation successful\";
        client.testSurveyAtPosition(\"DSS2_Color\", m51);
    } else {
        qDebug() << \"❌ URL generation failed\";
        return 1;
    }
    
    return app.exec();
}
"
    )
endif()

# Packaging (optional)
set(CPACK_PACKAGE_NAME "ProperHiPS-M51")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HiPS client for astronomical imaging with M51 mosaic capability")
set(CPACK_PACKAGE_VENDOR "Astronomical Software")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)