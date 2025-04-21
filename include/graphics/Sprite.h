/**
 * Sprite.h
 * Sprite handling for NiXX-32 arcade board emulation
 * 
 * This file defines the sprite system that handles hardware sprites
 * including scaling, rotation, and collision detection for the NiXX-32
 * arcade hardware.
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
  * Sprite attributes
  */
 struct SpriteAttributes {
	 uint16_t x;             // X position on screen
	 uint16_t y;             // Y position on screen
	 uint16_t width;         // Sprite width in pixels
	 uint16_t height;        // Sprite height in pixels
	 uint16_t tileIndex;     // Index in the sprite tile set
	 uint16_t paletteIndex;  // Palette to use for this sprite
	 bool hFlip;             // Horizontal flip
	 bool vFlip;             // Vertical flip
	 uint8_t priority;       // Sprite priority for ordering
	 uint8_t scaleX;         // Horizontal scaling factor (fixed point)
	 uint8_t scaleY;         // Vertical scaling factor (fixed point)
	 uint8_t rotation;       // Rotation value (0-255 representing 0-359 degrees)
	 bool visible;           // Whether sprite is visible
	 uint8_t blendMode;      // Blend/transparency mode (NiXX-32+ only)
	 uint8_t effectFlags;    // Special effect flags (NiXX-32+ only)
 };
 
 /**
  * Sprite collision information
  */
 struct SpriteCollision {
	 uint16_t spriteIndex1;  // Index of first sprite in collision
	 uint16_t spriteIndex2;  // Index of second sprite in collision
	 uint16_t x;             // X position of collision
	 uint16_t y;             // Y position of collision
	 uint8_t intersectArea;  // Approximate area of intersection
 };
 
 /**
  * Sprite configuration
  */
 struct SpriteConfig {
	 uint8_t maxSpriteWidth;     // Maximum sprite width in pixels
	 uint8_t maxSpriteHeight;    // Maximum sprite height in pixels
	 uint16_t maxSprites;        // Maximum number of sprites
	 bool scalingSupported;      // Whether scaling is supported
	 bool rotationSupported;     // Whether rotation is supported
	 bool blendingSupported;     // Whether blending/transparency is supported
	 uint8_t colorDepth;         // Color depth (bits per pixel)
 };
 
 /**
  * Class for managing a single sprite
  */
 class Sprite {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  * @param spriteIndex Index of this sprite
	  */
	 Sprite(System& system, MemoryManager& memoryManager, Logger& logger, 
			uint16_t spriteIndex);
	 
	 /**
	  * Destructor
	  */
	 ~Sprite();
	 
	 /**
	  * Initialize the sprite
	  * @param config Sprite configuration
	  * @param spriteDataBaseAddress Base address of sprite data in VRAM
	  * @return True if initialization was successful
	  */
	 bool Initialize(const SpriteConfig& config, uint32_t spriteDataBaseAddress);
	 
	 /**
	  * Reset the sprite to initial state
	  */
	 void Reset();
	 
	 /**
	  * Update sprite state for the current frame
	  * @param deltaTime Time elapsed since last update in milliseconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Render the sprite to a frame buffer
	  * @param frameBuffer Pointer to frame buffer to render into
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch (bytes per row)
	  */
	 void Render(uint32_t* frameBuffer, int width, int height, int pitch);
	 
	 /**
	  * Set the sprite's attributes
	  * @param attributes New sprite attributes
	  */
	 void SetAttributes(const SpriteAttributes& attributes);
	 
	 /**
	  * Get the sprite's attributes
	  * @return Current sprite attributes
	  */
	 const SpriteAttributes& GetAttributes() const;
	 
	 /**
	  * Set the sprite's position
	  * @param x X position on screen
	  * @param y Y position on screen
	  */
	 void SetPosition(uint16_t x, uint16_t y);
	 
	 /**
	  * Get the sprite's position
	  * @param x Output parameter for X position
	  * @param y Output parameter for Y position
	  */
	 void GetPosition(uint16_t& x, uint16_t& y) const;
	 
	 /**
	  * Set the sprite's visibility
	  * @param visible True to make visible, false to hide
	  */
	 void SetVisible(bool visible);
	 
	 /**
	  * Check if the sprite is visible
	  * @return True if visible
	  */
	 bool IsVisible() const;
	 
	 /**
	  * Set the sprite's priority
	  * @param priority Priority value (higher = in front)
	  */
	 void SetPriority(uint8_t priority);
	 
	 /**
	  * Get the sprite's priority
	  * @return Priority value
	  */
	 uint8_t GetPriority() const;
	 
	 /**
	  * Set the sprite's scaling factors
	  * @param scaleX Horizontal scaling factor
	  * @param scaleY Vertical scaling factor
	  */
	 void SetScale(uint8_t scaleX, uint8_t scaleY);
	 
	 /**
	  * Get the sprite's scaling factors
	  * @param scaleX Output parameter for horizontal scaling
	  * @param scaleY Output parameter for vertical scaling
	  */
	 void GetScale(uint8_t& scaleX, uint8_t& scaleY) const;
	 
	 /**
	  * Set the sprite's rotation
	  * @param rotation Rotation value (0-255 representing 0-359 degrees)
	  */
	 void SetRotation(uint8_t rotation);
	 
	 /**
	  * Get the sprite's rotation
	  * @return Rotation value
	  */
	 uint8_t GetRotation() const;
	 
	 /**
	  * Set the sprite's tile index
	  * @param tileIndex Index in the sprite tile set
	  */
	 void SetTileIndex(uint16_t tileIndex);
	 
	 /**
	  * Get the sprite's tile index
	  * @return Tile index
	  */
	 uint16_t GetTileIndex() const;
	 
	 /**
	  * Set the sprite's palette index
	  * @param paletteIndex Index in the palette
	  */
	 void SetPaletteIndex(uint16_t paletteIndex);
	 
	 /**
	  * Get the sprite's palette index
	  * @return Palette index
	  */
	 uint16_t GetPaletteIndex() const;
	 
	 /**
	  * Set the sprite's flip attributes
	  * @param hFlip Horizontal flip
	  * @param vFlip Vertical flip
	  */
	 void SetFlip(bool hFlip, bool vFlip);
	 
	 /**
	  * Get the sprite's flip attributes
	  * @param hFlip Output parameter for horizontal flip
	  * @param vFlip Output parameter for vertical flip
	  */
	 void GetFlip(bool& hFlip, bool& vFlip) const;
	 
	 /**
	  * Set the sprite's blend mode (NiXX-32+ only)
	  * @param blendMode Blend mode value
	  */
	 void SetBlendMode(uint8_t blendMode);
	 
	 /**
	  * Get the sprite's blend mode
	  * @return Blend mode value
	  */
	 uint8_t GetBlendMode() const;
	 
	 /**
	  * Set the sprite's effect flags (NiXX-32+ only)
	  * @param effectFlags Effect flags value
	  */
	 void SetEffectFlags(uint8_t effectFlags);
	 
	 /**
	  * Get the sprite's effect flags
	  * @return Effect flags value
	  */
	 uint8_t GetEffectFlags() const;
	 
	 /**
	  * Check for collision with another sprite
	  * @param other Sprite to check collision with
	  * @param collision Output parameter for collision information
	  * @return True if sprites collide
	  */
	 bool CheckCollision(const Sprite& other, SpriteCollision& collision) const;
	 
	 /**
	  * Update sprite data from VRAM
	  */
	 void UpdateFromVRAM();
	 
	 /**
	  * Get the sprite's index
	  * @return Sprite index
	  */
	 uint16_t GetSpriteIndex() const;
	 
	 /**
	  * Get the sprite's configuration
	  * @return Sprite configuration
	  */
	 const SpriteConfig& GetConfig() const;
	 
	 /**
	  * Handle register write
	  * @param address Register address
	  * @param value Value to write
	  */
	 void HandleRegisterWrite(uint32_t address, uint16_t value);
	 
	 /**
	  * Read from register
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
	 
	 // Sprite index
	 uint16_t m_spriteIndex;
	 
	 // Sprite configuration
	 SpriteConfig m_config;
	 
	 // Sprite attributes
	 SpriteAttributes m_attributes;
	 
	 // Base address in VRAM
	 uint32_t m_spriteDataBaseAddress;
	 
	 // Cached sprite data for performance
	 struct CachedSpriteData {
		 uint16_t tileIndex;
		 uint16_t paletteIndex;
		 bool hFlip;
		 bool vFlip;
		 uint8_t scaleX;
		 uint8_t scaleY;
		 uint8_t rotation;
		 std::vector<uint8_t> pixelData;
	 };
	 CachedSpriteData m_cachedData;
	 
	 // Hardware registers
	 struct {
		 uint16_t control;       // Control register
		 uint16_t positionX;     // X position register
		 uint16_t positionY;     // Y position register
		 uint16_t attributes;    // Attributes register
		 uint16_t tileIndex;     // Tile index register
		 uint16_t scaling;       // Scaling register
		 uint16_t rotation;      // Rotation register
		 uint16_t effectControl; // Effect control register (NiXX-32+ only)
	 } m_registers;
	 
	 /**
	  * Decode sprite data from VRAM
	  * @param tileIndex Index of the tile in the sprite set
	  * @param paletteIndex Palette to use for this sprite
	  * @param hFlip Whether to flip horizontally
	  * @param vFlip Whether to flip vertically
	  * @param scaleX Horizontal scaling factor
	  * @param scaleY Vertical scaling factor
	  * @param rotation Rotation value
	  * @return Vector containing decoded pixel data
	  */
	 std::vector<uint8_t> DecodeSpriteData(uint16_t tileIndex, uint16_t paletteIndex,
										 bool hFlip, bool vFlip, 
										 uint8_t scaleX, uint8_t scaleY,
										 uint8_t rotation);
	 
	 /**
	  * Get cached sprite data or decode if not in cache
	  * @return Pointer to pixel data
	  */
	 const uint8_t* GetSpriteData();
	 
	 /**
	  * Convert attribute value to sprite attributes
	  * @param attributeValue Raw value from sprite attributes
	  * @return Decoded sprite attributes
	  */
	 SpriteAttributes DecodeAttributeValue(uint16_t attributeValue);
	 
	 /**
	  * Convert sprite attributes to attribute value
	  * @param attributes Sprite attributes
	  * @return Encoded attribute value
	  */
	 uint16_t EncodeAttributeValue(const SpriteAttributes& attributes);
	 
	 /**
	  * Update sprite cache when attributes change
	  */
	 void UpdateSpriteCache();
	 
	 /**
	  * Clear the sprite cache
	  */
	 void ClearSpriteCache();
	 
	 /**
	  * Apply scaling to sprite
	  * @param sourceData Source pixel data
	  * @param sourceWidth Source width
	  * @param sourceHeight Source height
	  * @param scaleX Horizontal scaling factor
	  * @param scaleY Vertical scaling factor
	  * @return Scaled pixel data
	  */
	 std::vector<uint8_t> ApplyScaling(const uint8_t* sourceData, 
									 uint16_t sourceWidth, uint16_t sourceHeight,
									 uint8_t scaleX, uint8_t scaleY);
	 
	 /**
	  * Apply rotation to sprite
	  * @param sourceData Source pixel data
	  * @param sourceWidth Source width
	  * @param sourceHeight Source height
	  * @param rotation Rotation value
	  * @return Rotated pixel data
	  */
	 std::vector<uint8_t> ApplyRotation(const uint8_t* sourceData,
									  uint16_t sourceWidth, uint16_t sourceHeight,
									  uint8_t rotation);
	 
	 /**
	  * Calculate collision rectangle
	  * @param rectX X coordinate of rectangle
	  * @param rectY Y coordinate of rectangle
	  * @param rectWidth Width of rectangle
	  * @param rectHeight Height of rectangle
	  */
	 void CalculateCollisionRect(uint16_t& rectX, uint16_t& rectY,
								uint16_t& rectWidth, uint16_t& rectHeight) const;
 };
 
 /**
  * Sprite system class that manages all sprites
  */
 class SpriteSystem {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 SpriteSystem(System& system, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~SpriteSystem();
	 
	 /**
	  * Initialize the sprite system
	  * @param config Sprite system configuration
	  * @param spriteTableBaseAddress Base address of sprite table in VRAM
	  * @param spriteDataBaseAddress Base address of sprite data in VRAM
	  * @return True if initialization was successful
	  */
	 bool Initialize(const SpriteConfig& config,
					uint32_t spriteTableBaseAddress,
					uint32_t spriteDataBaseAddress);
	 
	 /**
	  * Reset the sprite system to initial state
	  */
	 void Reset();
	 
	 /**
	  * Update all sprites for the current frame
	  * @param deltaTime Time elapsed since last update in milliseconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Render all sprites to a frame buffer
	  * @param frameBuffer Pointer to frame buffer to render into
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch (bytes per row)
	  */
	 void Render(uint32_t* frameBuffer, int width, int height, int pitch);
	 
	 /**
	  * Get a specific sprite
	  * @param index Sprite index
	  * @return Reference to sprite, or nullptr if invalid
	  */
	 Sprite* GetSprite(uint16_t index);
	 
	 /**
	  * Get all sprites
	  * @return Vector of all sprites
	  */
	 std::vector<Sprite*>& GetAllSprites();
	 
	 /**
	  * Update sprite data from VRAM
	  */
	 void UpdateFromVRAM();
	 
	 /**
	  * Process sprite collisions
	  * @return Vector of collision information
	  */
	 std::vector<SpriteCollision> ProcessCollisions();
	 
	 /**
	  * Handle register write
	  * @param address Register address
	  * @param value Value to write
	  */
	 void HandleRegisterWrite(uint32_t address, uint16_t value);
	 
	 /**
	  * Read from register
	  * @param address Register address
	  * @return Register value
	  */
	 uint16_t HandleRegisterRead(uint32_t address);
	 
	 /**
	  * Get the sprite system configuration
	  * @return Sprite system configuration
	  */
	 const SpriteConfig& GetConfig() const;
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Sprite configuration
	 SpriteConfig m_config;
	 
	 // All sprites in the system
	 std::vector<std::unique_ptr<Sprite>> m_sprites;
	 
	 // Quick access pointers to sprites
	 std::vector<Sprite*> m_spritePointers;
	 
	 // Base addresses in VRAM
	 uint32_t m_spriteTableBaseAddress;
	 uint32_t m_spriteDataBaseAddress;
	 
	 // Hardware registers
	 struct {
		 uint16_t control;           // Sprite system control register
		 uint16_t status;            // Sprite system status register
		 uint16_t collisionControl;  // Collision detection control
		 uint16_t collisionStatus;   // Collision status
	 } m_registers;
	 
	 /**
	  * Sort sprites by priority
	  */
	 void SortSpritesByPriority();
	 
	 /**
	  * Read sprite attributes from VRAM
	  * @param index Sprite index
	  * @return Sprite attributes
	  */
	 SpriteAttributes ReadSpriteAttributes(uint16_t index);
	 
	 /**
	  * Write sprite attributes to VRAM
	  * @param index Sprite index
	  * @param attributes Sprite attributes to write
	  */
	 void WriteSpriteAttributes(uint16_t index, const SpriteAttributes& attributes);
	 
	 /**
	  * Check if a sprite is active
	  * @param index Sprite index
	  * @return True if sprite is active
	  */
	 bool IsSpriteActive(uint16_t index) const;
	 
	 /**
	  * Handle collision interrupt
	  * @param collision Collision information
	  */
	 void HandleCollisionInterrupt(const SpriteCollision& collision);
 };
 
 } // namespace NiXX32