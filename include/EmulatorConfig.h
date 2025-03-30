// main.cpp - Entry point for the NiXX-32 emulator application
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <memory>
#include <SDL.h>

// Include our emulation components
#include "NiXX32System.h"
#include "SDLRenderer.h"
#include "SDLAudioOutput.h"
#include "InputSystem.h"
#include "Debugger.h"

// Configuration
struct EmulatorConfig {
    std::string romPath;
    bool isEnhancedModel;
    bool enableDebugger;
    bool fullscreen;
    int windowScale;
    int audioFrequency;
    int audioBufferSize;
    bool vsync;
    bool showFPS;
};

// Forward declarations
bool parseCommandLine(int argc, char** argv, EmulatorConfig& config);
void printUsage();

int main(int argc, char** argv) {
    // Default configuration
    EmulatorConfig config;
    config.romPath = "";
    config.isEnhancedModel = false;
    config.enableDebugger = false;
    config.fullscreen = false;
    config.windowScale = 3;  // 3x native resolution by default
    config.audioFrequency = 44100;
    config.audioBufferSize = 2048;
    config.vsync = true;
    config.showFPS = false;
    
    // Parse command-line arguments
    if (!parseCommandLine(argc, argv, config)) {
        printUsage();
        return 1;
    }
    
    // Check if ROM path was provided
    if (config.romPath.empty()) {
        std::cout << "Error: No ROM file specified. Use --rom to specify a ROM file." << std::endl;
        printUsage();
        return 1;
    }
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Setup window and renderer
    int windowWidth = GraphicsSystem::SCREEN_WIDTH * config.windowScale;
    int windowHeight = GraphicsSystem::SCREEN_HEIGHT * config.windowScale;
    
    SDL_Window* window = SDL_CreateWindow(
        "NiXX-32 Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight,
        config.fullscreen ? SDL_WINDOW_FULLSCREEN : 0
    );
    
    if (!window) {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create our renderer implementation
    std::unique_ptr<SDLRenderer> renderer = std::make_unique<SDLRenderer>(
        window, 
        GraphicsSystem::SCREEN_WIDTH, 
        GraphicsSystem::SCREEN_HEIGHT,
        config.vsync
    );
    
    // Create audio output implementation
    std::unique_ptr<SDLAudioOutput> audioOutput = std::make_unique<SDLAudioOutput>(
        config.audioFrequency, 
        config.audioBufferSize
    );
    
    // Create input system for handling controls
    std::unique_ptr<InputSystem> inputSystem = std::make_unique<InputSystem>();
    
    // Create the main emulation system
    std::unique_ptr<NiXX32System> system = std::make_unique<NiXX32System>(config.isEnhancedModel);
    
    // Set up components with the system
    system->setRenderer(renderer.get());
    system->setAudioOutput(audioOutput.get());
    system->setInputSystem(inputSystem.get());
    
    // Initialize the system
    if (!system->initialize()) {
        std::cerr << "Error initializing NiXX-32 system" << std::endl;
        return 1;
    }
    
    // Load the ROM
    if (!system->loadROM(config.romPath)) {
        std::cerr << "Error loading ROM file: " << config.romPath << std::endl;
        return 1;
    }
    
    // Optional: Create and attach debugger if enabled
    std::unique_ptr<Debugger> debugger;
    if (config.enableDebugger) {
        debugger = std::make_unique<Debugger>(system.get());
        debugger->initialize();
    }
    
    // Main emulation loop
    bool running = true;
    SDL_Event event;
    
    // Frame timing variables
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    double frameTime = 1.0 / 60.0; // Target 60fps
    int framesPerSecond = 0;
    int frameCounter = 0;
    auto lastFPSTime = std::chrono::high_resolution_clock::now();
    
    while (running) {
        // Process SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                // Handle key presses
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_F5) {
                    // Toggle debugger
                    if (debugger) {
                        debugger->toggleBreakpoint();
                    }
                } else if (event.key.keysym.sym == SDLK_F9) {
                    // Reset system
                    system->reset();
                }
                
                // Pass other key events to the input system
                inputSystem->handleKeyEvent(event.key);
            } else if (event.type == SDL_KEYUP) {
                // Pass key release events to the input system
                inputSystem->handleKeyEvent(event.key);
            } else if (event.type == SDL_CONTROLLERDEVICEADDED || 
                      event.type == SDL_CONTROLLERDEVICEREMOVED) {
                // Handle controller connection/disconnection
                inputSystem->handleControllerDeviceEvent(event.cdevice);
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN || 
                      event.type == SDL_CONTROLLERBUTTONUP) {
                // Handle controller button events
                inputSystem->handleControllerButtonEvent(event.cbutton);
            } else if (event.type == SDL_CONTROLLERAXISMOTION) {
                // Handle controller axis events
                inputSystem->handleControllerAxisEvent(event.caxis);
            }
        }
        
        // Run debugger if active
        if (debugger && debugger->isActive()) {
            debugger->update();
            if (debugger->isStepping()) {
                system->stepInstruction();
            }
        } else {
            // Run the emulation for one frame
            system->runFrame();
        }
        
        // Render the current frame
        renderer->render();
        
        // Calculate and limit frame rate
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = currentTime - lastFrameTime;
        double elapsedSeconds = elapsed.count();
        
        // Only sleep if we're running too fast
        if (elapsedSeconds < frameTime) {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>((frameTime - elapsedSeconds) * 1000)
            ));
        }
        
        // Update frame timing
        lastFrameTime = std::chrono::high_resolution_clock::now();
        frameCounter++;
        
        // Update FPS counter once per second
        std::chrono::duration<double> fpsElapsed = lastFrameTime - lastFPSTime;
        if (fpsElapsed.count() >= 1.0) {
            framesPerSecond = frameCounter;
            frameCounter = 0;
            lastFPSTime = lastFrameTime;
            
            if (config.showFPS) {
                std::string title = "NiXX-32 Emulator - FPS: " + std::to_string(framesPerSecond);
                SDL_SetWindowTitle(window, title.c_str());
            }
        }
    }
    
    // Clean up SDL
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}

