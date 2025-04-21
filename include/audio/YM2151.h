/**
 * YM2151.h
 * Yamaha YM2151 FM synthesis chip emulation for NiXX-32 arcade board
 * 
 * This file defines the emulation of the YM2151 FM synthesis chip that handles
 * the music and sound effect generation in the NiXX-32 arcade system. The YM2151
 * (OPM) is an 8-channel FM synthesis chip used in many arcade systems of the era.
 */

 #pragma once

 #include <cstdint>
 #include <vector>
 #include <array>
 #include <string>
 #include <functional>
 #include <memory>
 
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class AudioSystem;
 
 /**
  * YM2151 register addresses
  */
 enum YM2151Registers {
	 YM_TEST            = 0x01,  // Test register
	 YM_LFO_FREQ        = 0x18,  // LFO frequency
	 YM_LFO_PMD         = 0x19,  // LFO phase modulation depth
	 YM_LFO_AMD         = 0x19,  // LFO amplitude modulation depth
	 YM_LFO_WAVEFORM    = 0x1B,  // LFO waveform
	 YM_TIMER_A_MSB     = 0x10,  // Timer A MSB
	 YM_TIMER_A_LSB     = 0x11,  // Timer A LSB
	 YM_TIMER_B         = 0x12,  // Timer B
	 YM_TIMER_CONTROL   = 0x14,  // Timer control
	 YM_NOISE           = 0x0F,  // Noise control
	 YM_CSM_MODE        = 0x15,  // CSM mode
	 YM_CHANNEL_MASK    = 0x20,  // Channel mask
	 // Operator registers (base addresses, add channel/operator offset)
	 YM_DT_MULT         = 0x40,  // Detune/Multiple
	 YM_TL              = 0x60,  // Total Level
	 YM_KS_AR           = 0x80,  // Key Scale/Attack Rate
	 YM_A_D1R           = 0xA0,  // AM On/Decay Rate 1
	 YM_DT2_D2R         = 0xC0,  // Detune 2/Decay Rate 2
	 YM_D1L_RR          = 0xE0,  // Decay Level 1/Release Rate
	 // Channel registers (base addresses, add channel offset)
	 YM_FB_CONN         = 0x20,  // Feedback/Connection
	 YM_KC              = 0x28,  // Key Code
	 YM_KF              = 0x30,  // Key Fraction
	 YM_PMS_AMS         = 0x38   // Phase/Amplitude Modulation Sensitivity
 };
 
 /**
  * YM2151 LFO (Low Frequency Oscillator) waveforms
  */
 enum YM2151LFOWaveform {
	 LFO_SAWTOOTH   = 0,
	 LFO_SQUARE     = 1,
	 LFO_TRIANGLE   = 2,
	 LFO_NOISE      = 3
 };
 
 /**
  * YM2151 operator state
  */
 struct YM2151Operator {
	 uint8_t detune;         // Detune value
	 uint8_t multiple;       // Frequency multiplier
	 uint8_t totalLevel;     // Total level (volume)
	 uint8_t keyScale;       // Key scaling
	 uint8_t attackRate;     // Attack rate
	 uint8_t amOn;           // Amplitude modulation enabled
	 uint8_t decayRate1;     // First decay rate
	 uint8_t detune2;        // Second detune value
	 uint8_t decayRate2;     // Second decay rate
	 uint8_t decayLevel1;    // First decay level
	 uint8_t releaseRate;    // Release rate
	 uint8_t keyOn;          // Key is on
	 
	 // Runtime state
	 double phase;           // Current phase
	 double output;          // Current output level
	 int envelope;           // Current envelope level
	 int envelopeState;      // Current envelope state (attack/decay/sustain/release)
 };
 
 /**
  * YM2151 channel state
  */
 struct YM2151Channel {
	 uint8_t feedback;       // Feedback amount
	 uint8_t algorithm;      // Connection algorithm (0-7)
	 uint8_t keyCode;        // Key code (note)
	 uint8_t keyFraction;    // Key fraction (fine-tuning)
	 uint8_t pms;            // Phase modulation sensitivity
	 uint8_t ams;            // Amplitude modulation sensitivity
	 bool keyOn;             // Channel key is on
	 
	 // Operators for this channel
	 std::array<YM2151Operator, 4> operators;
	 
	 // Runtime state
	 int16_t output;         // Current channel output
 };
 
 /**
  * Class for emulating the Yamaha YM2151 FM synthesis chip
  */
 class YM2151 {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param audioSystem Reference to the audio system
	  * @param logger Reference to the system logger
	  */
	 YM2151(System& system, AudioSystem& audioSystem, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~YM2151();
	 
	 /**
	  * Initialize the YM2151 emulation
	  * @param clockRate Chip clock rate in Hz
	  * @param sampleRate Output sample rate in Hz
	  * @return True if initialization was successful
	  */
	 bool Initialize(uint32_t clockRate = 3579545, uint32_t sampleRate = 44100);
	 
	 /**
	  * Reset the YM2151 to initial state
	  */
	 void Reset();
	 
	 /**
	  * Write to a YM2151 register
	  * @param reg Register address
	  * @param value Value to write
	  */
	 void WriteReg(uint8_t reg, uint8_t value);
	 
	 /**
	  * Read from a YM2151 register
	  * @param reg Register address
	  * @return Register value
	  */
	 uint8_t ReadReg(uint8_t reg);
	 
	 /**
	  * Generate audio samples
	  * @param buffer Output buffer
	  * @param samples Number of samples to generate
	  */
	 void Generate(int16_t* buffer, int samples);
	 
	 /**
	  * Update YM2151 state
	  * @param deltaTime Time elapsed since last update in seconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Set key on/off for a channel
	  * @param channel Channel number (0-7)
	  * @param keyOn True for key on, false for key off
	  * @param operatorMask Mask of operators to affect (bit 0-3)
	  */
	 void SetKeyOnOff(uint8_t channel, bool keyOn, uint8_t operatorMask = 0x0F);
	 
	 /**
	  * Set the note for a channel
	  * @param channel Channel number (0-7)
	  * @param note MIDI note number
	  * @param detune Detune value (-128 to 127)
	  */
	 void SetNote(uint8_t channel, uint8_t note, int8_t detune = 0);
	 
	 /**
	  * Set the volume for a channel
	  * @param channel Channel number (0-7)
	  * @param volume Volume level (0-127)
	  */
	 void SetVolume(uint8_t channel, uint8_t volume);
	 
	 /**
	  * Set an instrument for a channel
	  * @param channel Channel number (0-7)
	  * @param algorithm Algorithm (0-7)
	  * @param feedback Feedback amount (0-7)
	  * @param operators Array of operator settings
	  */
	 void SetInstrument(uint8_t channel, uint8_t algorithm, uint8_t feedback, 
					   const std::array<YM2151Operator, 4>& operators);
	 
	 /**
	  * Set LFO parameters
	  * @param frequency LFO frequency (0-255)
	  * @param pmd Phase modulation depth (0-127)
	  * @param amd Amplitude modulation depth (0-127)
	  * @param waveform LFO waveform type
	  */
	 void SetLFO(uint8_t frequency, uint8_t pmd, uint8_t amd, YM2151LFOWaveform waveform);
	 
	 /**
	  * Get the current status register
	  * @return Status register value
	  */
	 uint8_t GetStatus() const;
	 
	 /**
	  * Check if a timer has expired
	  * @param timerA True for timer A, false for timer B
	  * @return True if timer has expired
	  */
	 bool IsTimerExpired(bool timerA) const;
	 
	 /**
	  * Start/stop a timer
	  * @param timerA True for timer A, false for timer B
	  * @param start True to start, false to stop
	  * @param irqEnable True to enable IRQ on timer expiration
	  */
	 void SetTimer(bool timerA, bool start, bool irqEnable = false);
	 
	 /**
	  * Set a timer period
	  * @param timerA True for timer A, false for timer B
	  * @param period Timer period value
	  */
	 void SetTimerPeriod(bool timerA, uint16_t period);
	 
	 /**
	  * Register an IRQ callback
	  * @param callback Function to call when IRQ is triggered
	  */
	 void RegisterIRQCallback(std::function<void()> callback);
	 
	 /**
	  * Get the channel output level
	  * @param channel Channel number (0-7)
	  * @return Output level (-32768 to 32767)
	  */
	 int16_t GetChannelOutput(uint8_t channel) const;
	 
	 /**
	  * Get all channel outputs
	  * @return Array of channel output levels
	  */
	 std::array<int16_t, 8> GetAllChannelOutputs() const;
	 
	 /**
	  * Get the current frequency for a channel
	  * @param channel Channel number (0-7)
	  * @return Frequency in Hz
	  */
	 float GetChannelFrequency(uint8_t channel) const;
	 
	 /**
	  * Get the envelope state for an operator
	  * @param channel Channel number (0-7)
	  * @param op Operator number (0-3)
	  * @return Envelope level (0-1023)
	  */
	 int GetOperatorEnvelope(uint8_t channel, uint8_t op) const;
	 
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
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // YM2151 clock rate
	 uint32_t m_clockRate;
	 
	 // Output sample rate
	 uint32_t m_sampleRate;
	 
	 // Sample rate ratio for interpolation
	 double m_sampleRateRatio;
	 
	 // Internal register values
	 std::array<uint8_t, 0x100> m_registers;
	 
	 // Status register
	 uint8_t m_status;
	 
	 // IRQ callback
	 std::function<void()> m_irqCallback;
	 
	 // LFO state
	 struct {
		 uint8_t frequency;
		 uint8_t pmd;
		 uint8_t amd;
		 YM2151LFOWaveform waveform;
		 double phase;
		 double output;
	 } m_lfo;
	 
	 // Timer state
	 struct {
		 uint16_t periodA;
		 uint16_t periodB;
		 uint16_t counterA;
		 uint16_t counterB;
		 bool runningA;
		 bool runningB;
		 bool irqEnabledA;
		 bool irqEnabledB;
		 bool expiredA;
		 bool expiredB;
	 } m_timer;
	 
	 // Channel state
	 std::array<YM2151Channel, 8> m_channels;
	 
	 // Current output buffer position
	 int m_bufferPos;
	 
	 // Sin table for fast lookups
	 std::array<int16_t, 1024> m_sinTable;
	 
	 // Exp table for envelope calculations
	 std::array<uint16_t, 256> m_expTable;
	 
	 // Frequency lookup tables
	 std::array<uint32_t, 128> m_fnumTable;
	 
	 /**
	  * Initialize tables
	  */
	 void InitTables();
	 
	 /**
	  * Update timers
	  * @param deltaTime Time elapsed since last update in seconds
	  */
	 void UpdateTimers(float deltaTime);
	 
	 /**
	  * Update LFO
	  * @param deltaTime Time elapsed since last update in seconds
	  */
	 void UpdateLFO(float deltaTime);
	 
	 /**
	  * Update channel frequencies
	  */
	 void UpdateFrequencies();
	 
	 /**
	  * Calculate channel output
	  * @param channel Channel to calculate
	  * @return Sample value (-32768 to 32767)
	  */
	 int16_t CalculateChannelOutput(uint8_t channel);
	 
	 /**
	  * Calculate operator output
	  * @param channel Channel number
	  * @param op Operator number
	  * @param modulation Modulation input
	  * @return Operator output
	  */
	 double CalculateOperatorOutput(uint8_t channel, uint8_t op, double modulation);
	 
	 /**
	  * Update operator envelope
	  * @param channel Channel number
	  * @param op Operator number
	  * @param deltaTime Time elapsed since last update in seconds
	  */
	 void UpdateOperatorEnvelope(uint8_t channel, uint8_t op, float deltaTime);
	 
	 /**
	  * Get MIDI note from key code
	  * @param kc Key code
	  * @param kf Key fraction
	  * @return MIDI note number
	  */
	 uint8_t KeyCodeToMIDINote(uint8_t kc, uint8_t kf) const;
	 
	 /**
	  * Get key code from MIDI note
	  * @param note MIDI note number
	  * @param kf Output parameter for key fraction
	  * @return Key code
	  */
	 uint8_t MIDINoteToKeyCode(uint8_t note, uint8_t& kf) const;
	 
	 /**
	  * Get frequency for a key code and fraction
	  * @param kc Key code
	  * @param kf Key fraction
	  * @return Frequency in Hz
	  */
	 float KeyToFrequency(uint8_t kc, uint8_t kf) const;
	 
	 /**
	  * Get LFO output value
	  * @return LFO output (-1.0 to 1.0)
	  */
	 double GetLFOOutput() const;
	 
	 /**
	  * Process register write operations
	  * @param reg Register address
	  * @param value Value to write
	  */
	 void ProcessRegWrite(uint8_t reg, uint8_t value);
	 
	 /**
	  * Get the algorithm connection pattern
	  * @param algorithm Algorithm number (0-7)
	  * @param opOutputs Array to fill with operator connections
	  */
	 void GetAlgorithmConnections(uint8_t algorithm, int connections[4][4]) const;
 };
 
 } // namespace NiXX32