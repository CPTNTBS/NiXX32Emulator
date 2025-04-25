/**
 * AudioSystem.h
 * Audio system for NiXX-32 arcade board emulation
 * 
 * This file defines the audio system that handles sound generation,
 * FM synthesis, PCM playback, and sound effects for the NiXX-32
 * arcade hardware.
 */

 #pragma once

 #include <cstdint>
 #include <memory>
 #include <vector>
 #include <string>
 #include <functional>
 #include <unordered_map>
 
 #include "MemoryManager.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class Z80CPU;
 class YM2151;
 class PCMPlayer;
 class QSound;
 
 /**
  * Defines the hardware variant supported by the audio system
  */
 enum class AudioHardwareVariant {
	 NIXX32_ORIGINAL,  // Original 1989 model (YM2151 + PCM @ 11kHz)
	 NIXX32_PLUS       // Enhanced 1992 model (YM2151 + PCM @ 22kHz + enhanced effects)
 };
 
 /**
  * Sound channel types
  */
 enum class SoundChannelType {
	 FM,             // FM synthesis channel
	 PCM,            // PCM sample playback channel
	 QSOUND,         // Q-Sound spatial audio channel
	 NOISE           // Noise generator channel
 };
 
 /**
  * Sound effect parameters
  */
 struct SoundEffectParams {
	 uint16_t sampleIndex;    // Sample index in the ROM
	 uint16_t frequency;      // Playback frequency
	 uint8_t volume;          // Volume (0-255)
	 uint8_t pan;             // Panning (0-255, 128 = center)
	 bool loop;               // Loop flag
	 uint16_t loopStart;      // Loop start point
	 uint16_t loopEnd;        // Loop end point
	 uint8_t priority;        // Priority level (higher = more important)
 };
 
 /**
  * FM instrument definition
  */
 struct FMInstrument {
	 uint8_t algorithm;       // FM algorithm (0-7)
	 uint8_t feedback;        // Feedback value
	 
	 // Operator parameters (4 operators)
	 struct Operator {
		 uint8_t attack;      // Attack rate
		 uint8_t decay;       // Decay rate
		 uint8_t sustain;     // Sustain level
		 uint8_t release;     // Release rate
		 uint8_t totalLevel;  // Total level (volume)
		 uint8_t multiple;    // Frequency multiplier
		 uint8_t detune;      // Detune value
		 uint8_t ksr;         // Key scale rate
		 uint8_t ksl;         // Key scale level
		 uint8_t waveform;    // Waveform selection
		 uint8_t am;          // Amplitude modulation flag
	 };
	 
	 Operator operators[4];   // Operator parameters
 };
 
 /**
  * PCM sample definition
  */
 struct PCMSample {
	 uint32_t address;        // Address in sound ROM
	 uint32_t length;         // Length in bytes
	 uint16_t frequency;      // Default playback frequency
	 uint8_t bits;            // Sample bit depth (8 or 16)
	 bool stereo;             // Stereo flag
	 uint16_t loopStart;      // Loop start point
	 uint16_t loopEnd;        // Loop end point
	 bool loop;               // Loop flag
 };
 
 /**
  * Sound channel status
  */
 struct ChannelStatus {
	 bool active;             // Channel is active
	 uint8_t volume;          // Current volume
	 uint16_t frequency;      // Current frequency
	 uint8_t pan;             // Current pan position
	 uint16_t currentPos;     // Current playback position
	 uint8_t priority;        // Channel priority
	 uint8_t note;            // MIDI note (for FM channels)
	 bool keyOn;              // Key is pressed (for FM channels)
 };
 
 /**
  * Audio system configuration
  */
 struct AudioConfig {
	 uint16_t sampleRate;     // Sample rate in Hz
	 uint8_t fmChannels;      // Number of FM synthesis channels
	 uint8_t pcmChannels;     // Number of PCM playback channels
	 uint16_t maxSamples;     // Maximum number of samples
	 uint16_t soundRamSize;   // Sound RAM size in bytes
	 bool spatialAudio;       // Whether spatial audio is supported
	 uint8_t bitDepth;        // Sample bit depth (8 or 16)
 };
 
 /**
  * Main class that manages the NiXX-32 audio system
  */
 class AudioSystem {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 AudioSystem(System& system, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~AudioSystem();
	 
	 /**
	  * Initialize the audio system
	  * @param variant Hardware variant to configure for
	  * @return True if initialization was successful
	  */
	 bool Initialize(AudioHardwareVariant variant);
	 
	 /**
	  * Reset the audio system to initial state
	  */
	 void Reset();
	 
	 /**
	  * Update audio state for the current frame
	  * @param deltaTime Time elapsed since last update in milliseconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Process audio and fill the output buffer
	  * @param buffer Pointer to output buffer
	  * @param sampleCount Number of samples to generate
	  */
	 void FillAudioBuffer(int16_t* buffer, int sampleCount);
	 
	 /**
	  * Set the master volume
	  * @param volume Volume level (0-255)
	  */
	 void SetMasterVolume(uint8_t volume);
	 
	 /**
	  * Get the master volume
	  * @return Current master volume
	  */
	 uint8_t GetMasterVolume() const;
	 
	 /**
	  * Play a sound effect
	  * @param sampleIndex Index of the sample
	  * @param params Sound effect parameters
	  * @return Channel ID if successful, -1 if failed
	  */
	 int PlaySoundEffect(uint16_t sampleIndex, const SoundEffectParams& params);
	 
	 /**
	  * Stop a playing sound effect
	  * @param channelId Channel ID to stop
	  * @param immediate True for immediate stop, false for fade out
	  * @return True if successful
	  */
	 bool StopSoundEffect(int channelId, bool immediate = false);
	 
	 /**
	  * Play an FM note
	  * @param instrumentIndex Index of the FM instrument
	  * @param note MIDI note number
	  * @param velocity Note velocity (0-127)
	  * @param duration Duration in milliseconds (0 = until stopped)
	  * @return Channel ID if successful, -1 if failed
	  */
	 int PlayFMNote(uint8_t instrumentIndex, uint8_t note, uint8_t velocity, 
					uint16_t duration = 0);
	 
	 /**
	  * Stop an FM note
	  * @param channelId Channel ID to stop
	  * @param immediate True for immediate stop, false for key release
	  * @return True if successful
	  */
	 bool StopFMNote(int channelId, bool immediate = false);
	 
	 /**
	  * Load PCM sample data
	  * @param index Sample index
	  * @param address Address in sound ROM
	  * @param length Length in bytes
	  * @param frequency Default playback frequency
	  * @param bits Sample bit depth (8 or 16)
	  * @param stereo Stereo flag
	  * @return True if successful
	  */
	 bool LoadPCMSample(uint16_t index, uint32_t address, uint32_t length, 
						uint16_t frequency, uint8_t bits, bool stereo);
	 
	 /**
	  * Set PCM sample loop points
	  * @param index Sample index
	  * @param loopStart Loop start point
	  * @param loopEnd Loop end point
	  * @param loop Loop flag
	  * @return True if successful
	  */
	 bool SetPCMSampleLoop(uint16_t index, uint16_t loopStart, uint16_t loopEnd, 
						   bool loop);
	 
	 /**
	  * Get PCM sample information
	  * @param index Sample index
	  * @return PCM sample information
	  */
	 PCMSample GetPCMSample(uint16_t index) const;
	 
	 /**
	  * Define an FM instrument
	  * @param index Instrument index
	  * @param instrument FM instrument definition
	  * @return True if successful
	  */
	 bool DefineFMInstrument(uint8_t index, const FMInstrument& instrument);
	 
	 /**
	  * Get FM instrument definition
	  * @param index Instrument index
	  * @return FM instrument definition
	  */
	 FMInstrument GetFMInstrument(uint8_t index) const;
	 
	 /**
	  * Set channel volume
	  * @param channelId Channel ID
	  * @param volume Volume level (0-255)
	  * @return True if successful
	  */
	 bool SetChannelVolume(int channelId, uint8_t volume);
	 
	 /**
	  * Set channel panning
	  * @param channelId Channel ID
	  * @param pan Panning value (0-255, 128 = center)
	  * @return True if successful
	  */
	 bool SetChannelPan(int channelId, uint8_t pan);
	 
	 /**
	  * Set channel frequency
	  * @param channelId Channel ID
	  * @param frequency Frequency value
	  * @return True if successful
	  */
	 bool SetChannelFrequency(int channelId, uint16_t frequency);
	 
	 /**
	  * Get channel status
	  * @param channelId Channel ID
	  * @return Channel status
	  */
	 ChannelStatus GetChannelStatus(int channelId) const;
	 
	 /**
	  * Set spatial position for a channel (Q-Sound)
	  * @param channelId Channel ID
	  * @param x X position (-128 to 127)
	  * @param y Y position (-128 to 127)
	  * @param z Z position (-128 to 127)
	  * @return True if successful
	  */
	 bool SetChannelPosition(int channelId, int8_t x, int8_t y, int8_t z);
	 
	 /**
	  * Pause or resume audio
	  * @param paused True to pause, false to resume
	  */
	 void SetPaused(bool paused);
	 
	 /**
	  * Check if audio is paused
	  * @return True if paused
	  */
	 bool IsPaused() const;
	 
	 /**
	  * Get YM2151 FM synthesis chip
	  * @return Reference to YM2151
	  */
	 YM2151& GetYM2151();
	 
	 /**
	  * Get PCM player
	  * @return Reference to PCM player
	  */
	 PCMPlayer& GetPCMPlayer();
	 
	 /**
	  * Get Q-Sound spatial audio
	  * @return Reference to Q-Sound
	  */
	 QSound& GetQSound();
	 
	 /**
	  * Handle sound register write
	  * @param address Register address
	  * @param value Value to write
	  */
	 void HandleRegisterWrite(uint32_t address, uint8_t value);
	 
	 /**
	  * Read from sound register
	  * @param address Register address
	  * @return Register value
	  */
	 uint8_t HandleRegisterRead(uint32_t address);
	 
	 /**
	  * Register a Z80 port handler
	  * @param port Port number
	  * @param readHandler Read handler function
	  * @param writeHandler Write handler function
	  * @return True if successful
	  */
	 bool RegisterPortHandler(uint8_t port, 
							 std::function<uint8_t()> readHandler,
							 std::function<void(uint8_t)> writeHandler);
	 
	 /**
	  * Update from sound RAM
	  */
	 void UpdateFromSoundRAM();
	 
	 /**
	  * Get the audio configuration
	  * @return Audio configuration
	  */
	 const AudioConfig& GetConfig() const;
	 
	 /**
	  * Get the hardware variant
	  * @return Audio hardware variant
	  */
	 AudioHardwareVariant GetVariant() const;
	 
	 /**
	  * Set audio CPU
	  * @param cpu Pointer to Z80 CPU
	  */
	 void SetAudioCPU(Z80CPU* cpu);

	 //Power Management Methods (TODO: Document)
	 void SetQualityReduction(bool enabled);	 
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Pointer to audio CPU (Z80)
	 Z80CPU* m_audioCPU;
	 
	 // Hardware variant
	 AudioHardwareVariant m_variant;
	 
	 // Audio configuration
	 AudioConfig m_config;
	 
	 // Master volume
	 uint8_t m_masterVolume;
	 
	 // Paused state
	 bool m_paused;
	 
	 // FM synthesis chip
	 std::unique_ptr<YM2151> m_ym2151;
	 
	 // PCM player
	 std::unique_ptr<PCMPlayer> m_pcmPlayer;
	 
	 // Q-Sound spatial audio
	 std::unique_ptr<QSound> m_qSound;
	 
	 // Next available channel ID
	 int m_nextChannelId;
	 
	 // Channel ID to channel info mapping
	 struct ChannelInfo {
		 SoundChannelType type;
		 int internalIndex;
		 ChannelStatus status;
	 };
	 std::unordered_map<int, ChannelInfo> m_channels;
	 
	 // PCM samples
	 std::vector<PCMSample> m_pcmSamples;
	 
	 // FM instruments
	 std::vector<FMInstrument> m_fmInstruments;
	 
	 // Hardware registers
	 struct {
		 uint8_t control;           // Audio control register
		 uint8_t status;            // Audio status register
		 uint8_t interruptEnable;   // Interrupt enable register
		 uint8_t interruptStatus;   // Interrupt status register
		 uint8_t masterVolume;      // Master volume register
		 uint8_t fmControl;         // FM control register
		 uint8_t pcmControl;        // PCM control register
		 uint8_t qsoundControl;     // Q-Sound control register
	 } m_registers;
	 
	 // Port handler mappings
	 std::unordered_map<uint8_t, std::function<uint8_t()>> m_portReadHandlers;
	 std::unordered_map<uint8_t, std::function<void(uint8_t)>> m_portWriteHandlers;
	 
	 /**
	  * Configure audio system for original NiXX-32 hardware
	  */
	 void ConfigureOriginalHardware();
	 
	 /**
	  * Configure audio system for enhanced NiXX-32+ hardware
	  */
	 void ConfigurePlusHardware();
	 
	 /**
	  * Find an available FM channel
	  * @param priority Channel priority
	  * @return Channel index, or -1 if not available
	  */
	 int FindAvailableFMChannel(uint8_t priority);
	 
	 /**
	  * Find an available PCM channel
	  * @param priority Channel priority
	  * @return Channel index, or -1 if not available
	  */
	 int FindAvailablePCMChannel(uint8_t priority);
	 
	 /**
	  * Find channel info by ID
	  * @param channelId Channel ID
	  * @return Pointer to channel info, or nullptr if not found
	  */
	 ChannelInfo* FindChannelInfo(int channelId);
	 
	 /**
	  * Update channel status
	  * @param channelId Channel ID
	  */
	 void UpdateChannelStatus(int channelId);
	 
	 /**
	  * Handle sound interrupts
	  */
	 void HandleInterrupts();
	 
	 /**
	  * Mix all active channels
	  * @param buffer Output buffer
	  * @param sampleCount Number of samples to generate
	  */
	 void MixChannels(int16_t* buffer, int sampleCount);
	 
	 /**
	  * Apply master volume and effects
	  * @param buffer Audio buffer
	  * @param sampleCount Number of samples
	  */
	 void ApplyMasterVolumeAndEffects(int16_t* buffer, int sampleCount);
 };
 
 } // namespace NiXX32