// Parse command-line arguments
bool parseCommandLine(int argc, char** argv, EmulatorConfig& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--rom" && i + 1 < argc) {
            config.romPath = argv[++i];
        } else if (arg == "--enhanced") {
            config.isEnhancedModel = true;
        } else if (arg == "--debug") {
            config.enableDebugger = true;
        } else if (arg == "--fullscreen") {
            config.fullscreen = true;
        } else if (arg == "--scale" && i + 1 < argc) {
            config.windowScale = std::stoi(argv[++i]);
            if (config.windowScale < 1 || config.windowScale > 10) {
                std::cerr << "Invalid scale factor. Must be between 1 and 10." << std::endl;
                return false;
            }
        } else if (arg == "--audio-freq" && i + 1 < argc) {
            config.audioFrequency = std::stoi(argv[++i]);
        } else if (arg == "--audio-buffer" && i + 1 < argc) {
            config.audioBufferSize = std::stoi(argv[++i]);
        } else if (arg == "--no-vsync") {
            config.vsync = false;
        } else if (arg == "--show-fps") {
            config.showFPS = true;
        } else if (arg == "--help") {
            return false;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return false;
        }
    }
    
    return true;
}

// Display usage information
void printUsage() {
    std::cout << "NiXX-32 Emulator Usage:" << std::endl;
    std::cout << "  --rom FILE           Path to ROM file" << std::endl;
    std::cout << "  --enhanced           Emulate NiXX-32+ enhanced model" << std::endl;
    std::cout << "  --debug              Enable debugger" << std::endl;
    std::cout << "  --fullscreen         Run in fullscreen mode" << std::endl;
    std::cout << "  --scale N            Window scale factor (1-10)" << std::endl;
    std::cout << "  --audio-freq N       Audio frequency (default: 44100)" << std::endl;
    std::cout << "  --audio-buffer N     Audio buffer size (default: 2048)" << std::endl;
    std::cout << "  --no-vsync           Disable vertical sync" << std::endl;
    std::cout << "  --show-fps           Display frames per second" << std::endl;
    std::cout << "  --help               Display this help information" << std::endl;
}