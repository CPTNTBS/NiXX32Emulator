// GraphicsSystem.h - NiXX-32 Graphics Emulation
#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <memory>

class MemoryManager;
class Renderer;  // Abstract interface to your rendering API (SDL2, OpenGL, etc.)

class GraphicsSystem {
public:
    // Constants from the NiXX-32 specs
    static constexpr int SCREEN_WIDTH = 384;
    static constexpr int SCREEN_HEIGHT = 224;
    static constexpr int BASE_COLOR_DEPTH = 4096;
    static constexpr int ENHANCED_COLOR_DEPTH = 8192;
    static constexpr int BASE_MAX_SPRITES = 96;
    static constexpr int ENHANCED_MAX_SPRITES = 128;
    
    // Sprite attribute structure (maps to hardware registers)
    struct SpriteAttributes {
        uint16_t x;           // X position
        uint16_t y;           // Y position
        uint16_t tileIndex;   // Index into sprite tile memory
        uint16_t attributes;  // Flip, palette, priority bits
        uint16_t scaleX;      // X scaling factor (fixed point)
        uint16_t scaleY;      // Y scaling factor (fixed point)
        uint16_t rotation;    // Rotation angle (0-1023, 1024 = 360 degrees)
    };
    
    // Background layer structure
    struct BackgroundLayer {
        uint16_t scrollX;     // Horizontal scroll position
        uint16_t scrollY;     // Vertical scroll position
        uint16_t* tileMap;    // Pointer to tile map in VRAM
        uint16_t width;       // Width in tiles
        uint16_t height;      // Height in tiles
        uint16_t attributes;  // Layer attributes (priority, etc.)
    };

private:
    MemoryManager* m_memory;
    Renderer* m_renderer;
    bool m_isEnhancedModel;
    
    // Video RAM (512KB for base, 1MB for enhanced)
    std::vector<uint8_t> m_vram;
    
    // Color palettes
    std::vector<uint16_t> m_palettes;
    
    // Sprite data
    std::vector<SpriteAttributes> m_sprites;
    
    // Background layers (3 for base, 4 for enhanced)
    std::vector<BackgroundLayer> m_layers;
    
    // Framebuffer (final rendered output)
    std::vector<uint32_t> m_frameBuffer;
    
    // Internal rendering flags
    bool m_frameComplete;
    int m_scanline;

public:
    GraphicsSystem(MemoryManager* memory, Renderer* renderer, bool isEnhancedModel);
    ~GraphicsSystem();
    
    // System interface
    void initialize();
    void reset();
    
    // Rendering pipeline
    void startFrame();
    void renderScanline(int line);
    void endFrame();
    
    // Memory-mapped register access
    uint16_t readRegister(uint32_t address);
    void writeRegister(uint32_t address, uint16_t value);
    
    // Video RAM access
    uint8_t readVRAM8(uint32_t address);
    uint16_t readVRAM16(uint32_t address);
    void writeVRAM8(uint32_t address, uint8_t value);
    void writeVRAM16(uint32_t address, uint16_t value);
    
    // Frame information
    bool isFrameComplete() const { return m_frameComplete; }
    const std::vector<uint32_t>& getFrameBuffer() const { return m_frameBuffer; }
    
private:
    // Internal rendering functions
    void renderBackgroundLayers(int line);
    void renderSprites(int line);
    void renderForegroundLayer(int line);
    
    // Sprite transformation utilities
    void transformSprite(const SpriteAttributes& sprite, int& outWidth, int& outHeight);
    uint16_t getTransformedSpritePixel(const SpriteAttributes& sprite, int x, int y);
    
    // Color conversion from NiXX-32 format to RGBA8888
    uint32_t convertColor(uint16_t colorValue);
};