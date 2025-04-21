/**
 * Effects.h
 * Special visual effects for NiXX-32 arcade board emulation
 * 
 * This file defines the special effects system that handles visual effects
 * such as scaling, rotation, palette manipulation, and particle systems
 * for the NiXX-32 arcade hardware.
 */

 #pragma once

 #include <cstdint>
 #include <memory>
 #include <vector>
 #include <unordered_map>
 #include <string>
 #include <functional>
 
 #include "MemoryManager.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class GraphicsSystem;
 
 /**
  * Effect types supported by the hardware
  */
 enum class EffectType {
	 NONE,               // No effect
	 FADE,               // Fade in/out
	 PALETTE_SWAP,       // Palette color cycling or swapping
	 WAVE,               // Wave distortion
	 SHAKE,              // Screen shake
	 VIGNETTE,           // Vignette/spotlight effect
	 SCANLINES,          // Scanline effect
	 BLUR,               // Blur effect (NiXX-32+ only)
	 PARTICLE,           // Particle system (NiXX-32+ only)
	 STROBOSCOPE,        // Stroboscope flashing (NiXX-32+ only)
	 CUSTOM              // Custom effect defined by game code
 };
 
 /**
  * Parameters for fade effect
  */
 struct FadeParams {
	 uint8_t targetAlpha;    // Target alpha value (0-255)
	 uint8_t currentAlpha;   // Current alpha value (0-255)
	 uint8_t speed;          // Fade speed (steps per frame)
	 bool fadeIn;            // True for fade in, false for fade out
	 uint16_t colorIndex;    // Color to fade to/from, or 0xFFFF for all colors
 };
 
 /**
  * Parameters for palette swap effect
  */
 struct PaletteSwapParams {
	 uint16_t startIndex;    // Start index in palette
	 uint16_t endIndex;      // End index in palette
	 uint8_t cycleSpeed;     // Cycle speed (steps per frame)
	 bool reverse;           // Reverse cycle direction
	 uint16_t frameCount;    // Number of frames in cycle
	 uint16_t currentFrame;  // Current frame in cycle
 };
 
 /**
  * Parameters for wave effect
  */
 struct WaveParams {
	 uint8_t amplitude;      // Wave amplitude
	 uint8_t frequency;      // Wave frequency
	 uint8_t speed;          // Wave speed
	 bool horizontal;        // True for horizontal wave, false for vertical
	 uint16_t offset;        // Current wave offset
 };
 
 /**
  * Parameters for shake effect
  */
 struct ShakeParams {
	 uint8_t intensity;      // Shake intensity
	 uint16_t duration;      // Shake duration in frames
	 uint16_t currentFrame;  // Current frame of shake
	 bool horizontal;        // True for horizontal shake, false for vertical
	 bool both;              // True for both directions
 };
 
 /**
  * Parameters for vignette effect
  */
 struct VignetteParams {
	 uint16_t centerX;       // Center X position
	 uint16_t centerY;       // Center Y position
	 uint16_t radius;        // Vignette radius
	 uint8_t intensity;      // Vignette intensity
 };
 
 /**
  * Parameters for scanline effect
  */
 struct ScanlineParams {
	 uint8_t intensity;      // Scanline intensity
	 uint8_t spacing;        // Spacing between scanlines
	 bool alternateCols;     // Also apply to columns for grid effect
 };
 
 /**
  * Parameters for blur effect (NiXX-32+ only)
  */
 struct BlurParams {
	 uint8_t radius;         // Blur radius
	 uint8_t intensity;      // Blur intensity
	 bool horizontal;        // Apply horizontal blur
	 bool vertical;          // Apply vertical blur
 };
 
 /**
  * Particle definition (NiXX-32+ only)
  */
 struct Particle {
	 float x;                // X position
	 float y;                // Y position
	 float velocityX;        // X velocity
	 float velocityY;        // Y velocity
	 float accelerationX;    // X acceleration
	 float accelerationY;    // Y acceleration
	 uint8_t size;           // Particle size
	 uint16_t colorIndex;    // Particle color index
	 uint16_t lifetime;      // Total lifetime in frames
	 uint16_t currentFrame;  // Current frame
	 float alpha;            // Particle alpha (0.0-1.0)
	 bool active;            // Whether particle is active
 };
 
 /**
  * Parameters for particle system (NiXX-32+ only)
  */
 struct ParticleSystemParams {
	 uint16_t emitterX;          // Emitter X position
	 uint16_t emitterY;          // Emitter Y position
	 uint16_t emitterWidth;      // Emitter width
	 uint16_t emitterHeight;     // Emitter height
	 uint8_t particlesPerFrame;  // Particles to emit per frame
	 uint16_t maxParticles;      // Maximum particles in system
	 uint16_t minLifetime;       // Minimum particle lifetime
	 uint16_t maxLifetime;       // Maximum particle lifetime
	 uint8_t minSize;            // Minimum particle size
	 uint8_t maxSize;            // Maximum particle size
	 float minVelocityX;         // Minimum X velocity
	 float maxVelocityX;         // Maximum X velocity
	 float minVelocityY;         // Minimum Y velocity
	 float maxVelocityY;         // Maximum Y velocity
	 float gravity;              // Gravity effect
	 uint16_t startColorIndex;   // Start color index in palette
	 uint16_t endColorIndex;     // End color index in palette
	 bool fadeOut;               // Whether particles fade out over lifetime
	 std::vector<Particle> particles; // Active particles
 };
 
 /**
  * Parameters for stroboscope effect (NiXX-32+ only)
  */
 struct StroboscopeParams {
	 uint8_t frequency;      // Flash frequency
	 uint8_t duration;       // Effect duration in frames
	 uint16_t colorIndex;    // Flash color index
	 uint8_t currentFrame;   // Current frame in cycle
	 bool active;            // Whether effect is active
 };
 
 /**
  * Parameters for custom effect
  */
 struct CustomEffectParams {
	 uint16_t effectId;      // Custom effect ID
	 uint16_t param1;        // Parameter 1
	 uint16_t param2;        // Parameter 2
	 uint16_t param3;        // Parameter 3
	 uint16_t param4;        // Parameter 4
	 uint8_t* dataPtr;       // Pointer to effect data
 };
 
 /**
  * Effect definition
  */
 struct Effect {
	 EffectType type;        // Effect type
	 bool active;            // Whether effect is active
	 uint16_t priority;      // Effect priority (higher = applied later)
	 
	 // Effect parameters (union to save memory)
	 union {
		 FadeParams fade;
		 PaletteSwapParams paletteSwap;
		 WaveParams wave;
		 ShakeParams shake;
		 VignetteParams vignette;
		 ScanlineParams scanline;
		 BlurParams blur;
		 ParticleSystemParams particleSystem;
		 StroboscopeParams stroboscope;
		 CustomEffectParams custom;
	 } params;
 };
 
 /**
  * Effects system configuration
  */
 struct EffectsConfig {
	 uint8_t maxActiveEffects;       // Maximum number of concurrent effects
	 bool particleSystemSupported;   // Whether particle systems are supported
	 bool blurEffectSupported;       // Whether blur effect is supported
	 bool customEffectsSupported;    // Whether custom effects are supported
	 uint16_t maxParticles;          // Maximum number of particles
 };
 
 /**
  * Class for managing special visual effects
  */
 class Effects {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 Effects(System& system, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~Effects();
	 
	 /**
	  * Initialize the effects system
	  * @param variant Hardware variant to configure for
	  * @return True if initialization was successful
	  */
	 bool Initialize(GraphicsHardwareVariant variant);
	 
	 /**
	  * Reset the effects system to initial state
	  */
	 void Reset();
	 
	 /**
	  * Update effects for the current frame
	  * @param deltaTime Time elapsed since last update in milliseconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Apply active effects to the frame buffer
	  * @param frameBuffer Pointer to frame buffer
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch (bytes per row)
	  */
	 void ApplyEffects(uint32_t* frameBuffer, int width, int height, int pitch);
	 
	 /**
	  * Create a new effect
	  * @param type Effect type
	  * @param priority Effect priority
	  * @return Effect ID, or -1 if failed
	  */
	 int CreateEffect(EffectType type, uint16_t priority = 0);
	 
	 /**
	  * Activate an effect
	  * @param effectId Effect ID
	  * @param active True to activate, false to deactivate
	  * @return True if successful
	  */
	 bool SetEffectActive(int effectId, bool active);
	 
	 /**
	  * Get effect by ID
	  * @param effectId Effect ID
	  * @return Pointer to effect, or nullptr if not found
	  */
	 Effect* GetEffect(int effectId);
	 
	 /**
	  * Remove an effect
	  * @param effectId Effect ID
	  * @return True if successful
	  */
	 bool RemoveEffect(int effectId);
	 
	 /**
	  * Check if the system supports a specific effect type
	  * @param type Effect type to check
	  * @return True if supported
	  */
	 bool IsEffectSupported(EffectType type) const;
	 
	 /**
	  * Get the effects system configuration
	  * @return Effects configuration
	  */
	 const EffectsConfig& GetConfig() const;
	 
	 /**
	  * Configure fade effect parameters
	  * @param effectId Effect ID
	  * @param params Fade parameters
	  * @return True if successful
	  */
	 bool ConfigureFade(int effectId, const FadeParams& params);
	 
	 /**
	  * Configure palette swap effect parameters
	  * @param effectId Effect ID
	  * @param params Palette swap parameters
	  * @return True if successful
	  */
	 bool ConfigurePaletteSwap(int effectId, const PaletteSwapParams& params);
	 
	 /**
	  * Configure wave effect parameters
	  * @param effectId Effect ID
	  * @param params Wave parameters
	  * @return True if successful
	  */
	 bool ConfigureWave(int effectId, const WaveParams& params);
	 
	 /**
	  * Configure shake effect parameters
	  * @param effectId Effect ID
	  * @param params Shake parameters
	  * @return True if successful
	  */
	 bool ConfigureShake(int effectId, const ShakeParams& params);
	 
	 /**
	  * Configure vignette effect parameters
	  * @param effectId Effect ID
	  * @param params Vignette parameters
	  * @return True if successful
	  */
	 bool ConfigureVignette(int effectId, const VignetteParams& params);
	 
	 /**
	  * Configure scanline effect parameters
	  * @param effectId Effect ID
	  * @param params Scanline parameters
	  * @return True if successful
	  */
	 bool ConfigureScanline(int effectId, const ScanlineParams& params);
	 
	 /**
	  * Configure blur effect parameters (NiXX-32+ only)
	  * @param effectId Effect ID
	  * @param params Blur parameters
	  * @return True if successful
	  */
	 bool ConfigureBlur(int effectId, const BlurParams& params);
	 
	 /**
	  * Configure particle system parameters (NiXX-32+ only)
	  * @param effectId Effect ID
	  * @param params Particle system parameters
	  * @return True if successful
	  */
	 bool ConfigureParticleSystem(int effectId, const ParticleSystemParams& params);
	 
	 /**
	  * Configure stroboscope effect parameters (NiXX-32+ only)
	  * @param effectId Effect ID
	  * @param params Stroboscope parameters
	  * @return True if successful
	  */
	 bool ConfigureStroboscope(int effectId, const StroboscopeParams& params);
	 
	 /**
	  * Configure custom effect parameters
	  * @param effectId Effect ID
	  * @param params Custom effect parameters
	  * @return True if successful
	  */
	 bool ConfigureCustomEffect(int effectId, const CustomEffectParams& params);
	 
	 /**
	  * Register a callback for a custom effect
	  * @param effectId Custom effect ID
	  * @param callback Function to call when applying the effect
	  * @return True if successful
	  */
	 bool RegisterCustomEffectCallback(uint16_t effectId, 
									 std::function<void(uint32_t*, int, int, int, const CustomEffectParams&)> callback);
	 
	 /**
	  * Remove a custom effect callback
	  * @param effectId Custom effect ID
	  * @return True if successful
	  */
	 bool RemoveCustomEffectCallback(uint16_t effectId);
	 
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
	 
	 // Effects configuration
	 EffectsConfig m_config;
	 
	 // Active effects
	 std::vector<Effect> m_effects;
	 
	 // Next available effect ID
	 int m_nextEffectId;
	 
	 // Effect ID to index mapping
	 std::unordered_map<int, size_t> m_effectMap;
	 
	 // Custom effect callbacks
	 std::unordered_map<uint16_t, std::function<void(uint32_t*, int, int, int, const CustomEffectParams&)>> m_customEffectCallbacks;
	 
	 // Hardware registers
	 struct {
		 uint16_t control;           // Effects control register
		 uint16_t status;            // Effects status register
		 uint16_t effectSelect;      // Selected effect
		 uint16_t effectParams[4];   // Effect parameters
	 } m_registers;
	 
	 // Hardware variant currently configured for
	 GraphicsHardwareVariant m_variant;
	 
	 /**
	  * Configure effects system for original NiXX-32 hardware
	  */
	 void ConfigureOriginalHardware();
	 
	 /**
	  * Configure effects system for enhanced NiXX-32+ hardware
	  */
	 void ConfigurePlusHardware();
	 
	 /**
	  * Apply fade effect
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyFadeEffect(const Effect& effect, uint32_t* frameBuffer, 
						 int width, int height, int pitch);
	 
	 /**
	  * Apply palette swap effect
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyPaletteSwapEffect(const Effect& effect, uint32_t* frameBuffer, 
								int width, int height, int pitch);
	 
	 /**
	  * Apply wave effect
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyWaveEffect(const Effect& effect, uint32_t* frameBuffer, 
						 int width, int height, int pitch);
	 
	 /**
	  * Apply shake effect
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyShakeEffect(const Effect& effect, uint32_t* frameBuffer, 
						  int width, int height, int pitch);
	 
	 /**
	  * Apply vignette effect
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyVignetteEffect(const Effect& effect, uint32_t* frameBuffer, 
							 int width, int height, int pitch);
	 
	 /**
	  * Apply scanline effect
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyScanlineEffect(const Effect& effect, uint32_t* frameBuffer, 
							 int width, int height, int pitch);
	 
	 /**
	  * Apply blur effect (NiXX-32+ only)
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyBlurEffect(const Effect& effect, uint32_t* frameBuffer, 
						 int width, int height, int pitch);
	 
	 /**
	  * Update and render particle system (NiXX-32+ only)
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyParticleSystem(Effect& effect, uint32_t* frameBuffer, 
							int width, int height, int pitch);
	 
	 /**
	  * Apply stroboscope effect (NiXX-32+ only)
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyStroboscopeEffect(Effect& effect, uint32_t* frameBuffer, 
								int width, int height, int pitch);
	 
	 /**
	  * Apply custom effect
	  * @param effect Effect to apply
	  * @param frameBuffer Frame buffer to apply to
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch
	  */
	 void ApplyCustomEffect(const Effect& effect, uint32_t* frameBuffer, 
						   int width, int height, int pitch);
	 
	 /**
	  * Sort effects by priority
	  */
	 void SortEffectsByPriority();
	 
	 /**
	  * Find effect index by ID
	  * @param effectId Effect ID
	  * @return Index in the effects vector, or -1 if not found
	  */
	 int FindEffectIndex(int effectId);
	 
	 /**
	  * Create a new particle (NiXX-32+ only)
	  * @param params Particle system parameters
	  * @return Initialized particle
	  */
	 Particle CreateParticle(const ParticleSystemParams& params);
	 
	 /**
	  * Update particle (NiXX-32+ only)
	  * @param particle Particle to update
	  * @param params System parameters
	  * @param deltaTime Time elapsed since last update
	  */
	 void UpdateParticle(Particle& particle, const ParticleSystemParams& params, float deltaTime);
 };
 
 } // namespace NiXX32