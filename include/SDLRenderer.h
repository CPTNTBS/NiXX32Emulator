// SDLRenderer.h - SDL2-based renderer for the NiXX-32 emulator
#pragma once

#include <SDL.h>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
class NiXX32System;
class GraphicsSystem;

class SDLRenderer {
private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_frameTexture;
    
    // Dimensions and scaling
    int m_screenWidth;
    int m_screenHeight;
    int m_windowWidth;
    int m_windowHeight;
    float m_scaleX;
    float m_scaleY;
    
    // Buffer for pixel data
    std::vector<uint32_t> m_pixelBuffer;
    
    // Reference to the graphics system
    GraphicsSystem* m_graphicsSystem;
    NiXX32System* m_system;
    
    // Configuration
    bool m_vsync;
    bool m_fullscreen;
    bool m_showFPS;
    
    // Performance metrics
    uint32_t m_frameCount;
    uint32_t m_lastFPSTime;
    float m_currentFPS;
    
    // Debug visualization options
    bool m_showLayerBoundaries;
    bool m_showSpriteBoxes;
    int m_activeLayerMask;  // Bitmask for layer visibility

public:
    SDLRenderer(SDL_Window* window, int screenWidth, int screenHeight, bool vsync);
    ~SDLRenderer();
    
    // Initialization and configuration
    bool initialize();
    void setGraphicsSystem(GraphicsSystem* graphics) { m_graphicsSystem = graphics; }
    void setSystem(NiXX32System* system) { m_system = system; }
    
    // Main rendering functions
    void render();
    void clear();
    void present();
    
    // Frame buffer manipulation
    void updateFrameBuffer(const std::vector<uint32_t>& frameBuffer);
    
    // Debug visualization controls
    void toggleLayerBoundaries() { m_showLayerBoundaries = !m_showLayerBoundaries; }
    void toggleSpriteBoxes() { m_showSpriteBoxes = !m_showSpriteBoxes; }
    void setLayerVisibility(int layerIndex, bool visible);
    
    // Window management
    void setFullscreen(bool fullscreen);
    void setScale(float scale);
    void toggleVSync();
    void resizeWindow(int width, int height);
    
    // Getters
    int getScreenWidth() const { return m_screenWidth; }
    int getScreenHeight() const { return m_screenHeight; }
    float getCurrentFPS() const { return m_currentFPS; }
};

// SDLRenderer.cpp - Implementation of the SDL renderer
#include "SDLRenderer.h"
#include "GraphicsSystem.h"
#include "NiXX32System.h"
#include <iostream>

SDLRenderer::SDLRenderer(SDL_Window* window, int screenWidth, int screenHeight, bool vsync)
    : m_window(window)
    , m_renderer(nullptr)
    , m_frameTexture(nullptr)
    , m_screenWidth(screenWidth)
    , m_screenHeight(screenHeight)
    , m_windowWidth(0)
    , m_windowHeight(0)
    , m_scaleX(1.0f)
    , m_scaleY(1.0f)
    , m_graphicsSystem(nullptr)
    , m_system(nullptr)
    , m_vsync(vsync)
    , m_fullscreen(false)
    , m_showFPS(false)
    , m_frameCount(0)
    , m_lastFPSTime(0)
    , m_currentFPS(0.0f)
    , m_showLayerBoundaries(false)
    , m_showSpriteBoxes(false)
    , m_activeLayerMask(0xFF)  // All layers visible by default
{
    // Initialize the pixel buffer to the screen dimensions
    m_pixelBuffer.resize(screenWidth * screenHeight, 0);
    
    // Get the window size
    SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    
    // Calculate scaling factors
    m_scaleX = static_cast<float>(m_windowWidth) / static_cast<float>(m_screenWidth);
    m_scaleY = static_cast<float>(m_windowHeight) / static_cast<float>(m_screenHeight);
}

SDLRenderer::~SDLRenderer() {
    // Clean up SDL resources
    if (m_frameTexture) {
        SDL_DestroyTexture(m_frameTexture);
        m_frameTexture = nullptr;
    }
    
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    // Note: We don't destroy the window here as it was passed to us
}

