/**
 * NiXX32System.h
 * Core system architecture for NiXX-32 arcade board emulation
 * 
 * This file defines the main system class that coordinates all components
 * of the NiXX-32 arcade board emulation including CPU, memory, graphics,
 * and audio subsystems.
 */

 #pragma once

 #include <memory>
 #include <string>
 #include <vector>
 
 #include "MemoryManager.h"
 #include "M68000CPU.h"
 #include "Z80CPU.h"
 #include "GraphicsSystem.h"
 #include "AudioSystem.h"
 #include "InputSystem.h"
 #include "ROMLoader.h"
 #include "Config.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class Debugger;
 
 /**
  * Defines the hardware variant of the NiXX-32 system
  */
 enum class HardwareVariant {
	NIXX32_ORIGINAL,  // Original 1989 model
	NIXX32_PLUS       // Enhanced 1992 model
 };

// Power management states
 enum class PowerState {
	FULL_POWER,      // All components running at full speed
	IDLE,            // Some components throttled after period of inactivity
	LOW_POWER,       // Most components throttled, minimal functionality
	SLEEP            // Only critical components active, waiting for wake event
};
 
 /**
  * Main system class that orchestrates the NiXX-32 emulation
  */
 class System {
 public:
	/**
	 * Constructor
	 * @param variant Hardware variant to emulate
	 * @param configPath Path to configuration file
	 */
	System(HardwareVariant variant = HardwareVariant::NIXX32_ORIGINAL, 
			const std::string& configPath = "nixx32_config.ini");
	
	/**
	 * Destructor
	 */
	~System();
	
	/**
	 * Helper method for destructor, ensures proper cleanup order to avoid dependency issues
	 */
	void Cleanup();

	/**
	 * Initialize the emulation system
	 * @param romPath Path to the game ROM
	 * @return True if initialization was successful
	 */
	bool Initialize(const std::string& romPath);
	
	/**
	 * Run a single emulation cycle
	 * @param deltaTime Time elapsed since last cycle in milliseconds
	 */
	void RunCycle(float deltaTime);
	
	/**
	 * Reset the emulation system
	 */
	void Reset();
	
	/**
	 * Pause/unpause the emulation
	 * @param paused True to pause, false to unpause
	 */
	void SetPaused(bool paused);
	
	/**
	 * Check if the emulation is paused
	 * @return True if paused
	 */
	bool IsPaused() const;
	
	/**
	 * Get the hardware variant being emulated
	 * @return Hardware variant
	 */
	HardwareVariant GetHardwareVariant() const;
	
	/**
	 * Get access to the memory manager
	 * @return Reference to memory manager
	 */
	MemoryManager& GetMemoryManager();
	
	/**
	 * Get access to the main CPU (68000)
	 * @return Reference to main CPU
	 */
	M68000CPU& GetMainCPU();
	
	/**
	 * Get access to the audio CPU (Z80)
	 * @return Reference to audio CPU
	 */
	Z80CPU& GetAudioCPU();
	
	/**
	 * Get access to the graphics system
	 * @return Reference to graphics system
	 */
	GraphicsSystem& GetGraphicsSystem();
	
	/**
	 * Get access to the audio system
	 * @return Reference to audio system
	 */
	AudioSystem& GetAudioSystem();
	
	/**
	 * Get access to the input system
	 * @return Reference to input system
	 */
	InputSystem& GetInputSystem();
	
	/**
	 * Attach a debugger to the system
	 * @param debugger Pointer to debugger instance
	 */
	void AttachDebugger(std::shared_ptr<Debugger> debugger);
	
	/**
	 * Get the system configuration
	 * @return Reference to configuration
	 */
	Config& GetConfig();
	
	/**
	 * Get the logger
	 * @return Reference to logger
	 */
	Logger& GetLogger();

	/**
	 * Enable or disable power management
	 * @param enable True to enable, false to disable
	 */
	void EnablePowerManagement(bool enable);

	/**
	 * Check if power management is enabled
	 * @return True if enabled
	 */
	bool IsPowerManagementEnabled() const;

	/**
	 * Get the current power state
	 * @return Current power state
	 */
	PowerState GetPowerState() const;

	/**
	 * Set the idle threshold (time before entering idle mode)
	 * @param milliseconds Threshold in milliseconds
	 */
	void SetIdleThreshold(uint64_t milliseconds);

	/**
	 * Set the sleep threshold (time before entering sleep mode)
	 * @param milliseconds Threshold in milliseconds
	 */
	void SetSleepThreshold(uint64_t milliseconds);	
 
 private:
	// Hardware variant being emulated
	HardwareVariant m_variant;

	// System state
	bool m_initialized;
	bool m_paused;

	// Power management variables
	PowerState m_powerState;
	uint64_t m_lastActivityTime;
	uint64_t m_idleThresholdMs;     // Time before entering idle state
	uint64_t m_sleepThresholdMs;    // Time before entering sleep state
	bool m_powerManagementEnabled;	

	// Core components
	std::unique_ptr<MemoryManager> m_memoryManager;
	std::unique_ptr<M68000CPU> m_mainCPU;
	std::unique_ptr<Z80CPU> m_audioCPU;
	std::unique_ptr<GraphicsSystem> m_graphicsSystem;
	std::unique_ptr<AudioSystem> m_audioSystem;
	std::unique_ptr<InputSystem> m_inputSystem;
	std::unique_ptr<ROMLoader> m_romLoader;
	std::unique_ptr<Config> m_config;
	std::unique_ptr<Logger> m_logger;
	
	// Optional debugger attachment
	std::shared_ptr<Debugger> m_debugger;
	
	/**
	 * Configure system based on hardware variant
	 */
	void ConfigureForVariant();
	
	/**
	 * Connect the various subsystems
	 */
	void ConnectSubsystems();
	
	/**
	 * Set up memory mappings based on hardware variant
	 */
	void SetupMemoryMap();

	/**
      * Set up the 68000 interrupt vector table
      */
     void SetupInterruptVectorTable();
    
     /**
      * Install default exception handlers
      */
     void InstallDefaultExceptionHandlers();
	
	/**
	 * Update the power state based on system activity
	 * @param deltaTime Time elapsed since last update
	 */
	void UpdatePowerState(float deltaTime);

	/**
	 * Set system power state
	 * @param newState New power state to set
	 */
	void SetPowerState(PowerState newState);

	/**
	 * Apply power settings to CPU subsystems
	 * @param powerFactor Power factor (0.0-1.0)
	 */
	void SetCPUPowerState(float powerFactor);

	/**
	 * Apply power settings to graphics subsystem
	 * @param powerFactor Power factor (0.0-1.0)
	 */
	void SetGraphicsPowerState(float powerFactor);

	/**
	 * Apply power settings to audio subsystem
	 * @param powerFactor Power factor (0.0-1.0)
	 */
	void SetAudioPowerState(float powerFactor);

	/**
	 * Convert power state enum to string
	 * @param state Power state
	 * @return String representation
	 */
	std::string PowerStateToString(PowerState state);
 };
 
 } // namespace NiXX32