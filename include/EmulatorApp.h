/**
 * EmulatorApp.h
 * Main application entry point for the NiXX-32 arcade board emulation
 * 
 * This file defines the main application class that serves as the entry point
 * and coordinator for the NiXX-32 arcade system emulator. It initializes and
 * manages all subsystems, handles user input, and controls the emulation loop.
 */

 #pragma once

 #include <cstdint>
 #include <string>
 #include <memory>
 #include <vector>
 #include <functional>
 #include <atomic>
 #include <thread>
 #include <mutex>
 
 #include "NiXX32System.h"
 #include "MemoryManager.h"
 #include "GraphicsSystem.h"
 #include "AudioSystem.h"
 #include "InputSystem.h"
 #include "Config.h"
 #include "Logger.h"
 #include "ROMLoader.h"
 #include "FileSystem.h"
 #include "SDLRenderer.h"
 #include "SDLAudioOutput.h"
 #include "Debugger.h"
 
 namespace NiXX32 {
 
 /**
  * Emulator application states
  */
 enum class EmulatorState {
	 INITIALIZING,   // Application is initializing
	 IDLE,           // Ready but no ROM loaded
	 RUNNING,        // Emulation is running
	 PAUSED,         // Emulation is paused
	 DEBUGGING,      // In debugging mode
	 RESETTING,      // System is resetting
	 SHUTDOWN        // Application is shutting down
 };
 
 /**
  * Emulator speed settings
  */
 enum class EmulationSpeed {
	 SLOWEST,        // 25% speed
	 SLOW,           // 50% speed
	 NORMAL,         // 100% speed
	 FAST,           // 200% speed
	 FASTEST,        // 400% speed
	 UNLIMITED       // Run as fast as possible
 };
 
 /**
  * UI mode for the emulator
  */
 enum class UIMode {
	 STANDARD,       // Standard UI with controls and display
	 FULLSCREEN,     // Fullscreen display
	 MINIMAL,        // Minimal UI (just the display)
	 DEBUG           // Debug UI with additional panels
 };
 
 /**
  * Command line arguments structure
  */
 struct CommandLineArgs {
	 std::string romPath;           // Path to ROM to load
	 std::string configPath;        // Path to configuration file
	 std::string logPath;           // Path for log file
	 bool debugMode;                // Start in debug mode
	 bool fullscreen;               // Start in fullscreen
	 bool audioDisabled;            // Disable audio
	 bool helpRequested;            // Help requested
	 bool versionRequested;         // Version requested
	 std::string saveStatePath;     // Path to save state to load
 };
 
 /**
  * Main emulator application class
  */
 class EmulatorApp {
 public:
	 /**
	  * Constructor
	  * @param argc Command line argument count
	  * @param argv Command line argument array
	  */
	 EmulatorApp(int argc, char* argv[]);
	 
	 /**
	  * Destructor
	  */
	 ~EmulatorApp();
	 
	 /**
	  * Initialize the application
	  * @return True if initialization was successful
	  */
	 bool Initialize();
	 
	 /**
	  * Run the application main loop
	  * @return Exit code
	  */
	 int Run();
	 
	 /**
	  * Load a ROM
	  * @param romPath Path to ROM file
	  * @return True if loading was successful
	  */
	 bool LoadROM(const std::string& romPath);
	 
	 /**
	  * Unload the current ROM
	  * @return True if successful
	  */
	 bool UnloadROM();
	 
	 /**
	  * Reset the emulated system
	  * @param hard True for hard reset, false for soft reset
	  * @return True if reset was successful
	  */
	 bool Reset(bool hard = false);
	 
	 /**
	  * Pause or resume emulation
	  * @param paused True to pause, false to resume
	  */
	 void SetPaused(bool paused);
	 
	 /**
	  * Check if emulation is paused
	  * @return True if paused
	  */
	 bool IsPaused() const;
	 
	 /**
	  * Set emulation speed
	  * @param speed Emulation speed
	  */
	 void SetEmulationSpeed(EmulationSpeed speed);
	 
	 /**
	  * Get current emulation speed
	  * @return Current emulation speed
	  */
	 EmulationSpeed GetEmulationSpeed() const;
	 
	 /**
	  * Set UI mode
	  * @param mode UI mode
	  */
	 void SetUIMode(UIMode mode);
	 
	 /**
	  * Get current UI mode
	  * @return Current UI mode
	  */
	 UIMode GetUIMode() const;
	 
	 /**
	  * Enter debugging mode
	  * @return True if successful
	  */
	 bool EnterDebugMode();
	 
	 /**
	  * Exit debugging mode
	  * @return True if successful
	  */
	 bool ExitDebugMode();
	 
	 /**
	  * Check if in debugging mode
	  * @return True if in debugging mode
	  */
	 bool IsInDebugMode() const;
	 
	 /**
	  * Save emulator state
	  * @param filePath Path to save file (empty for default)
	  * @param includeScreenshot Whether to include a screenshot
	  * @return True if successful
	  */
	 bool SaveState(const std::string& filePath = "", bool includeScreenshot = true);
	 
	 /**
	  * Load emulator state
	  * @param filePath Path to save file
	  * @return True if successful
	  */
	 bool LoadState(const std::string& filePath);
	 
	 /**
	  * Take a screenshot
	  * @param filePath Path to save file (empty for default)
	  * @return True if successful
	  */
	 bool TakeScreenshot(const std::string& filePath = "");
	 
	 /**
	  * Start audio/video recording
	  * @param filePath Path to save file (empty for default)
	  * @return True if successful
	  */
	 bool StartRecording(const std::string& filePath = "");
	 
	 /**
	  * Stop recording
	  * @return True if successful
	  */
	 bool StopRecording();
	 
	 /**
	  * Check if recording is active
	  * @return True if recording
	  */
	 bool IsRecording() const;
	 
	 /**
	  * Open configuration dialog
	  */
	 void OpenConfigDialog();
	 
	 /**
	  * Process SDL events
	  */
	 void ProcessEvents();
	 
	 /**
	  * Get the current emulator state
	  * @return Current state
	  */
	 EmulatorState GetState() const;
	 
	 /**
	  * Get the emulated system
	  * @return Reference to emulated system
	  */
	 System& GetSystem();
	 
	 /**
	  * Get the debugger
	  * @return Reference to debugger
	  */
	 Debugger& GetDebugger();
	 
	 /**
	  * Get the configuration
	  * @return Reference to configuration
	  */
	 Config& GetConfig();
	 
	 /**
	  * Get the logger
	  * @return Reference to logger
	  */
	 Logger& GetLogger();
	 
	 /**
	  * Check if a ROM is loaded
	  * @return True if ROM is loaded
	  */
	 bool IsROMLoaded() const;
	 
	 /**
	  * Get information about the loaded ROM
	  * @return ROM information
	  */
	 ROMInfo GetLoadedROMInfo() const;
	 
	 /**
	  * Register a state change callback
	  * @param callback Function to call when state changes
	  * @return Callback ID
	  */
	 int RegisterStateCallback(std::function<void(EmulatorState)> callback);
	 
	 /**
	  * Remove a state change callback
	  * @param callbackId Callback ID
	  * @return True if successful
	  */
	 bool RemoveStateCallback(int callbackId);
	 
	 /**
	  * Parse command line arguments
	  * @param argc Argument count
	  * @param argv Argument array
	  * @return Parsed arguments
	  */
	 static CommandLineArgs ParseCommandLine(int argc, char* argv[]);
	 
	 /**
	  * Get application version
	  * @return Version string
	  */
	 static std::string GetVersion();
	 
	 /**
	  * Show help information
	  */
	 static void ShowHelp();
 
 private:
	 // Command line arguments
	 CommandLineArgs m_args;
	 
	 // Core subsystems
	 std::unique_ptr<Logger> m_logger;
	 std::unique_ptr<Config> m_config;
	 std::unique_ptr<FileSystem> m_fileSystem;
	 std::unique_ptr<System> m_system;
	 std::unique_ptr<ROMLoader> m_romLoader;
	 std::unique_ptr<Debugger> m_debugger;
	 
	 // Output systems
	 std::unique_ptr<SDLRenderer> m_renderer;
	 std::unique_ptr<SDLAudioOutput> m_audioOutput;
	 
	 // Current state
	 std::atomic<EmulatorState> m_state;
	 
	 // Emulation parameters
	 EmulationSpeed m_speed;
	 UIMode m_uiMode;
	 bool m_romLoaded;
	 bool m_recording;
	 
	 // Emulation statistics
	 struct {
		 float fps;
		 float audioLatency;
		 uint64_t frameCount;
		 uint64_t cycleCount;
		 float emulationSpeed;
	 } m_stats;
	 
	 // Emulation timing
	 struct {
		 uint64_t lastFrameTime;
		 uint64_t frameStartTime;
		 float targetFrameTime;
		 float accumulatedTime;
		 int framesSkipped;
	 } m_timing;
	 
	 // Thread management
	 std::unique_ptr<std::thread> m_emulationThread;
	 std::atomic<bool> m_threadRunning;
	 std::mutex m_stateMutex;
	 
	 // State change callbacks
	 std::unordered_map<int, std::function<void(EmulatorState)>> m_stateCallbacks;
	 int m_nextCallbackId;
	 
	 /**
	  * Initialize subsystems
	  * @return True if initialization was successful
	  */
	 bool InitializeSubsystems();
	 
	 /**
	  * Initialize SDL
	  * @return True if initialization was successful
	  */
	 bool InitializeSDL();
	 
	 /**
	  * Create configuration based on settings
	  * @return True if successful
	  */
	 bool CreateConfiguration();
	 
	 /**
	  * Run the emulation loop
	  */
	 void EmulationLoop();
	 
	 /**
	  * Process a single emulation frame
	  * @return True if frame was processed
	  */
	 bool ProcessFrame();
	 
	 /**
	  * Update emulation timing
	  * @return Time until next frame in milliseconds
	  */
	 float UpdateTiming();
	 
	 /**
	  * Update statistics
	  */
	 void UpdateStats();
	 
	 /**
	  * Set emulator state
	  * @param state New state
	  */
	 void SetState(EmulatorState state);
	 
	 /**
	  * Notify state change callbacks
	  * @param state New state
	  */
	 void NotifyStateCallbacks(EmulatorState state);
	 
	 /**
	  * Clean up resources
	  */
	 void Cleanup();
	 
	 /**
	  * Initialize and start emulation thread
	  * @return True if successful
	  */
	 bool StartEmulationThread();
	 
	 /**
	  * Stop emulation thread
	  */
	 void StopEmulationThread();
	 
	 /**
	  * Apply configuration changes
	  * @param reloadSubsystems Whether to reload subsystems
	  * @return True if successful
	  */
	 bool ApplyConfiguration(bool reloadSubsystems = false);
	 
	 /**
	  * Initialize UI
	  * @return True if successful
	  */
	 bool InitializeUI();
	 
	 /**
	  * Update UI
	  */
	 void UpdateUI();
	 
	 /**
	  * Handle keyboard input
	  * @param key Key code
	  * @param pressed True if key was pressed, false if released
	  * @param modifiers Keyboard modifier flags
	  */
	 void HandleKeyboard(int key, bool pressed, int modifiers);
	 
	 /**
	  * Handle window events
	  * @param width Window width
	  * @param height Window height
	  * @param fullscreen Whether window is fullscreen
	  */
	 void HandleWindowEvent(int width, int height, bool fullscreen);
	 
	 /**
	  * Convert SDL input to emulator input
	  */
	 void ProcessSDLInput();
	 
	 /**
	  * Load configuration
	  * @return True if successful
	  */
	 bool LoadConfiguration();
	 
	 /**
	  * Save configuration
	  * @return True if successful
	  */
	 bool SaveConfiguration();
 };
 
 } // namespace NiXX32