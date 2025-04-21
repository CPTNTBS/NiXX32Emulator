/**
 * BackgroundLayer.h
 * Background layer management for NiXX-32 arcade board emulation
 * 
 * This file defines the background layer system that handles tile-based
 * scrolling backgrounds with parallax effects for the NiXX-32 arcade hardware.
 */

 #pragma once

 #include <cstdint>
 #include <vector>
 #include <string>
 #include <memory>
 
 #include "MemoryManager.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class GraphicsSystem;
 
 /**
  * Tile attributes for background layers
  */
 struct TileAttributes {
	 uint16_t tileIndex;     // Index in the tile set
	 uint16_t paletteIndex;  // Palette to use for this tile
	 bool hFlip;             // Horizontal flip
	 bool vFlip;             // Vertical flip
	 uint8_t priority;       // Layer priority within the layer
 };
 
 /**
  * Background layer configuration
  */
 struct BackgroundConfig {
	 uint16_t width;         // Layer width in tiles
	 uint16_t height;        // Layer height in tiles
	 uint8_t tileWidth;      // Tile width in pixels
	 uint8_t tileHeight;     // Tile height in pixels
	 bool wrappingEnabled;   // Whether the layer wraps around
	 bool enabled;           // Whether the layer is enabled
	 uint8_t colorDepth;     // Color depth (bits per pixel)
	 uint16_t layerPriority; // Layer priority for inter-layer ordering
 };
 
 /**
  * Class for managing a single background layer
  */
 class BackgroundLayer {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  * @param layerIndex Index of this layer
	  */
	 BackgroundLayer(System& system, MemoryManager& memoryManager, Logger& logger, 
					uint8_t layerIndex);
	 
	 /**
	  * Destructor
	  */
	 ~BackgroundLayer();
	 
	 /**
	  * Initialize the background layer
	  * @param config Layer configuration
	  * @param tileDataBaseAddress Base address of tile data in VRAM
	  * @param tileMapBaseAddress Base address of tile map in VRAM
	  * @return True if initialization was successful
	  */
	 bool Initialize(const BackgroundConfig& config, 
					uint32_t tileDataBaseAddress, 
					uint32_t tileMapBaseAddress);
	 
	 /**
	  * Reset the layer to initial state
	  */
	 void Reset();
	 
	 /**
	  * Update layer state for the current frame
	  * @param deltaTime Time elapsed since last update in milliseconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Render the layer to a frame buffer
	  * @param frameBuffer Pointer to frame buffer to render into
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch (bytes per row)
	  */
	 void Render(uint32_t* frameBuffer, int width, int height, int pitch);
	 
	 /**
	  * Set the layer's scroll position
	  * @param scrollX Horizontal scroll position
	  * @param scrollY Vertical scroll position
	  */
	 void SetScrollPosition(int16_t scrollX, int16_t scrollY);
	 
	 /**
	  * Get the layer's scroll position
	  * @param scrollX Output parameter for horizontal scroll position
	  * @param scrollY Output parameter for vertical scroll position
	  */
	 void GetScrollPosition(int16_t& scrollX, int16_t& scrollY) const;
	 
	 /**
	  * Enable or disable the layer
	  * @param enabled True to enable, false to disable
	  */
	 void SetEnabled(bool enabled);
	 
	 /**
	  * Check if the layer is enabled
	  * @return True if enabled
	  */
	 bool IsEnabled() const;
	 
	 /**
	  * Set the layer's priority
	  * @param priority Priority value (higher = in front)
	  */
	 void SetPriority(uint16_t priority);
	 
	 /**
	  * Get the layer's priority
	  * @return Priority value
	  */
	 uint16_t GetPriority() const;
	 
	 /**
	  * Update layer data from VRAM
	  */
	 void UpdateFromVRAM();
	 
	 /**
	  * Get the layer's configuration
	  * @return Layer configuration
	  */
	 const BackgroundConfig& GetConfig() const;
	 
	 /**
	  * Get a tile's attributes at the specified position
	  * @param x X position in tile coordinates
	  * @param y Y position in tile coordinates
	  * @return Tile attributes
	  */
	 TileAttributes GetTileAttributes(uint16_t x, uint16_t y) const;
	 
	 /**
	  * Set a tile's attributes at the specified position
	  * @param x X position in tile coordinates
	  * @param y Y position in tile coordinates
	  * @param attributes New tile attributes
	  */
	 void SetTileAttributes(uint16_t x, uint16_t y, const TileAttributes& attributes);
	 
	 /**
	  * Get the layer's index
	  * @return Layer index
	  */
	 uint8_t GetLayerIndex() const;
	 
	 /**
	  * Set the parallax factor for this layer
	  * @param xFactor Horizontal parallax factor (1.0 = normal speed)
	  * @param yFactor Vertical parallax factor (1.0 = normal speed)
	  */
	 void SetParallaxFactor(float xFactor, float yFactor);
	 
	 /**
	  * Get the parallax factor for this layer
	  * @param xFactor Output parameter for horizontal parallax factor
	  * @param yFactor Output parameter for vertical parallax factor
	  */
	 void GetParallaxFactor(float& xFactor, float& yFactor) const;
	 
	 /**
	  * Handle control register write
	  * @param address Register address
	  * @param value Value to write
	  */
	 void HandleRegisterWrite(uint32_t address, uint16_t value);
	 
	 /**
	  * Read from control register
	  * @param address Register address
	  * @return Register value
	  */
	 uint16_t HandleRegisterRead(uint32_t address);
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Layer index
	 uint8_t m_layerIndex;
	 
	 // Layer configuration
	 BackgroundConfig m_config;
	 
	 // Layer enabled state
	 bool m_enabled;
	 
	 // Scroll position
	 int16_t m_scrollX;
	 int16_t m_scrollY;
	 
	 // Parallax factors
	 float m_parallaxFactorX;
	 float m_parallaxFactorY;
	 
	 // Base addresses in VRAM
	 uint32_t m_tileDataBaseAddress;
	 uint32_t m_tileMapBaseAddress;
	 
	 // Tile map data
	 std::vector<TileAttributes> m_tileMap;
	 
	 // Cached rendered tiles for performance
	 struct CachedTile {
		 uint16_t tileIndex;
		 uint16_t paletteIndex;
		 bool hFlip;
		 bool vFlip;
		 std::vector<uint8_t> pixelData;
	 };
	 std::vector<CachedTile> m_tileCache;
	 
	 // Hardware registers
	 struct {
		 uint16_t control;           // Layer control register
		 uint16_t scrollX;           // Horizontal scroll register
		 uint16_t scrollY;           // Vertical scroll register
		 uint16_t priority;          // Layer priority register
		 uint16_t tileDataAddress;   // Tile data address register
		 uint16_t tileMapAddress;    // Tile map address register
	 } m_registers;
	 
	 /**
	  * Decode a tile from VRAM
	  * @param tileIndex Index of the tile in the tile set
	  * @param paletteIndex Palette to use for this tile
	  * @param hFlip Whether to flip horizontally
	  * @param vFlip Whether to flip vertically
	  * @return Vector containing decoded pixel data
	  */
	 std::vector<uint8_t> DecodeTile(uint16_t tileIndex, uint16_t paletteIndex, 
									 bool hFlip, bool vFlip);
	 
	 /**
	  * Get cached tile data or decode if not in cache
	  * @param tileIndex Index of the tile in the tile set
	  * @param paletteIndex Palette to use for this tile
	  * @param hFlip Whether to flip horizontally
	  * @param vFlip Whether to flip vertically
	  * @return Pointer to pixel data
	  */
	 const uint8_t* GetTileData(uint16_t tileIndex, uint16_t paletteIndex, 
							   bool hFlip, bool vFlip);
	 
	 /**
	  * Calculate visible area of the layer
	  * @param viewportX Viewport X position
	  * @param viewportY Viewport Y position
	  * @param viewportWidth Viewport width
	  * @param viewportHeight Viewport height
	  * @param startTileX Output parameter for starting tile X
	  * @param startTileY Output parameter for starting tile Y
	  * @param endTileX Output parameter for ending tile X
	  * @param endTileY Output parameter for ending tile Y
	  */
	 void CalculateVisibleArea(int viewportX, int viewportY, 
							  int viewportWidth, int viewportHeight,
							  int& startTileX, int& startTileY, 
							  int& endTileX, int& endTileY);
	 
	 /**
	  * Convert a tile map index to tile attributes
	  * @param tileMapValue Raw value from the tile map
	  * @return Decoded tile attributes
	  */
	 TileAttributes DecodeTileMapValue(uint16_t tileMapValue);
	 
	 /**
	  * Convert tile attributes to a tile map value
	  * @param attributes Tile attributes
	  * @return Encoded tile map value
	  */
	 uint16_t EncodeTileMapValue(const TileAttributes& attributes);
	 
	 /**
	  * Update tile cache for frequently used tiles
	  */
	 void UpdateTileCache();
	 
	 /**
	  * Clear the tile cache
	  */
	 void ClearTileCache();
 };
 
 } // namespace NiXX32