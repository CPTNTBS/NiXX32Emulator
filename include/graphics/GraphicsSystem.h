/**
 * GraphicsSystem.h
 * Graphics system for NiXX-32 arcade board emulation
 * 
 * This file defines the graphics system that handles sprite processing,
 * background layers, scaling effects, and rendering for the NiXX-32
 * arcade hardware.
 */

 #pragma once

 #include <cstdint>
 #include <memory>
 #include <vector>
 #include <array>
 #include <string>
 #include <functional>
 
 #include "MemoryManager.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class BackgroundLayer;
 class Sprite;
 class Effects;
 
 /**
  * Defines the hardware variant supported by the graphics system
  */
 enum class GraphicsHardwareVariant {
	 NIXX32_ORIGINAL,  // Original 1989 model (384×224, 4,096 colors, 96 sprites)
	 NIXX32_PLUS       // Enhanced 1992 model (384×224, 8,192 colors, 128 sprites)
 };
 
 /**
  * Represents a color in the NiXX-32 palette format
  */
 struct Color {
	 uint16_t value;  // 15-bit color value (5 bits each for R, G, B)
	 
	 // RGB component accessors
	 uint8_t GetR() const { return (value >> 10) & 0x1F; }
	 uint8_t GetG() const { return (value >> 5) & 0x1F; }
	 uint8_t GetB() const { return value & 0x1F; }
	 
	 // Create from RGB components
	 static Color FromRGB(uint8_t r, uint8_t g, uint8_t b) {
		 return { static_cast<uint16_t>((r & 0x1F) << 10 | (g & 0x1F) << 5 | (b & 0x1F)) };
	 }
 };
 
 /**
  * Graphics system configuration
  */
 struct GraphicsConfig {
	 uint16_t screenWidth;        // Screen width in pixels
	 uint16_t screenHeight;       // Screen height in pixels
	 uint16_t maxSprites;         // Maximum sprite count
	 uint16_t maxColors;          // Maximum number of colors
	 uint8_t backgroundLayers;    // Number of background layers
	 bool hasTransparency;        // Whether transparency effects are supported
	 bool hasRotation;            // Whether rotation is supported
 };
 
 /**
  * Main class that manages the NiXX-32 graphics system
  */
 class GraphicsSystem {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 GraphicsSystem(System& system, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~GraphicsSystem();
	 
	 /**
	  * Initialize the graphics system
	  * @param variant Hardware variant to configure for
	  * @return True if initialization was successful
	  */
	 bool Initialize(GraphicsHardwareVariant variant);
	 
	 /**
	  * Reset the graphics system to initial state
	  */
	 void Reset();
	 
	 /**
	  * Update graphics state for the current frame
	  * @param deltaTime Time elapsed since last update in milliseconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Render the current frame
	  * @param frameBuffer Pointer to frame buffer to render into
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch (bytes per row)
	  */
	 void Render(uint32_t* frameBuffer, int width, int height, int pitch);
	 
	 /**
	  * Update palette data from VRAM
	  */
	 void UpdatePalette();
	 
	 /**
	  * Configure the graphics system for a specific hardware variant
	  * @param variant Hardware variant to configure for
	  */
	 void ConfigureForVariant(GraphicsHardwareVariant variant);
	 
	 /**
	  * Get the current screen resolution
	  * @param width Output parameter for screen width
	  * @param height Output parameter for screen height
	  */
	 void GetScreenResolution(uint16_t& width, uint16_t& height) const;
	 
	 /**
	  * Get access to a specific background layer
	  * @param layerIndex Index of the layer to access
	  * @return Pointer to the background layer, or nullptr if invalid
	  */
	 BackgroundLayer* GetBackgroundLayer(uint8_t layerIndex);
	 
	 /**
	  * Get access to sprite system
	  * @return Reference to sprite system
	  */
	 std::vector<Sprite>& GetSprites();
	 
	 /**
	  * Get access to visual effects system
	  * @return Reference to effects system
	  */
	 Effects& GetEffectsSystem();
	 
	 /**
	  * Get a color from the current palette
	  * @param index Palette index
	  * @return Color value
	  */
	 Color GetPaletteColor(uint16_t index) const;
	 
	 /**
	  * Set a specific palette entry
	  * @param index Palette index
	  * @param color Color value
	  */
	 void SetPaletteColor(uint16_t index, Color color);
	 
	 /**
	  * Check if a specific graphics feature is supported
	  * @param featureName Name of the feature to check
	  * @return True if the feature is supported
	  */
	 bool IsFeatureSupported(const std::string& featureName) const;
	 
	 /**
	  * Get the hardware variant being emulated
	  * @return Current hardware variant
	  */
	 GraphicsHardwareVariant GetVariant() const;
	 
	 /**
	  * Get the current graphics configuration
	  * @return Graphics configuration
	  */
	 const GraphicsConfig& GetConfig() const;
	 
	 /**
	  * Register a callback for VRAM writes
	  * @param callback Function to call when VRAM is written
	  * @return True if callback was registered successfully
	  */
	 bool RegisterVRAMWriteCallback(std::function<void(uint32_t, uint16_t)> callback);
	 
	 /**
	  * Disable VRAM write callback
	  */
	 void DisableVRAMWriteCallback();
	 
	 /**
	  * Convert screen coordinates to layer coordinates
	  * @param screenX X coordinate in screen space
	  * @param screenY Y coordinate in screen space
	  * @param layerIndex Index of the layer
	  * @param layerX Output parameter for X coordinate in layer space
	  * @param layerY Output parameter for Y coordinate in layer space
	  * @return True if conversion was successful
	  */
	 bool ScreenToLayerCoordinates(int screenX, int screenY, uint8_t layerIndex, 
								   int& layerX, int& layerY) const;
	 
	 /**
	  * Get direct access to the frame buffer
	  * @return Pointer to internal frame buffer
	  */
	 uint32_t* GetFrameBuffer();
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Hardware variant currently configured for
	 GraphicsHardwareVariant m_variant;
	 
	 // Current graphics configuration
	 GraphicsConfig m_config;
	 
	 // Screen dimensions
	 uint16_t m_screenWidth;
	 uint16_t m_screenHeight;
	 
	 // Internal frame buffer
	 std::vector<uint32_t> m_frameBuffer;
	 
	 // Color palette
	 std::vector<Color> m_palette;
	 
	 // Background layers
	 std::vector<std::unique_ptr<BackgroundLayer>> m_backgroundLayers;
	 
	 // Sprites
	 std::vector<Sprite> m_sprites;
	 
	 // Visual effects system
	 std::unique_ptr<Effects> m_effects;
	 
	 // VRAM write callback
	 std::function<void(uint32_t, uint16_t)> m_vramWriteCallback;
	 
	 // Hardware-specific implementation details
	 struct {
		 // Base addresses for different graphics regions in VRAM
		 uint32_t paletteBaseAddress;
		 uint32_t spriteTableBaseAddress;
		 uint32_t bgLayerBaseAddresses[4];  // Up to 4 background layers
		 
		 // Hardware registers
		 uint16_t controlRegister;
		 uint16_t statusRegister;
		 uint16_t interruptRegister;
		 uint16_t displayEnable;
		 uint16_t spriteControl;
		 uint16_t bgScrollX[4];
		 uint16_t bgScrollY[4];
		 uint16_t bgControl[4];
	 } m_registers;
	 
	 /**
	  * Configure graphics system for original NiXX-32 hardware
	  */
	 void ConfigureOriginalHardware();
	 
	 /**
	  * Configure graphics system for enhanced NiXX-32+ hardware
	  */
	 void ConfigurePlusHardware();
	 
	 /**
	  * Process sprite data from VRAM
	  */
	 void ProcessSprites();
	 
	 /**
	  * Update background layer data from VRAM
	  */
	 void UpdateBackgroundLayers();
	 
	 /**
	  * Render background layers
	  * @param frameBuffer Target frame buffer
	  */
	 void RenderBackgroundLayers(uint32_t* frameBuffer);
	 
	 /**
	  * Render sprites
	  * @param frameBuffer Target frame buffer
	  */
	 void RenderSprites(uint32_t* frameBuffer);
	 
	 /**
	  * Apply post-processing effects
	  * @param frameBuffer Target frame buffer
	  */
	 void ApplyEffects(uint32_t* frameBuffer);
	 
	 /**
	  * Get VRAM pointer for a specific graphics region
	  * @param baseAddress Base address of the region
	  * @param size Size of the region in bytes
	  * @return Pointer to VRAM region, or nullptr if invalid
	  */
	 uint8_t* GetVRAMPointer(uint32_t baseAddress, uint32_t size);
	 
	 /**
	  * Handle video register write
	  * @param address Register address
	  * @param value Value to write
	  */
	 void HandleRegisterWrite(uint32_t address, uint16_t value);
	 
	 /**
	  * Read from video register
	  * @param address Register address
	  * @return Register value
	  */
	 uint16_t HandleRegisterRead(uint32_t address);
 };
 
 } // namespace NiXX32