bool SDLRenderer::initialize() {
    // Create the SDL renderer
    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
    if (m_vsync) {
        rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
    }
    
    m_renderer = SDL_CreateRenderer(m_window, -1, rendererFlags);
    if (!m_renderer) {
        std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Set logical size to maintain aspect ratio
    SDL_RenderSetLogicalSize(m_renderer, m_screenWidth, m_screenHeight);
    
    // Create texture for frame buffer
    m_frameTexture = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        m_screenWidth,
        m_screenHeight
    );
    
    if (!m_frameTexture) {
        std::cerr << "Failed to create frame texture: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize FPS counter
    m_lastFPSTime = SDL_GetTicks();
    m_frameCount = 0;
    m_currentFPS = 0.0f;
    
    return true;
}

void SDLRenderer::render() {
    // Clear the renderer
    clear();
    
    // If we have a graphics system, get the frame buffer
    if (m_graphicsSystem) {
        // Get the current frame buffer from the graphics system
        updateFrameBuffer(m_graphicsSystem->getFrameBuffer());
    }
    
    // Update the texture with the pixel buffer
    SDL_UpdateTexture(
        m_frameTexture,
        nullptr,
        m_pixelBuffer.data(),
        m_screenWidth * sizeof(uint32_t)
    );
    
    // Copy the texture to the renderer
    SDL_RenderCopy(m_renderer, m_frameTexture, nullptr, nullptr);
    
    // Draw debug visualizations if enabled
    if (m_showLayerBoundaries && m_graphicsSystem) {
        // Draw boundaries between background layers
        // This is a placeholder - would need actual layer data from graphics system
    }
    
    if (m_showSpriteBoxes && m_graphicsSystem) {
        // Draw bounding boxes around sprites
        // This is a placeholder - would need actual sprite data from graphics system
    }
    
    // Present the rendered frame
    present();
    
    // Update FPS counter
    m_frameCount++;
    uint32_t currentTime = SDL_GetTicks();
    uint32_t elapsed = currentTime - m_lastFPSTime;
    
    if (elapsed >= 1000) {
        m_currentFPS = 1000.0f * m_frameCount / static_cast<float>(elapsed);
        m_frameCount = 0;
        m_lastFPSTime = currentTime;
        
        if (m_showFPS) {
            std::string title = "NiXX-32 Emulator - FPS: " + std::to_string(static_cast<int>(m_currentFPS));
            SDL_SetWindowTitle(m_window, title.c_str());
        }
    }
}

void SDLRenderer::clear() {
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
}

void SDLRenderer::present() {
    SDL_RenderPresent(m_renderer);
}

void SDLRenderer::updateFrameBuffer(const std::vector<uint32_t>& frameBuffer) {
    // Copy the frame buffer data to our pixel buffer
    if (frameBuffer.size() == m_pixelBuffer.size()) {
        m_pixelBuffer = frameBuffer;
    } else {
        // Sizes don't match, do a safe copy
        size_t copySize = std::min(frameBuffer.size(), m_pixelBuffer.size());
        std::copy(frameBuffer.begin(), frameBuffer.begin() + copySize, m_pixelBuffer.begin());
    }
}

void SDLRenderer::setLayerVisibility(int layerIndex, bool visible) {
    if (layerIndex >= 0 && layerIndex < 8) {  // Assuming max 8 layers
        if (visible) {
            m_activeLayerMask |= (1 << layerIndex);
        } else {
            m_activeLayerMask &= ~(1 << layerIndex);
        }
    }
}

void SDLRenderer::setFullscreen(bool fullscreen) {
    m_fullscreen = fullscreen;
    SDL_SetWindowFullscreen(m_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    
    // Update window size and scaling factors
    SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    m_scaleX = static_cast<float>(m_windowWidth) / static_cast<float>(m_screenWidth);
    m_scaleY = static_cast<float>(m_windowHeight) / static_cast<float>(m_screenHeight);
}

void SDLRenderer::setScale(float scale) {
    // Resize the window based on the scale factor
    int newWidth = static_cast<int>(m_screenWidth * scale);
    int newHeight = static_cast<int>(m_screenHeight * scale);
    resizeWindow(newWidth, newHeight);
}

void SDLRenderer::toggleVSync() {
    m_vsync = !m_vsync;
    
    // Recreate the renderer with the new vsync setting
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
    }
    
    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
    if (m_vsync) {
        rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
    }
    
    m_renderer = SDL_CreateRenderer(m_window, -1, rendererFlags);
    
    // Recreate the texture
    if (m_frameTexture) {
        SDL_DestroyTexture(m_frameTexture);
    }
    
    m_frameTexture = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        m_screenWidth,
        m_screenHeight
    );
    
    // Set logical size to maintain aspect ratio
    SDL_RenderSetLogicalSize(m_renderer, m_screenWidth, m_screenHeight);
}

void SDLRenderer::resizeWindow(int width, int height) {
    SDL_SetWindowSize(m_window, width, height);
    m_windowWidth = width;
    m_windowHeight = height;
    m_scaleX = static_cast<float>(width) / static_cast<float>(m_screenWidth);
    m_scaleY = static_cast<float>(height) / static_cast<float>(m_screenHeight);
}