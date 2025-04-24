cmake_minimum_required(VERSION 3.10)
project(NiXX32Emulator VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Set SDL2 paths manually
set(SDL2_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/SDL2-2.30.11/include")
set(SDL2_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/extern/SDL2-2.30.11/lib/x64/SDL2.lib")
set(SDL2_MAIN_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/extern/SDL2-2.30.11/lib/x64/SDL2main.lib")


# Find SDL2 package
# find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# Source files
set(SOURCES
    src/main.cpp
	src/core/NiXX32System.cpp
    src/core/M68000CPU.cpp
    src/core/Z80CPU.cpp
    src/core/MemoryManager.cpp
	src/graphics/GraphicsSystem.cpp
	src/graphics/BackgroundLayer.cpp
	src/graphics/Effects.cpp
	src/graphics/Sprite.cpp
	src/audio/AudioSystem.cpp
	src/audio/PCMPlayer.cpp
	src/audio/QSound.cpp
	src/audio/YM2151.cpp
	src/network/NetworkSystem.cpp
	src/input/InputSystem.cpp
	src/platform/SDLRenderer.cpp
    src/platform/SDLAudioOutput.cpp
	src/rom/ROMLoader.cpp
	src/security/SecuritySystem.cpp
	src/debug/CPUDebugger.cpp
	src/debug/Debugger.cpp
	src/debug/Logger.cpp
	src/debug/MemoryViewer.cpp
	src/util/Config.cpp
	src/util/FileSystem.cpp
)

# Header files
set(HEADERS
    include/EmulatorApp.h
	include/core/NiXX32System.h
    include/core/M68000CPU.h
    include/core/Z80CPU.h
    include/core/MemoryManager.h
	include/graphics/GraphicsSystem.h
	include/graphics/BackgroundLayer.h
	include/graphics/Effects.h
	include/graphics/Sprite.h
	include/audio/AudioSystem.h
	include/audio/PCMPlayer.h
	include/audio/QSound.h
	include/audio/YM2151.h
	include/network/NetworkSystem.h
	include/input/InputSystem.h
	include/platform/SDLRenderer.h
    include/platform/SDLAudioOutput.h
	include/rom/ROMLoader.h
	include/security/SecuritySystem.h
	include/debug/CPUDebugger.h
	include/debug/Debugger.h
	include/debug/Logger.h
	include/debug/MemoryViewer.h
	include/util/Config.h
	include/util/FileSystem.h
)

# Create executable
add_executable(nixx32 ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(nixx32 PRIVATE include)

# Link with SDL2 libraries
target_link_libraries(nixx32 PRIVATE ${SDL2_LIBRARY} ${SDL2_MAIN_LIBRARY})

# Install target
install(TARGETS nixx32 DESTINATION bin)

# Testing (optional)
option(BUILD_TESTS "Build the tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation (optional)
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        # Doxygen configuration file
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
                      ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
        
        # Add documentation target
        add_custom_target(docs
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    endif()
endif()

# Copy ROMs directory to build directory
add_custom_command(TARGET nixx32 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/roms $<TARGET_FILE_DIR:nixx32>/roms
)

# Output configuration information
message(STATUS "Configuration:")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
#message(STATUS "  SDL2 found: ${SDL2_FOUND}")
message(STATUS "  SDL2 libraries: ${SDL2_LIBRARY}, ${SDL2_MAIN_LIBRARY}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build docs: ${BUILD_DOCS}")