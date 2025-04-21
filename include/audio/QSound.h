/**
 * QSound.h
 * Q-Sound spatial audio processing for NiXX-32 arcade board emulation
 * 
 * This file defines the emulation of the Q-Sound DSP that provides spatial
 * audio processing for the NiXX-32 arcade system. Q-Sound creates 3D-like 
 * sound placement from stereo output, enhancing the audio experience with
 * positional audio effects.
 */

 #pragma once

 #include <cstdint>
 #include <vector>
 #include <array>
 #include <string>
 #include <functional>
 #include <memory>
 #include <unordered_map>
 
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class AudioSystem;
 class MemoryManager;
 
 /**
  * Q-Sound register addresses
  */
 enum QSoundRegisters {
	 QS_CONTROL        = 0x00,  // Control register
	 QS_STATUS         = 0x01,  // Status register
	 QS_CHANNEL_SELECT = 0x02,  // Channel select register
	 QS_POS_X          = 0x03,  // X position register
	 QS_POS_Y          = 0x04,  // Y position register
	 QS_POS_Z          = 0x05,  // Z position register
	 QS_VOLUME         = 0x06,  // Volume register
	 QS_REVERB         = 0x07,  // Reverb level register
	 QS_DELAY          = 0x08,  // Delay effect register
	 QS_PAN_MODE       = 0x09,  // Panning mode register
	 QS_FILTER         = 0x0A   // Filter control register
 };
 
 /**
  * Q-Sound panning modes
  */
 enum QSoundPanMode {
	 PAN_NORMAL        = 0,     // Standard stereo panning
	 PAN_SPATIAL       = 1,     // Full 3D spatial processing
	 PAN_SURROUND      = 2,     // Surround effect
	 PAN_WIDE          = 3      // Wide stereo effect
 };
 
 /**
  * Q-Sound filter types
  */
 enum QSoundFilterType {
	 FILTER_NONE       = 0,     // No filtering
	 FILTER_LOWPASS    = 1,     // Low-pass filter
	 FILTER_HIGHPASS   = 2,     // High-pass filter
	 FILTER_BANDPASS   = 3      // Band-pass filter
 };
 
 /**
  * Q-Sound channel positional information
  */
 struct QSoundPosition {
	 int8_t x;                  // X position (-128 to 127)
	 int8_t y;                  // Y position (-128 to 127)
	 int8_t z;                  // Z position (-128 to 127)
	 uint8_t volume;            // Volume level (0-255)
	 uint8_t reverb;            // Reverb level (0-255)
	 uint8_t delay;             // Delay effect (0-255)
	 QSoundPanMode panMode;     // Panning mode
	 QSoundFilterType filter;   // Filter type
	 uint8_t filterCutoff;      // Filter cutoff frequency
	 uint8_t filterResonance;   // Filter resonance
 };
 
 /**
  * Q-Sound channel state
  */
 struct QSoundChannel {
	 QSoundPosition position;   // Positional information
	 int16_t lastSampleL;       // Last processed sample (left)
	 int16_t lastSampleR;       // Last processed sample (right)
	 bool active;               // Channel is active
	 float delayBuffer[512];    // Delay buffer for effects
	 int delayPos;              // Current position in delay buffer
	 float filterHistory[4];    // Filter history values
 };
 
 /**
  * Q-Sound configuration
  */
 struct QSoundConfig {
	 uint8_t channels;          // Number of supported channels
	 bool reverbSupported;      // Whether reverb is supported
	 bool delaySupported;       // Whether delay is supported
	 bool filteringSupported;   // Whether filtering is supported
	 bool spatialModeSupported; // Whether spatial mode is supported
 };
 
 /**
  * Class for emulating the Q-Sound spatial audio DSP
  */
 class QSound {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param audioSystem Reference to the audio system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 QSound(System& system, AudioSystem& audioSystem, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~QSound();
	 
	 /**
	  * Initialize the Q-Sound emulation
	  * @param config Q-Sound configuration
	  * @param variant Hardware variant to configure for
	  * @return True if initialization was successful
	  */
	 bool Initialize(const QSoundConfig& config);
	 
	 /**
	  * Reset the Q-Sound to initial state
	  */
	 void Reset();
	 
	 /**
	  * Write to a Q-Sound register
	  * @param reg Register address
	  * @param value Value to write
	  */
	 void WriteReg(uint8_t reg, uint8_t value);
	 
	 /**
	  * Read from a Q-Sound register
	  * @param reg Register address
	  * @return Register value
	  */
	 uint8_t ReadReg(uint8_t reg);
	 
	 /**
	  * Process audio samples with spatial effects
	  * @param inputL Input samples (left channel)
	  * @param inputR Input samples (right channel)
	  * @param outputL Output buffer for processed samples (left channel)
	  * @param outputR Output buffer for processed samples (right channel)
	  * @param samples Number of samples to process
	  */
	 void Process(const int16_t* inputL, const int16_t* inputR, 
				 int16_t* outputL, int16_t* outputR, int samples);
	 
	 /**
	  * Update Q-Sound state
	  * @param deltaTime Time elapsed since last update in seconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Set channel positional information
	  * @param channel Channel number
	  * @param position Positional information
	  * @return True if successful
	  */
	 bool SetChannelPosition(uint8_t channel, const QSoundPosition& position);
	 
	 /**
	  * Get channel positional information
	  * @param channel Channel number
	  * @return Positional information
	  */
	 QSoundPosition GetChannelPosition(uint8_t channel) const;
	 
	 /**
	  * Set channel active state
	  * @param channel Channel number
	  * @param active True to activate, false to deactivate
	  * @return True if successful
	  */
	 bool SetChannelActive(uint8_t channel, bool active);
	 
	 /**
	  * Check if a channel is active
	  * @param channel Channel number
	  * @return True if channel is active
	  */
	 bool IsChannelActive(uint8_t channel) const;
	 
	 /**
	  * Set channel spatial position
	  * @param channel Channel number
	  * @param x X position (-128 to 127)
	  * @param y Y position (-128 to 127)
	  * @param z Z position (-128 to 127)
	  * @return True if successful
	  */
	 bool SetChannelSpatialPosition(uint8_t channel, int8_t x, int8_t y, int8_t z);
	 
	 /**
	  * Get channel spatial position
	  * @param channel Channel number
	  * @param x Output parameter for X position
	  * @param y Output parameter for Y position
	  * @param z Output parameter for Z position
	  * @return True if successful
	  */
	 bool GetChannelSpatialPosition(uint8_t channel, int8_t& x, int8_t& y, int8_t& z) const;
	 
	 /**
	  * Set channel volume
	  * @param channel Channel number
	  * @param volume Volume level (0-255)
	  * @return True if successful
	  */
	 bool SetChannelVolume(uint8_t channel, uint8_t volume);
	 
	 /**
	  * Get channel volume
	  * @param channel Channel number
	  * @return Volume level
	  */
	 uint8_t GetChannelVolume(uint8_t channel) const;
	 
	 /**
	  * Set channel reverb level
	  * @param channel Channel number
	  * @param reverb Reverb level (0-255)
	  * @return True if successful
	  */
	 bool SetChannelReverb(uint8_t channel, uint8_t reverb);
	 
	 /**
	  * Get channel reverb level
	  * @param channel Channel number
	  * @return Reverb level
	  */
	 uint8_t GetChannelReverb(uint8_t channel) const;
	 
	 /**
	  * Set channel delay effect
	  * @param channel Channel number
	  * @param delay Delay effect level (0-255)
	  * @return True if successful
	  */
	 bool SetChannelDelay(uint8_t channel, uint8_t delay);
	 
	 /**
	  * Get channel delay effect
	  * @param channel Channel number
	  * @return Delay effect level
	  */
	 uint8_t GetChannelDelay(uint8_t channel) const;
	 
	 /**
	  * Set channel panning mode
	  * @param channel Channel number
	  * @param mode Panning mode
	  * @return True if successful
	  */
	 bool SetChannelPanMode(uint8_t channel, QSoundPanMode mode);
	 
	 /**
	  * Get channel panning mode
	  * @param channel Channel number
	  * @return Panning mode
	  */
	 QSoundPanMode GetChannelPanMode(uint8_t channel) const;
	 
	 /**
	  * Set channel filter
	  * @param channel Channel number
	  * @param type Filter type
	  * @param cutoff Filter cutoff frequency
	  * @param resonance Filter resonance
	  * @return True if successful
	  */
	 bool SetChannelFilter(uint8_t channel, QSoundFilterType type, 
						   uint8_t cutoff, uint8_t resonance);
	 
	 /**
	  * Get channel filter parameters
	  * @param channel Channel number
	  * @param type Output parameter for filter type
	  * @param cutoff Output parameter for cutoff frequency
	  * @param resonance Output parameter for resonance
	  * @return True if successful
	  */
	 bool GetChannelFilter(uint8_t channel, QSoundFilterType& type,
						  uint8_t& cutoff, uint8_t& resonance) const;
	 
	 /**
	  * Get the Q-Sound configuration
	  * @return Q-Sound configuration
	  */
	 const QSoundConfig& GetConfig() const;
	 
	 /**
	  * Save the current state to a buffer
	  * @param buffer Output buffer for state data
	  * @return Size of saved state in bytes
	  */
	 size_t SaveState(uint8_t* buffer);
	 
	 /**
	  * Load state from a buffer
	  * @param buffer Buffer containing state data
	  * @param size Size of the buffer
	  * @return True if state was loaded successfully
	  */
	 bool LoadState(const uint8_t* buffer, size_t size);
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to audio system
	 AudioSystem& m_audioSystem;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Q-Sound configuration
	 QSoundConfig m_config;
	 
	 // Internal register values
	 std::array<uint8_t, 16> m_registers;
	 
	 // Channel data
	 std::vector<QSoundChannel> m_channels;
	 
	 // Currently selected channel
	 uint8_t m_currentChannel;
	 
	 // Master control flags
	 bool m_enabled;            // Q-Sound processing is enabled
	 bool m_bypassMode;         // Bypass mode (no processing)
	 uint8_t m_masterVolume;    // Master volume level
	 
	 // Reverb parameters
	 struct {
		 uint8_t roomSize;      // Room size for reverb
		 uint8_t damping;       // Reverb damping
		 uint8_t wetLevel;      // Wet level
		 uint8_t dryLevel;      // Dry level
		 float buffer[1024];    // Reverb buffer
		 int bufferPos;         // Current position in buffer
	 } m_reverb;
	 
	 // Tables for spatial calculations
	 std::array<float, 256> m_panTable;       // Panning lookup table
	 std::array<float, 256> m_distanceTable;  // Distance attenuation table
	 
	 /**
	  * Initialize tables
	  */
	 void InitTables();
	 
	 /**
	  * Calculate panning values based on position
	  * @param position Positional information
	  * @param leftGain Output parameter for left gain
	  * @param rightGain Output parameter for right gain
	  */
	 void CalculatePanning(const QSoundPosition& position, float& leftGain, float& rightGain);
	 
	 /**
	  * Apply reverb effect to a sample
	  * @param input Input sample
	  * @param channel Channel number
	  * @return Processed sample
	  */
	 float ApplyReverb(float input, uint8_t channel);
	 
	 /**
	  * Apply delay effect to a sample
	  * @param input Input sample
	  * @param channel Channel number
	  * @return Processed sample
	  */
	 float ApplyDelay(float input, uint8_t channel);
	 
	 /**
	  * Apply filter to a sample
	  * @param input Input sample
	  * @param channel Channel number
	  * @return Processed sample
	  */
	 float ApplyFilter(float input, uint8_t channel);
	 
	 /**
	  * Process a single channel
	  * @param channel Channel number
	  * @param inputL Input sample (left)
	  * @param inputR Input sample (right)
	  * @param outputL Output parameter for processed sample (left)
	  * @param outputR Output parameter for processed sample (right)
	  */
	 void ProcessChannel(uint8_t channel, int16_t inputL, int16_t inputR, 
						float& outputL, float& outputR);
	 
	 /**
	  * Calculate distance attenuation based on Z position
	  * @param z Z position (-128 to 127)
	  * @return Attenuation factor (0.0 to 1.0)
	  */
	 float CalculateDistanceAttenuation(int8_t z) const;
	 
	 /**
	  * Process register write operations
	  * @param reg Register address
	  * @param value Value to write
	  */
	 void ProcessRegWrite(uint8_t reg, uint8_t value);
 };
 
 } // namespace NiXX32