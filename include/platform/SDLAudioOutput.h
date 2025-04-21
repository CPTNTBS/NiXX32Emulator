/**
 * SDLAudioOutput.h
 * SDL2-based audio output for NiXX-32 arcade board emulation
 * 
 * This file defines the SDL2-based audio output implementation that handles
 * sound playback for the NiXX-32 arcade system emulation. It provides
 * low-latency audio output supporting both the original and enhanced
 * hardware variants.
 */

 #pragma once

 #include <cstdint>
 #include <string>
 #include <vector>
 #include <memory>
 #include <functional>
 #include <atomic>
 #include <mutex>
 
 #include "AudioSystem.h"
 #include "Logger.h"
 
 // Forward declarations for SDL types to avoid including SDL headers
 struct SDL_AudioDeviceID;
 struct SDL_AudioSpec;
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 
 /**
  * Audio output modes
  */
 enum class AudioOutputMode {
	 SDL,                // SDL audio output
	 FILE,               // File output (for recording)
	 CALLBACK            // Custom callback
 };
 
 /**
  * Audio resampling quality
  */
 enum class ResamplingQuality {
	 LOW,                // Fast, lower quality resampling
	 MEDIUM,             // Balanced resampling
	 HIGH                // High quality, more CPU-intensive resampling
 };
 
 /**
  * SDL audio output configuration
  */
 struct SDLAudioOutputConfig {
	 uint32_t sampleRate;           // Output sample rate in Hz
	 uint16_t bufferSize;           // Audio buffer size in samples
	 uint8_t channels;              // Number of channels (1=mono, 2=stereo)
	 bool enableResampling;         // Whether to use resampling
	 ResamplingQuality quality;     // Resampling quality
	 AudioOutputMode mode;          // Output mode
	 float masterVolume;            // Master volume (0.0-1.0)
	 std::string recordingPath;     // Path for audio recording (if enabled)
	 bool dynamicRateControl;       // Adjust rate to maintain synchronization
 };
 
 /**
  * Class that implements audio output using SDL2
  */
 class SDLAudioOutput {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param audioSystem Reference to the audio system
	  * @param logger Reference to the system logger
	  */
	 SDLAudioOutput(System& system, AudioSystem& audioSystem, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~SDLAudioOutput();
	 
	 /**
	  * Initialize the SDL audio output
	  * @param config Audio output configuration
	  * @return True if initialization was successful
	  */
	 bool Initialize(const SDLAudioOutputConfig& config);
	 
	 /**
	  * Reset the audio output state
	  */
	 void Reset();
	 
	 /**
	  * Start audio playback
	  * @return True if successful
	  */
	 bool Start();
	 
	 /**
	  * Stop audio playback
	  * @return True if successful
	  */
	 bool Stop();
	 
	 /**
	  * Pause/unpause audio playback
	  * @param paused True to pause, false to unpause
	  * @return True if successful
	  */
	 bool SetPaused(bool paused);
	 
	 /**
	  * Check if audio is paused
	  * @return True if paused
	  */
	 bool IsPaused() const;
	 
	 /**
	  * Set master volume
	  * @param volume Volume level (0.0-1.0)
	  */
	 void SetMasterVolume(float volume);
	 
	 /**
	  * Get master volume
	  * @return Current master volume
	  */
	 float GetMasterVolume() const;
	 
	 /**
	  * Set resampling quality
	  * @param quality Resampling quality
	  * @return True if successful
	  */
	 bool SetResamplingQuality(ResamplingQuality quality);
	 
	 /**
	  * Set output mode
	  * @param mode Output mode
	  * @return True if successful
	  */
	 bool SetOutputMode(AudioOutputMode mode);
	 
	 /**
	  * Start audio recording to file
	  * @param filePath Path to output file
	  * @return True if recording started successfully
	  */
	 bool StartRecording(const std::string& filePath);
	 
	 /**
	  * Stop audio recording
	  * @return True if successful
	  */
	 bool StopRecording();
	 
	 /**
	  * Check if recording is active
	  * @return True if recording
	  */
	 bool IsRecording() const;
	 
	 /**
	  * Register audio callback
	  * @param callback Function to call for audio processing
	  * @return Callback ID
	  */
	 int RegisterAudioCallback(std::function<void(int16_t*, int)> callback);
	 
	 /**
	  * Remove audio callback
	  * @param callbackId Callback ID
	  * @return True if successful
	  */
	 bool RemoveAudioCallback(int callbackId);
	 
	 /**
	  * Get audio buffer fill level
	  * @return Fill level as percentage (0.0-1.0)
	  */
	 float GetBufferFillLevel() const;
	 
	 /**
	  * Get audio latency
	  * @return Latency in milliseconds
	  */
	 float GetLatency() const;
	 
	 /**
	  * Get the current audio output configuration
	  * @return Current configuration
	  */
	 const SDLAudioOutputConfig& GetConfig() const;
	 
	 /**
	  * List available audio devices
	  * @return Vector of device names
	  */
	 std::vector<std::string> ListAudioDevices() const;
	 
	 /**
	  * Set audio device
	  * @param deviceName Device name
	  * @return True if successful
	  */
	 bool SetAudioDevice(const std::string& deviceName);
	 
	 /**
	  * Get current audio device name
	  * @return Current device name
	  */
	 std::string GetCurrentAudioDevice() const;
	 
	 /**
	  * Update synchronization parameters
	  * @param emulationSpeed Current emulation speed factor
	  */
	 void UpdateSynchronization(float emulationSpeed);
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to audio system
	 AudioSystem& m_audioSystem;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Current configuration
	 SDLAudioOutputConfig m_config;
	 
	 // SDL audio device
	 SDL_AudioDeviceID m_audioDevice;
	 SDL_AudioSpec* m_audioSpec;
	 
	 // Audio buffers
	 struct {
		 std::vector<int16_t> primary;  // Primary audio buffer
		 std::vector<int16_t> mixing;   // Mixing buffer
		 std::vector<float> resampling; // Resampling buffer
		 size_t readPosition;          // Current read position
		 size_t writePosition;         // Current write position
		 std::atomic<float> fillLevel; // Buffer fill level
	 } m_buffer;
	 
	 // Synchronization data
	 struct {
		 float targetLatency;          // Target audio latency
		 float currentLatency;         // Current audio latency
		 float adjustmentFactor;       // Rate adjustment factor
		 bool enabled;                 // Whether synchronization is enabled
	 } m_sync;
	 
	 // Recording state
	 struct {
		 bool active;                  // Whether recording is active
		 std::string filePath;         // Recording file path
		 std::ofstream* fileStream;    // File stream
		 uint32_t bytesWritten;        // Bytes written
	 } m_recording;
	 
	 // Audio callbacks
	 std::unordered_map<int, std::function<void(int16_t*, int)>> m_audioCallbacks;
	 int m_nextCallbackId;
	 
	 // Thread synchronization
	 mutable std::mutex m_mutex;
	 std::atomic<bool> m_paused;
	 std::atomic<bool> m_running;
	 
	 // Master volume value
	 std::atomic<float> m_masterVolume;
	 
	 // Current audio device name
	 std::string m_currentDevice;
	 
	 /**
	  * Initialize SDL audio subsystem
	  * @return True if successful
	  */
	 bool InitializeSDLAudio();
	 
	 /**
	  * Create audio buffers
	  * @return True if successful
	  */
	 bool CreateAudioBuffers();
	 
	 /**
	  * Audio callback function (static, called by SDL)
	  * @param userdata User data pointer
	  * @param stream Audio stream buffer
	  * @param len Buffer length in bytes
	  */
	 static void AudioCallback(void* userdata, uint8_t* stream, int len);
	 
	 /**
	  * Process audio (called by audio callback)
	  * @param buffer Output buffer
	  * @param samples Number of samples to process
	  */
	 void ProcessAudio(int16_t* buffer, int samples);
	 
	 /**
	  * Fill audio buffer from audio system
	  * @param samples Number of samples to fill
	  * @return Number of samples actually filled
	  */
	 int FillBuffer(int samples);
	 
	 /**
	  * Apply volume to audio samples
	  * @param buffer Sample buffer
	  * @param samples Number of samples
	  * @param volume Volume level (0.0-1.0)
	  */
	 void ApplyVolume(int16_t* buffer, int samples, float volume);
	 
	 /**
	  * Resample audio
	  * @param input Input buffer
	  * @param output Output buffer
	  * @param inputSamples Number of input samples
	  * @param ratio Resampling ratio
	  * @return Number of output samples
	  */
	 int ResampleAudio(const int16_t* input, int16_t* output, int inputSamples, float ratio);
	 
	 /**
	  * Write audio to recording file
	  * @param buffer Audio buffer
	  * @param samples Number of samples
	  * @return True if successful
	  */
	 bool WriteToRecordingFile(const int16_t* buffer, int samples);
	 
	 /**
	  * Update buffer fill level
	  */
	 void UpdateBufferFillLevel();
	 
	 /**
	  * Adjust audio rate for synchronization
	  * @param emulationSpeed Current emulation speed
	  * @return Adjusted resampling ratio
	  */
	 float CalculateResamplingRatio(float emulationSpeed);
	 
	 /**
	  * Close audio device
	  */
	 void CloseAudioDevice();
	 
	 /**
	  * Close recording file
	  */
	 void CloseRecordingFile();
	 
	 /**
	  * Update WAV header in recording file
	  * @return True if successful
	  */
	 bool UpdateWAVHeader();
 };
 
 } // namespace NiXX32