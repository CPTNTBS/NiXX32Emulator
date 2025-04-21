/**
 * PCMPlayer.h
 * PCM sample playback for NiXX-32 arcade board emulation
 * 
 * This file defines the PCM (Pulse Code Modulation) player that handles digital
 * sample playback for sound effects and music in the NiXX-32 arcade hardware.
 * It supports both 8-bit and 16-bit samples at various sample rates, with
 * looping capabilities for both the original and enhanced hardware variants.
 */

 #pragma once

 #include <cstdint>
 #include <vector>
 #include <array>
 #include <string>
 #include <memory>
 #include <unordered_map>
 
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class AudioSystem;
 class MemoryManager;
 
 /**
  * PCM channel states
  */
 enum class PCMChannelState {
	 IDLE,       // Channel is not playing
	 PLAYING,    // Channel is playing a sample
	 PAUSED,     // Channel is paused
	 STOPPING    // Channel is fading out
 };
 
 /**
  * PCM sample playback information
  */
 struct PCMPlaybackInfo {
	 uint16_t sampleIndex;    // Index of the sample being played
	 uint32_t position;       // Current playback position in samples
	 uint32_t length;         // Total length in samples
	 uint16_t frequency;      // Playback frequency
	 uint8_t volume;          // Volume (0-255)
	 uint8_t pan;             // Panning (0-255, 128 = center)
	 bool loop;               // Loop flag
	 uint32_t loopStart;      // Loop start point in samples
	 uint32_t loopEnd;        // Loop end point in samples
	 uint8_t priority;        // Priority level
	 PCMChannelState state;   // Current channel state
	 float fadeSpeed;         // Fade speed for stopping
	 int16_t lastSampleL;     // Last sample output (left)
	 int16_t lastSampleR;     // Last sample output (right)
 };
 
 /**
  * PCM player configuration
  */
 struct PCMPlayerConfig {
	 uint8_t channels;        // Number of playback channels
	 uint32_t sampleRate;     // Output sample rate in Hz
	 uint8_t maxSampleBits;   // Maximum sample bit depth (8 or 16)
	 bool stereoSupported;    // Whether stereo samples are supported
	 uint32_t sampleROMSize;  // Size of sample ROM in bytes
	 bool interpolation;      // Whether sample interpolation is enabled
 };
 
 /**
  * Class for PCM sample playback
  */
 class PCMPlayer {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param audioSystem Reference to the audio system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 PCMPlayer(System& system, AudioSystem& audioSystem, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~PCMPlayer();
	 
	 /**
	  * Initialize the PCM player
	  * @param config Player configuration
	  * @param sampleROMBaseAddress Base address of sample ROM in memory
	  * @return True if initialization was successful
	  */
	 bool Initialize(const PCMPlayerConfig& config, uint32_t sampleROMBaseAddress);
	 
	 /**
	  * Reset the PCM player to initial state
	  */
	 void Reset();
	 
	 /**
	  * Update PCM player state
	  * @param deltaTime Time elapsed since last update in seconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Generate audio samples
	  * @param buffer Output buffer (interleaved stereo)
	  * @param samples Number of sample frames to generate
	  */
	 void Generate(int16_t* buffer, int samples);
	 
	 /**
	  * Start playback of a sample
	  * @param channel Channel number to use
	  * @param sampleIndex Index of the sample to play
	  * @param frequency Playback frequency
	  * @param volume Volume level (0-255)
	  * @param pan Panning value (0-255, 128 = center)
	  * @param loop Whether to loop the sample
	  * @param priority Priority level (higher = more important)
	  * @return True if playback started successfully
	  */
	 bool PlaySample(uint8_t channel, uint16_t sampleIndex, uint16_t frequency,
					 uint8_t volume, uint8_t pan, bool loop, uint8_t priority);
	 
	 /**
	  * Stop playback on a channel
	  * @param channel Channel number
	  * @param immediate True for immediate stop, false for fade out
	  * @return True if successful
	  */
	 bool StopChannel(uint8_t channel, bool immediate = false);
	 
	 /**
	  * Pause/resume playback on a channel
	  * @param channel Channel number
	  * @param paused True to pause, false to resume
	  * @return True if successful
	  */
	 bool SetChannelPause(uint8_t channel, bool paused);
	 
	 /**
	  * Set channel volume
	  * @param channel Channel number
	  * @param volume Volume level (0-255)
	  * @return True if successful
	  */
	 bool SetChannelVolume(uint8_t channel, uint8_t volume);
	 
	 /**
	  * Set channel panning
	  * @param channel Channel number
	  * @param pan Panning value (0-255, 128 = center)
	  * @return True if successful
	  */
	 bool SetChannelPan(uint8_t channel, uint8_t pan);
	 
	 /**
	  * Set channel frequency
	  * @param channel Channel number
	  * @param frequency Playback frequency
	  * @return True if successful
	  */
	 bool SetChannelFrequency(uint8_t channel, uint16_t frequency);
	 
	 /**
	  * Set channel loop points
	  * @param channel Channel number
	  * @param loopStart Loop start point in samples
	  * @param loopEnd Loop end point in samples
	  * @param loop Whether to enable looping
	  * @return True if successful
	  */
	 bool SetChannelLoop(uint8_t channel, uint32_t loopStart, uint32_t loopEnd, bool loop);
	 
	 /**
	  * Get channel playback information
	  * @param channel Channel number
	  * @return Playback information
	  */
	 PCMPlaybackInfo GetChannelInfo(uint8_t channel) const;
	 
	 /**
	  * Check if a channel is active
	  * @param channel Channel number
	  * @return True if channel is active
	  */
	 bool IsChannelActive(uint8_t channel) const;
	 
	 /**
	  * Get the number of active channels
	  * @return Number of active channels
	  */
	 uint8_t GetActiveChannelCount() const;
	 
	 /**
	  * Register a sample in the system
	  * @param index Sample index
	  * @param address Address in sample ROM
	  * @param length Length in bytes
	  * @param bits Sample bit depth (8 or 16)
	  * @param stereo Whether sample is stereo
	  * @param frequency Default sample frequency
	  * @return True if successful
	  */
	 bool RegisterSample(uint16_t index, uint32_t address, uint32_t length,
						uint8_t bits, bool stereo, uint16_t frequency);
	 
	 /**
	  * Set sample loop points
	  * @param index Sample index
	  * @param loopStart Loop start point in samples
	  * @param loopEnd Loop end point in samples
	  * @param loop Whether to enable looping by default
	  * @return True if successful
	  */
	 bool SetSampleLoop(uint16_t index, uint32_t loopStart, uint32_t loopEnd, bool loop);
	 
	 /**
	  * Get the highest priority channel playing
	  * @return Channel number, or -1 if no channels are active
	  */
	 int GetHighestPriorityChannel() const;
	 
	 /**
	  * Find a channel to steal based on priority
	  * @param minPriority Minimum priority to consider for stealing
	  * @return Channel number, or -1 if no suitable channel found
	  */
	 int FindChannelToSteal(uint8_t minPriority) const;
	 
	 /**
	  * Get the PCM player configuration
	  * @return Player configuration
	  */
	 const PCMPlayerConfig& GetConfig() const;
	 
	 /**
	  * Handle register write
	  * @param address Register address
	  * @param value Value to write
	  */
	 void HandleRegisterWrite(uint8_t address, uint8_t value);
	 
	 /**
	  * Read from register
	  * @param address Register address
	  * @return Register value
	  */
	 uint8_t HandleRegisterRead(uint8_t address);
	 
	 /**
	  * Get a sample from ROM
	  * @param sampleIndex Sample index
	  * @param position Position in samples
	  * @param channel Channel (0=left, 1=right)
	  * @return Sample value (-32768 to 32767)
	  */
	 int16_t GetSample(uint16_t sampleIndex, uint32_t position, uint8_t channel) const;
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to audio system
	 AudioSystem& m_audioSystem;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Player configuration
	 PCMPlayerConfig m_config;
	 
	 // Base address of sample ROM
	 uint32_t m_sampleROMBaseAddress;
	 
	 // Channel playback information
	 std::vector<PCMPlaybackInfo> m_channels;
	 
	 // Sample information
	 struct SampleInfo {
		 uint32_t address;        // Address in sample ROM
		 uint32_t length;         // Length in bytes
		 uint8_t bits;            // Sample bit depth (8 or 16)
		 bool stereo;             // Whether sample is stereo
		 uint16_t frequency;      // Default sample frequency
		 uint32_t loopStart;      // Loop start point in samples
		 uint32_t loopEnd;        // Loop end point in samples
		 bool loop;               // Whether sample loops by default
	 };
	 std::unordered_map<uint16_t, SampleInfo> m_samples;
	 
	 // Hardware registers
	 struct {
		 uint8_t control;         // Control register
		 uint8_t status;          // Status register
		 uint8_t channelSelect;   // Channel select register
		 uint8_t sampleSelect;    // Sample select register
		 uint8_t volume;          // Volume register
		 uint8_t pan;             // Pan register
		 uint8_t frequency[2];    // Frequency registers (low/high)
		 uint8_t loopStart[3];    // Loop start registers
		 uint8_t loopEnd[3];      // Loop end registers
		 uint8_t loopEnable;      // Loop enable register
	 } m_registers;
	 
	 /**
	  * Get sample count for a sample
	  * @param sampleIndex Sample index
	  * @return Number of samples, or 0 if invalid
	  */
	 uint32_t GetSampleCount(uint16_t sampleIndex) const;
	 
	 /**
	  * Convert sample address to sample count
	  * @param address Sample address
	  * @param length Length in bytes
	  * @param bits Sample bit depth
	  * @param stereo Whether sample is stereo
	  * @return Number of samples
	  */
	 uint32_t AddressToSampleCount(uint32_t address, uint32_t length, uint8_t bits, bool stereo) const;
	 
	 /**
	  * Mix a channel into the output buffer
	  * @param channel Channel number
	  * @param buffer Output buffer
	  * @param samples Number of sample frames
	  */
	 void MixChannel(uint8_t channel, int16_t* buffer, int samples);
	 
	 /**
	  * Read an 8-bit sample from ROM
	  * @param address Address in sample ROM
	  * @return Sample value (-32768 to 32767)
	  */
	 int16_t Read8BitSample(uint32_t address) const;
	 
	 /**
	  * Read a 16-bit sample from ROM
	  * @param address Address in sample ROM
	  * @return Sample value (-32768 to 32767)
	  */
	 int16_t Read16BitSample(uint32_t address) const;
	 
	 /**
	  * Apply channel volume and panning
	  * @param sample Input sample
	  * @param volume Volume (0-255)
	  * @param pan Pan position (0-255, 128=center)
	  * @param left Output parameter for left channel
	  * @param right Output parameter for right channel
	  */
	 void ApplyVolumeAndPan(int16_t sample, uint8_t volume, uint8_t pan, int16_t& left, int16_t& right) const;
	 
	 /**
	  * Apply interpolation between samples
	  * @param prev Previous sample
	  * @param next Next sample
	  * @param fraction Fractional position (0.0 to 0.999...)
	  * @return Interpolated sample
	  */
	 int16_t Interpolate(int16_t prev, int16_t next, float fraction) const;
	 
	 /**
	  * Update channel position based on frequency
	  * @param channel Channel number
	  * @param samples Number of samples to advance
	  */
	 void UpdateChannelPosition(uint8_t channel, int samples);
 };
 
 } // namespace NiXX32