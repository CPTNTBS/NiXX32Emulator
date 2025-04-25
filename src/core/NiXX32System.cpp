/**
 * NiXX32System.cpp
 * Implementation of core system architecture for NiXX-32 arcade board emulation
 */

#include "NiXX32System.h"
#include "Debugger.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <stdexcept>

// Define memory map constants to avoid hard-coding addresses
namespace {
    // Base addresses for different hardware variants
    struct MemoryAddresses {
        // Original hardware (1989)
        struct Original {
            static constexpr uint32_t ROM_BASE         = 0x000000;
            static constexpr uint32_t ROM_SIZE         = 0x200000; // 2MB
            static constexpr uint32_t MAIN_RAM_BASE    = 0x200000;
            static constexpr uint32_t MAIN_RAM_SIZE    = 0x100000; // 1MB
            static constexpr uint32_t VIDEO_RAM_BASE   = 0x300000;
            static constexpr uint32_t VIDEO_RAM_SIZE   = 0x080000; // 512KB
            static constexpr uint32_t IO_REGISTERS_BASE = 0x400000;
            static constexpr uint32_t IO_REGISTERS_SIZE = 0x010000; // 64KB
            static constexpr uint32_t SOUND_RAM_BASE   = 0x410000;
            static constexpr uint32_t SOUND_RAM_SIZE   = 0x020000; // 128KB
        };
        
        // Enhanced hardware (1992)
        struct Plus {
            static constexpr uint32_t ROM_BASE         = 0x000000;
            static constexpr uint32_t ROM_SIZE         = 0x400000; // 4MB
            static constexpr uint32_t MAIN_RAM_BASE    = 0x400000;
            static constexpr uint32_t MAIN_RAM_SIZE    = 0x200000; // 2MB
            static constexpr uint32_t VIDEO_RAM_BASE   = 0x600000;
            static constexpr uint32_t VIDEO_RAM_SIZE   = 0x100000; // 1MB
            static constexpr uint32_t IO_REGISTERS_BASE = 0x700000;
            static constexpr uint32_t IO_REGISTERS_SIZE = 0x010000; // 64KB
            static constexpr uint32_t SOUND_RAM_BASE   = 0x710000;
            static constexpr uint32_t SOUND_RAM_SIZE   = 0x040000; // 256KB
        };
    };
    
    // Z80 memory map (same for both variants)
    struct Z80Memory {
        static constexpr uint16_t ROM_BASE  = 0x0000;
        static constexpr uint16_t ROM_SIZE  = 0x8000; // 32KB
        static constexpr uint16_t RAM_BASE  = 0x8000;
        static constexpr uint16_t RAM_SIZE  = 0x8000; // 32KB
    };
    
    // IO Register address ranges
    struct IORegisters {
        static constexpr uint32_t GRAPHICS_BASE = 0x000000; // Offset from IO_REGISTERS_BASE
        static constexpr uint32_t GRAPHICS_SIZE = 0x000040; // 64 bytes
        static constexpr uint32_t INPUT_BASE    = 0x001000; // Offset from IO_REGISTERS_BASE
        static constexpr uint32_t INPUT_SIZE    = 0x000100; // 256 bytes
        static constexpr uint32_t AUDIO_BASE    = 0x002000; // Offset from IO_REGISTERS_BASE
        static constexpr uint32_t AUDIO_SIZE    = 0x000100; // 256 bytes
    };

}

namespace NiXX32 {

System::System(HardwareVariant variant, const std::string& configPath)
    : m_variant(variant),
      m_initialized(false),
      m_paused(false),
      m_debugger(nullptr)
{
    try {
		// Create power state variables:
		m_powerState = PowerState::FULL_POWER;
		m_lastActivityTime = 0;
		m_idleThresholdMs = 300000;     // 5 minutes default idle threshold
		m_sleepThresholdMs = 1800000;   // 30 minutes default sleep threshold
		m_powerManagementEnabled = false; // Disabled by default for original hardware

		// Create logger first so other components can use it
        m_logger = std::make_unique<Logger>();
        if (!m_logger->Initialize()) {
            throw std::runtime_error("Failed to initialize logger");
        }
        m_logger->Info("System", "Initializing NiXX-32 Emulation System");
		
        // Create configuration manager
        m_config = std::make_unique<Config>(*this, *m_logger, configPath);
        
        // Create memory manager
        m_memoryManager = std::make_unique<MemoryManager>(*this, *m_logger);
        
        // Create ROM loader
        m_romLoader = std::make_unique<ROMLoader>(*this, *m_memoryManager, *m_logger);
        
        // Create audio system first (before audio CPU)
        m_audioSystem = std::make_unique<AudioSystem>(*this, *m_memoryManager, *m_logger);
        
        // Create CPU subsystems
        m_mainCPU = std::make_unique<M68000CPU>(*this, *m_memoryManager, *m_logger);
        m_audioCPU = std::make_unique<Z80CPU>(*this, *m_memoryManager, *m_logger, *m_audioSystem);
        
        // Now that audio CPU exists, set it in the audio system
        m_audioSystem->SetAudioCPU(m_audioCPU.get());
        
        // Create graphics system
        m_graphicsSystem = std::make_unique<GraphicsSystem>(*this, *m_memoryManager, *m_logger);
        
        // Create input system
        m_inputSystem = std::make_unique<InputSystem>(*this, *m_memoryManager, *m_logger);
        
    } catch (const std::exception& e) {
        if (m_logger) {
            m_logger->Error("System", std::string("Exception during initialization: ") + e.what());
        }
        // Clean up resources in case of failure
        Cleanup();
        throw; // Re-throw the exception
    }
}

System::~System() {
    m_logger->Info("System", "Shutting down NiXX-32 Emulation System");
    Cleanup();
}

void System::Cleanup() {
    // First detach debugger
    if (m_debugger) {
        m_logger->Info("System", "Detaching debugger");
        m_debugger.reset();
    }
    
    // Release components in reverse order of dependencies
    if (m_inputSystem) {
        m_logger->Info("System", "Shutting down input system");
        m_inputSystem.reset();
    }
    
    if (m_audioSystem) {
        m_logger->Info("System", "Shutting down audio system");
        m_audioSystem.reset();
    }
    
    if (m_graphicsSystem) {
        m_logger->Info("System", "Shutting down graphics system");
        m_graphicsSystem.reset();
    }
    
    if (m_audioCPU) {
        m_logger->Info("System", "Shutting down audio CPU");
        m_audioCPU.reset();
    }
    
    if (m_mainCPU) {
        m_logger->Info("System", "Shutting down main CPU");
        m_mainCPU.reset();
    }
    
    if (m_romLoader) {
        m_logger->Info("System", "Shutting down ROM loader");
        m_romLoader.reset();
    }
    
    if (m_memoryManager) {
        m_logger->Info("System", "Shutting down memory manager");
        m_memoryManager.reset();
    }
    
    if (m_config) {
        m_logger->Info("System", "Shutting down configuration manager");
        m_config.reset();
    }
    
    // Logger is the last to be released
    if (m_logger) {
        m_logger->Info("System", "Shutting down logger");
        m_logger->Shutdown();
        m_logger.reset();
    }
}

bool System::Initialize(const std::string& romPath) {
    if (m_initialized) {
        m_logger->Warning("System", "System already initialized");
        return true; // Already initialized is not an error
    }
    
    m_logger->Info("System", "Beginning system initialization");
    
    try {
        // Load configuration
        if (!m_config->Initialize()) {
            throw std::runtime_error("Failed to initialize configuration");
        }
        
        // Configure system based on hardware variant
        ConfigureForVariant();
        
        // Initialize memory manager
        if (!m_memoryManager->Initialize(m_variant)) {
            throw std::runtime_error("Failed to initialize memory manager");
        }
        
        // Setup memory mappings
        SetupMemoryMap();

		// Setup interrupt vector table
		SetupInterruptVectorTable();
        
        // Initialize ROM loader
        if (!m_romLoader->Initialize()) {
            throw std::runtime_error("Failed to initialize ROM loader");
        }
        
        // Initialize main CPU
        uint32_t mainCpuClockSpeed = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 16000000 : 20000000; // 16 MHz or 20 MHz
        if (!m_mainCPU->Initialize(mainCpuClockSpeed)) {
            throw std::runtime_error("Failed to initialize main CPU");
        }
        
        // Initialize audio system
        AudioHardwareVariant audioVariant = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ?
                                           AudioHardwareVariant::NIXX32_ORIGINAL :
                                           AudioHardwareVariant::NIXX32_PLUS;
        if (!m_audioSystem->Initialize(audioVariant)) {
            throw std::runtime_error("Failed to initialize audio system");
        }
        
        // Initialize audio CPU
        uint32_t audioCpuClockSpeed = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 8000000 : 10000000; // 8 MHz or 10 MHz
        if (!m_audioCPU->Initialize(audioCpuClockSpeed)) {
            throw std::runtime_error("Failed to initialize audio CPU");
        }
        
        // Initialize graphics system
        GraphicsHardwareVariant graphicsVariant = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ?
                                                GraphicsHardwareVariant::NIXX32_ORIGINAL :
                                                GraphicsHardwareVariant::NIXX32_PLUS;
        if (!m_graphicsSystem->Initialize(graphicsVariant)) {
            throw std::runtime_error("Failed to initialize graphics system");
        }
        
        // Initialize input system
        if (!m_inputSystem->Initialize(m_variant)) {
            throw std::runtime_error("Failed to initialize input system");
        }

		// Load power management settings from config
		if (m_config->HasOption("system.powerManagementEnabled")) {
			m_powerManagementEnabled = m_config->GetBool("system.powerManagementEnabled");
		}
		
		if (m_config->HasOption("system.idleThresholdMs")) {
			m_idleThresholdMs = static_cast<uint64_t>(m_config->GetInt("system.idleThresholdMs"));
		}
		
		if (m_config->HasOption("system.sleepThresholdMs")) {
			m_sleepThresholdMs = static_cast<uint64_t>(m_config->GetInt("system.sleepThresholdMs"));
		}	
	
        // Connect the subsystems to ensure communication
        ConnectSubsystems();

	    // Initialize power management
	    SetPowerState(PowerState::FULL_POWER);
    	auto now = std::chrono::steady_clock::now();
    	m_lastActivityTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        	now.time_since_epoch()).count();		
		
        // Load ROM if a path was provided
        if (!romPath.empty()) {
            // Verify ROM compatibility with hardware variant
            ROMInfo romInfo = m_romLoader->GetROMInfo(romPath);
            if (romInfo.variant != m_variant) {
                m_logger->Warning("System", "ROM variant does not match hardware variant, attempting to load anyway");
            }
            
            if (!m_romLoader->LoadROM(romPath)) {
                throw std::runtime_error("Failed to load ROM: " + romPath);
            }
            
            m_logger->Info("System", "Successfully loaded ROM: " + romPath);
        }
        
        m_initialized = true;
        m_logger->Info("System", "System initialization complete");
        
        return true;
    }
    catch (const std::exception& e) {
        m_logger->Error("System", std::string("Initialization failed: ") + e.what());
        return false;
    }
}

void System::RunCycle(float deltaTime) {
    if (!m_initialized) {
        m_logger->Warning("System", "Attempt to run cycle on uninitialized system");
        // Sleep briefly to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return;
    }
    
    if (m_paused) {
        // If paused, sleep briefly but still allow debugger updates
        if (m_debugger) {
            m_debugger->Update();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return;
    }
    
    try {

		// Update power management state
		UpdatePowerState(deltaTime);
		
		// Scale deltaTime based on power state if needed
		float adjustedDeltaTime = deltaTime;
		if (m_powerState == PowerState::SLEEP) {
			// In sleep mode, we can actually reduce update frequency
			// to save host CPU resources too (not just emulated hardware)
			static float sleepAccumulator = 0.0f;
			sleepAccumulator += deltaTime;
			
			// Only process updates at 1/10th the normal rate in sleep mode
			if (sleepAccumulator < 0.1f) {
				return; // Skip this update cycle
			}
			
			adjustedDeltaTime = sleepAccumulator;
			sleepAccumulator = 0.0f;
		}

		// Calculate CPU cycles based on elapsed time and clock speeds
        uint32_t mainCpuCycles = static_cast<uint32_t>(m_mainCPU->GetClockSpeed() * 1000 * adjustedDeltaTime);
        uint32_t audioCpuCycles = static_cast<uint32_t>(m_audioCPU->GetClockSpeed() * 1000 * adjustedDeltaTime);
        
        // Allow debugger to control execution if attached
        if (m_debugger) {
            m_debugger->Update();
            
            // If debugger is active, pause regular execution
            if (m_debugger->IsActive()) {
                return;
            }
        }
        
        // Execute main CPU cycles
        int executedMainCycles = m_mainCPU->Execute(mainCpuCycles);
        
        // Execute audio CPU cycles - keep in sync with main CPU
        float mainCpuRatio = static_cast<float>(executedMainCycles) / mainCpuCycles;
        int adjustedAudioCycles = static_cast<int>(audioCpuCycles * mainCpuRatio);
        int executedAudioCycles = m_audioCPU->Execute(adjustedAudioCycles);
        
        // Update subsystems - they need to know the actual time elapsed
        // which might be different from adjustedDeltaTime if CPU execution was slower than expected
        float actualDeltaTime = adjustedDeltaTime * (static_cast<float>(executedMainCycles) / mainCpuCycles);
        
        // Update subsystems with the calculated actual time
        m_graphicsSystem->Update(actualDeltaTime);
        m_audioSystem->Update(actualDeltaTime);
        m_inputSystem->Update(actualDeltaTime);



	}
    catch (const std::exception& e) {
        m_logger->Error("System", std::string("Error during cycle execution: ") + e.what());
        // Consider pausing the system on error
        SetPaused(true);
    }
}

void System::Reset() {
    m_logger->Info("System", "Resetting system");
    
    try {
        // Reset CPUs
        m_mainCPU->Reset();
        m_audioCPU->Reset();
        
        // Reset subsystems
        m_memoryManager->Reset();
        m_graphicsSystem->Reset();
        m_audioSystem->Reset();
        m_inputSystem->Reset();
        
        // Reset debugger if attached
        if (m_debugger) {
            m_debugger->Reset();
        }
        
        m_logger->Info("System", "System reset complete");
    }
    catch (const std::exception& e) {
        m_logger->Error("System", std::string("Error during system reset: ") + e.what());
        // Consider pausing the system on error
        SetPaused(true);
    }
}

void System::SetPaused(bool paused) {
    if (m_paused != paused) {
        m_paused = paused;
        m_logger->Info("System", paused ? "System paused" : "System resumed");
        
        try {
            // Pause/resume audio to avoid buffer underruns
            if (m_audioSystem) {
                m_audioSystem->SetPaused(paused);
            }
            
            // Notify debugger of pause state change
            if (m_debugger) {
                // If we're pausing, activate the debugger
                if (paused) {
                    m_debugger->PauseEmulation();
                } else {
                    m_debugger->ResumeEmulation();
                }
            }
        }
        catch (const std::exception& e) {
            m_logger->Error("System", std::string("Error changing pause state: ") + e.what());
        }
    }
}

bool System::IsPaused() const {
    return m_paused;
}

HardwareVariant System::GetHardwareVariant() const {
    return m_variant;
}

MemoryManager& System::GetMemoryManager() {
    if (!m_memoryManager) {
        throw std::runtime_error("Memory manager not initialized");
    }
    return *m_memoryManager;
}

M68000CPU& System::GetMainCPU() {
    if (!m_mainCPU) {
        throw std::runtime_error("Main CPU not initialized");
    }
    return *m_mainCPU;
}

Z80CPU& System::GetAudioCPU() {
    if (!m_audioCPU) {
        throw std::runtime_error("Audio CPU not initialized");
    }
    return *m_audioCPU;
}

GraphicsSystem& System::GetGraphicsSystem() {
    if (!m_graphicsSystem) {
        throw std::runtime_error("Graphics system not initialized");
    }
    return *m_graphicsSystem;
}

AudioSystem& System::GetAudioSystem() {
    if (!m_audioSystem) {
        throw std::runtime_error("Audio system not initialized");
    }
    return *m_audioSystem;
}

InputSystem& System::GetInputSystem() {
    if (!m_inputSystem) {
        throw std::runtime_error("Input system not initialized");
    }
    return *m_inputSystem;
}

void System::AttachDebugger(std::shared_ptr<Debugger> debugger) {
    m_debugger = debugger;
    if (m_debugger) {
        m_logger->Info("System", "Debugger attached");
        
        // Initialize the debugger with the current system state
        if (m_initialized) {
            m_debugger->Initialize();
        }
    } else {
        m_logger->Info("System", "Debugger detached");
    }
}

Config& System::GetConfig() {
    if (!m_config) {
        throw std::runtime_error("Configuration not initialized");
    }
    return *m_config;
}

Logger& System::GetLogger() {
    if (!m_logger) {
        throw std::runtime_error("Logger not initialized");
    }
    return *m_logger;
}

void System::ConfigureForVariant() {
    m_logger->Info("System", std::string("Configuring system for variant: ") + 
                   (m_variant == HardwareVariant::NIXX32_ORIGINAL ? "Original" : "Plus"));
    
    // Load variant-specific configuration values
    if (m_variant == HardwareVariant::NIXX32_ORIGINAL) {
        // Set up for original hardware (1989)
        m_config->SetInt("system.mainCpuSpeed", 16);        // 16 MHz
        m_config->SetInt("system.audioCpuSpeed", 8);        // 8 MHz
        m_config->SetInt("memory.mainRamSize", 1024);       // 1 MB
        m_config->SetInt("memory.videoRamSize", 512);       // 512 KB
        m_config->SetInt("memory.soundRamSize", 128);       // 128 KB
        m_config->SetInt("memory.maxRomSize", 2048);        // 2 MB
        m_config->SetInt("graphics.maxSprites", 96);        // 96 sprites
        m_config->SetInt("graphics.maxColors", 4096);       // 4,096 colors
        m_config->SetInt("audio.sampleRate", 11025);        // 11.025 kHz
        m_config->SetInt("audio.fmChannels", 8);            // 8 FM channels
        m_config->SetInt("audio.pcmChannels", 8);           // 8 PCM channels

		// Original hardware did not have sophisticated power management
        m_powerManagementEnabled = false;
        m_idleThresholdMs = 600000;     // 10 minutes (if enabled)
        m_sleepThresholdMs = 1800000;   // 30 minutes (if enabled)		
    } else {
        // Set up for enhanced hardware (1992)
        m_config->SetInt("system.mainCpuSpeed", 20);        // 20 MHz
        m_config->SetInt("system.audioCpuSpeed", 10);       // 10 MHz
        m_config->SetInt("memory.mainRamSize", 2048);       // 2 MB
        m_config->SetInt("memory.videoRamSize", 1024);      // 1 MB
        m_config->SetInt("memory.soundRamSize", 256);       // 256 KB
        m_config->SetInt("memory.maxRomSize", 4096);        // 4 MB
        m_config->SetInt("graphics.maxSprites", 128);       // 128 sprites
        m_config->SetInt("graphics.maxColors", 8192);       // 8,192 colors
        m_config->SetInt("audio.sampleRate", 22050);        // 22.05 kHz
        m_config->SetInt("audio.fmChannels", 12);           // 12 FM channels
        m_config->SetInt("audio.pcmChannels", 16);          // 16 PCM channels

        // Enhanced hardware has better power management
        m_powerManagementEnabled = true;
        m_idleThresholdMs = 300000;     // 5 minutes
        m_sleepThresholdMs = 900000;    // 15 minutes
    }

    m_config->SetBool("system.powerManagementEnabled", m_powerManagementEnabled);
    m_config->SetInt("system.idleThresholdMs", static_cast<int>(m_idleThresholdMs));
    m_config->SetInt("system.sleepThresholdMs", static_cast<int>(m_sleepThresholdMs));	
}

void System::ConnectSubsystems() {
    m_logger->Info("System", "Connecting subsystems");
    
    try {
        // Connect audio CPU to audio system
        m_audioSystem->SetAudioCPU(m_audioCPU.get());
        
        // Calculate the base address for I/O registers
        uint32_t ioRegBase = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 
                            MemoryAddresses::Original::IO_REGISTERS_BASE : 
                            MemoryAddresses::Plus::IO_REGISTERS_BASE;
                            
        uint32_t audioRegionStart = ioRegBase + IORegisters::AUDIO_BASE;
        
        // Register port I/O handlers for audio CPU
        for (uint8_t port = 0x00; port <= 0x0F; ++port) {
            uint8_t finalPort = port; // Make a copy for the lambda capture
            m_audioCPU->RegisterPortHooks(finalPort,
                // Read handler
                [this, finalPort]() -> uint8_t { 
                    return m_audioSystem->HandleRegisterRead(finalPort); 
                },
                // Write handler
                [this, finalPort](uint8_t value) { 
                    m_audioSystem->HandleRegisterWrite(finalPort, value); 
                }
            );
        }
        
        // Set up audio command/status communication between main CPU and audio CPU
        // These handlers allow the main CPU to send commands to the audio CPU
        MemoryRegion* audioRegion = m_memoryManager->GetRegionByName("SOUND_RAM");
        if (!audioRegion) {
            throw std::runtime_error("Audio region not found in memory map");
        }
        
        // Set up synchronization mechanism between CPUs
        uint32_t syncAddress = audioRegionStart + 0x00; // First byte of audio region
        
        // Handle interrupts from audio CPU to main CPU 
        m_audioCPU->RegisterExecutionHook(0x0038, [this]() {
            // Z80 RST 38h (interrupt) handler - signal main CPU
            m_mainCPU->SetInterruptLevel(InterruptLevel::IPL_4);
        });
        
        // Set up VBLANK interrupt handler
        // This interrupt occurs at the vertical blanking interval of the display
        // (typically 60Hz) and is used for timing-critical operations
        m_mainCPU->RegisterHook(0x00000068, [this]() {
            // VBLANK interrupt handler (Exception vector 0x68)
            // Update graphics system with frame timing
            const float frameTime = 1.0f / 60.0f; // 60Hz refresh rate
            m_graphicsSystem->Update(frameTime);
            
            // Signal audio CPU about VBLANK
            // Write to a shared memory location or trigger Z80 interrupt
            m_audioCPU->TriggerInterrupt(Z80InterruptType::INT);
        });
        
        // Connect input system to interrupt handlers
        // When input events occur (e.g., coins inserted, buttons pressed),
        // they can trigger interrupts to notify the CPUs
        m_inputSystem->RegisterInputCallback([this](uint8_t playerIndex, const InputState& state) {
            // Special inputs can trigger interrupts
            if (state.serviceControls[ServiceControl::SERVICE_BUTTON]) {
                // Service button triggers a special interrupt
                m_mainCPU->SetInterruptLevel(InterruptLevel::IPL_2);
            }
            if (state.coinControls[CoinControl::COIN_1] || 
                state.coinControls[CoinControl::COIN_2]) {
                // Coin insertion triggers a different interrupt
                m_mainCPU->SetInterruptLevel(InterruptLevel::IPL_3);
            }
        });
        
        m_logger->Info("System", "Subsystems connected successfully");
    }
    catch (const std::exception& e) {
        m_logger->Error("System", std::string("Connecting subsystems failed: ") + e.what());
        throw; // Re-throw to be caught by Initialize()
    }
}

void System::SetupMemoryMap() {
    m_logger->Info("System", "Setting up memory map");

    try {
        // Use constants for addresses based on hardware variant
        uint32_t romBase, romSize;
        uint32_t mainRamBase, mainRamSize;
        uint32_t videoRamBase, videoRamSize;
        uint32_t ioRegBase, ioRegSize;
        uint32_t soundRamBase, soundRamSize;
        
        if (m_variant == HardwareVariant::NIXX32_ORIGINAL) {
            romBase = MemoryAddresses::Original::ROM_BASE;
            romSize = MemoryAddresses::Original::ROM_SIZE;
            mainRamBase = MemoryAddresses::Original::MAIN_RAM_BASE;
            mainRamSize = MemoryAddresses::Original::MAIN_RAM_SIZE;
            videoRamBase = MemoryAddresses::Original::VIDEO_RAM_BASE;
            videoRamSize = MemoryAddresses::Original::VIDEO_RAM_SIZE;
            ioRegBase = MemoryAddresses::Original::IO_REGISTERS_BASE;
            ioRegSize = MemoryAddresses::Original::IO_REGISTERS_SIZE;
            soundRamBase = MemoryAddresses::Original::SOUND_RAM_BASE;
            soundRamSize = MemoryAddresses::Original::SOUND_RAM_SIZE;
        } else {
            romBase = MemoryAddresses::Plus::ROM_BASE;
            romSize = MemoryAddresses::Plus::ROM_SIZE;
            mainRamBase = MemoryAddresses::Plus::MAIN_RAM_BASE;
            mainRamSize = MemoryAddresses::Plus::MAIN_RAM_SIZE;
            videoRamBase = MemoryAddresses::Plus::VIDEO_RAM_BASE;
            videoRamSize = MemoryAddresses::Plus::VIDEO_RAM_SIZE;
            ioRegBase = MemoryAddresses::Plus::IO_REGISTERS_BASE;
            ioRegSize = MemoryAddresses::Plus::IO_REGISTERS_SIZE;
            soundRamBase = MemoryAddresses::Plus::SOUND_RAM_BASE;
            soundRamSize = MemoryAddresses::Plus::SOUND_RAM_SIZE;
        }
        
        // Override with configuration values if available
        if (m_config->HasOption("memory.mainRamSize")) {
            mainRamSize = m_config->GetInt("memory.mainRamSize") * 1024; // Convert KB to bytes
        }
        if (m_config->HasOption("memory.videoRamSize")) {
            videoRamSize = m_config->GetInt("memory.videoRamSize") * 1024;
        }
        if (m_config->HasOption("memory.soundRamSize")) {
            soundRamSize = m_config->GetInt("memory.soundRamSize") * 1024;
        }
        if (m_config->HasOption("memory.maxRomSize")) {
            romSize = m_config->GetInt("memory.maxRomSize") * 1024;
        }
        
        // Define memory regions for 68000 main CPU
        
        // ROM area
        m_memoryManager->DefineRegion("ROM", romBase, romSize, 
                                      MemoryAccess::READ_ONLY, MemoryRegionType::ROM);
        
        // Main RAM
        m_memoryManager->DefineRegion("MAIN_RAM", mainRamBase, mainRamSize, 
                                      MemoryAccess::READ_WRITE, MemoryRegionType::MAIN_RAM);
        
        // Video RAM
        m_memoryManager->DefineRegion("VIDEO_RAM", videoRamBase, videoRamSize, 
                                      MemoryAccess::READ_WRITE, MemoryRegionType::VIDEO_RAM);
        
        // IO Registers
        m_memoryManager->DefineRegion("IO_REGISTERS", ioRegBase, ioRegSize, 
                                      MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
        
        // Sound RAM (shared with Z80)
        m_memoryManager->DefineRegion("SOUND_RAM", soundRamBase, soundRamSize, 
                                      MemoryAccess::READ_WRITE, MemoryRegionType::SOUND_RAM);
        
        // Define memory regions for Z80 audio CPU
        
        // Z80 Program ROM
        m_memoryManager->DefineRegion("Z80_ROM", Z80Memory::ROM_BASE, Z80Memory::ROM_SIZE, 
                                      MemoryAccess::READ_ONLY, MemoryRegionType::ROM);
        
        // Z80 RAM
        m_memoryManager->DefineRegion("Z80_RAM", Z80Memory::RAM_BASE, Z80Memory::RAM_SIZE, 
                                      MemoryAccess::READ_WRITE, MemoryRegionType::SOUND_RAM);
        
        // Set up memory-mapped I/O handlers
        auto graphicsRegion = m_memoryManager->GetRegionByName("IO_REGISTERS");
        if (!graphicsRegion) {
            throw std::runtime_error("Failed to get IO_REGISTERS region");
        }
        
        // Calculate actual address ranges for I/O handlers
        uint32_t graphicsRegionStart = ioRegBase + IORegisters::GRAPHICS_BASE;
        uint32_t graphicsRegionEnd = graphicsRegionStart + IORegisters::GRAPHICS_SIZE - 1;
        uint32_t inputRegionStart = ioRegBase + IORegisters::INPUT_BASE;
        uint32_t inputRegionEnd = inputRegionStart + IORegisters::INPUT_SIZE - 1;
        uint32_t audioRegionStart = ioRegBase + IORegisters::AUDIO_BASE;
        uint32_t audioRegionEnd = audioRegionStart + IORegisters::AUDIO_SIZE - 1;
        
        m_memoryManager->SetReadHandlers("IO_REGISTERS",
            // 8-bit read handler
            [this, graphicsRegionStart, graphicsRegionEnd, inputRegionStart, inputRegionEnd, 
             audioRegionStart, audioRegionEnd](uint32_t address) -> uint8_t {
                // Check if address is within valid ranges
                if (address >= graphicsRegionStart && address <= graphicsRegionEnd) {
                    // Graphics system registers
                    return m_graphicsSystem->HandleRegisterRead(address) & 0xFF;
                } else if (address >= inputRegionStart && address <= inputRegionEnd) {
                    // Input system registers
                    return m_inputSystem->HandleRegisterRead(address) & 0xFF;
                } else if (address >= audioRegionStart && address <= audioRegionEnd) {
                    // Audio system registers (main CPU side)
                    return m_audioSystem->HandleRegisterRead(address) & 0xFF;
                }
                
                // Log unhandled reads
                m_logger->Warning("System", "Unhandled I/O read at address: " + 
                                 std::to_string(address));
                                 
                // Default for unhandled addresses
                return 0xFF;
            },
            // 16-bit read handler
            [this, graphicsRegionStart, graphicsRegionEnd, inputRegionStart, inputRegionEnd,
             audioRegionStart, audioRegionEnd](uint32_t address) -> uint16_t {
                // Check if address is within valid ranges (ensure it's 16-bit aligned)
                if ((address & 1) != 0) {
                    m_logger->Warning("System", "Unaligned 16-bit I/O read at address: " +
                                     std::to_string(address));
                    return 0xFFFF;
                }
                
                if (address >= graphicsRegionStart && address <= graphicsRegionEnd) {
                    // Graphics system registers
                    return m_graphicsSystem->HandleRegisterRead(address);
                } else if (address >= inputRegionStart && address <= inputRegionEnd) {
                    // Input system registers
                    return static_cast<uint16_t>(m_inputSystem->HandleRegisterRead(address));
                } else if (address >= audioRegionStart && address <= audioRegionEnd) {
                    // Audio system registers (main CPU side)
                    return static_cast<uint16_t>(m_audioSystem->HandleRegisterRead(address));
                }
                
                // Log unhandled reads
                m_logger->Warning("System", "Unhandled 16-bit I/O read at address: " + 
                                 std::to_string(address));
                                 
                // Default for unhandled addresses
                return 0xFFFF;
            }
        );
        
        m_memoryManager->SetWriteHandlers("IO_REGISTERS",
            // 8-bit write handler
            [this, graphicsRegionStart, graphicsRegionEnd, inputRegionStart, inputRegionEnd,
				audioRegionStart, audioRegionEnd](uint32_t address, uint8_t value) {
				   // Check if address is within valid ranges
				   if (address >= graphicsRegionStart && address <= graphicsRegionEnd) {
					   // Graphics system registers
					   m_graphicsSystem->HandleRegisterWrite(address, value);
				   } else if (address >= inputRegionStart && address <= inputRegionEnd) {
					   // Input system registers
					   m_inputSystem->HandleRegisterWrite(address, value);
				   } else if (address >= audioRegionStart && address <= audioRegionEnd) {
					   // Audio system registers (main CPU side)
					   m_audioSystem->HandleRegisterWrite(address, value);
				   } else {
					   // Log unhandled writes
					   m_logger->Warning("System", "Unhandled I/O write at address: " + 
										std::to_string(address) + ", value: " + 
										std::to_string(value));
				   }
			   },
			   // 16-bit write handler
			[this, graphicsRegionStart, graphicsRegionEnd, inputRegionStart, inputRegionEnd,
				audioRegionStart, audioRegionEnd](uint32_t address, uint16_t value) {
				   // Check if address is 16-bit aligned
				   if ((address & 1) != 0) {
					   m_logger->Warning("System", "Unaligned 16-bit I/O write at address: " +
										std::to_string(address) + ", value: " + 
										std::to_string(value));
					   return;
				   }
				   
				   if (address >= graphicsRegionStart && address <= graphicsRegionEnd) {
					   // Graphics system registers
					   m_graphicsSystem->HandleRegisterWrite(address, value);
				   } else if (address >= inputRegionStart && address <= inputRegionEnd) {
					   // Input system registers
					   m_inputSystem->HandleRegisterWrite(address, static_cast<uint8_t>(value));
				   } else if (address >= audioRegionStart && address <= audioRegionEnd) {
					   // Audio system registers (main CPU side)
					   m_audioSystem->HandleRegisterWrite(address, static_cast<uint8_t>(value));
				   } else {
					   // Log unhandled writes
					   m_logger->Warning("System", "Unhandled 16-bit I/O write at address: " + 
										std::to_string(address) + ", value: " + 
										std::to_string(value));
				   }
			   }
		   );
		   
		   m_logger->Info("System", "Memory map setup complete");
	   }
	   catch (const std::exception& e) {
		   m_logger->Error("System", std::string("Memory map setup failed: ") + e.what());
		   throw; // Re-throw to be caught by Initialize()
	   }
   }

void System::SetupInterruptVectorTable() {
	m_logger->Info("System", "Setting up 68000 interrupt vector table");

	try {
		// Get ROM region to place vector table at the beginning (address 0)
		MemoryRegion* romRegion = m_memoryManager->GetRegionByName("ROM");
		if (!romRegion) {
			throw std::runtime_error("ROM region not found");
		}

		// Define vector table addresses
		const uint32_t vectorTableSize = 0x100;  // 256 bytes (64 vectors, 4 bytes each)
		std::vector<uint32_t> vectorTable(vectorTableSize / 4, 0);

		// Initial SSP (Supervisor Stack Pointer) - typically points to top of RAM
		uint32_t mainRamSize = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 
							0x100000 : 0x200000;  // 1MB or 2MB
		uint32_t mainRamBase = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 
							0x200000 : 0x400000;
		vectorTable[0] = mainRamBase + mainRamSize - 4;  // Top of RAM - 4 bytes

		// Initial PC (Program Counter) - typically points to start of code
		vectorTable[1] = 0x000100;  // Start of ROM code after vector table

		// Exception vectors
		vectorTable[2] = 0x000200;  // Bus Error
		vectorTable[3] = 0x000210;  // Address Error
		vectorTable[4] = 0x000220;  // Illegal Instruction
		vectorTable[5] = 0x000230;  // Division by Zero
		vectorTable[6] = 0x000240;  // CHK Instruction
		vectorTable[7] = 0x000250;  // TRAPV Instruction
		vectorTable[8] = 0x000260;  // Privilege Violation
		vectorTable[9] = 0x000270;  // Trace
		vectorTable[10] = 0x000280; // Line 1010 Emulator
		vectorTable[11] = 0x000290; // Line 1111 Emulator
			
		// Reserved vectors (12-23)
		for (int i = 12; i <= 23; i++) {
			vectorTable[i] = 0x0002A0 + ((i - 12) * 0x10); // Align to 16-byte boundaries
		}
			
		// Spurious Interrupt
		vectorTable[24] = 0x0003A0;
			
		// Level 1-7 Autovector Interrupts
		for (int i = 25; i <= 31; i++) {
			vectorTable[i] = 0x0003B0 + ((i - 25) * 0x10); // Align to 16-byte boundaries
		}
			
		// TRAP Instruction Vectors (32-47)
		for (int i = 32; i <= 47; i++) {
			vectorTable[i] = 0x000410 + ((i - 32) * 0x10);
		}
			
		// FPU and MMU vectors (if applicable) (48-63)
		for (int i = 48; i <= 63; i++) {
			vectorTable[i] = 0x000510 + ((i - 48) * 0x10);
		}
			
		// User Interrupt Vectors - these are typically set up by the game ROM
		// but we provide default handlers
		for (int i = 64; i < vectorTableSize / 4; i++) {
			vectorTable[i] = 0x000610 + ((i - 64) * 0x10);
		}
			
		// Set up specific interrupt handlers for the NiXX-32 system
			
		// VBLANK interrupt - used for timing (vector 28, IRQ level 4)
		vectorTable[28] = 0x000068;  // Standard location for VBLANK handler
		
		// Coin/Service interrupt (vector 27, IRQ level 3)
		vectorTable[27] = 0x000300;
		
		// Control inputs interrupt (vector 26, IRQ level 2)
		vectorTable[26] = 0x000310;
		
		// Audio communication interrupt (vector 29, IRQ level 5)
		vectorTable[29] = 0x000320;
		
		// Timer interrupt (vector 30, IRQ level 6)
		vectorTable[30] = 0x000330;
		
		// NMI/System reset (vector 31, IRQ level 7)
		vectorTable[31] = 0x000340;
		
		// Write vector table to ROM memory region
		for (size_t i = 0; i < vectorTable.size(); i++) {
			// Convert to big-endian (68000 is big-endian)
			uint32_t bigEndianValue = 
				((vectorTable[i] & 0xFF) << 24) |
				((vectorTable[i] & 0xFF00) << 8) |
				((vectorTable[i] & 0xFF0000) >> 8) |
				((vectorTable[i] & 0xFF000000) >> 24);
				
			uint32_t addr = i * 4;
			romRegion->data[addr] = (bigEndianValue >> 24) & 0xFF;
			romRegion->data[addr+1] = (bigEndianValue >> 16) & 0xFF;
			romRegion->data[addr+2] = (bigEndianValue >> 8) & 0xFF;
			romRegion->data[addr+3] = bigEndianValue & 0xFF;
		}
			
		// Install default exception handlers
		InstallDefaultExceptionHandlers();
			
		m_logger->Info("System", "Interrupt vector table setup complete");
	}
	catch (const std::exception& e) {
		m_logger->Error("System", std::string("Interrupt vector table setup failed: ") + e.what());
		throw; // Re-throw to be caught by Initialize()
	}
}

void System::InstallDefaultExceptionHandlers() {
	// Create simple handlers for each exception
	// These handlers will log the exception and reset if necessary
		
	// Get ROM region to place handler code
	MemoryRegion* romRegion = m_memoryManager->GetRegionByName("ROM");
	if (!romRegion) {
		throw std::runtime_error("ROM region not found");
	}
		
	// Simple RTE (Return from Exception) instruction - 0x4E73
	const uint16_t RTE_INSTRUCTION = 0x4E73;
		
	// Define handler addresses (must match the vector table)
	struct HandlerInfo {
		uint32_t address;
		const char* name;
		bool shouldReset;
	};
		
	HandlerInfo handlers[] = {
		{ 0x000200, "Bus Error", true },
		{ 0x000210, "Address Error", true },
		{ 0x000220, "Illegal Instruction", true },
		{ 0x000230, "Division by Zero", false },
		{ 0x000240, "CHK Instruction", false },
		{ 0x000250, "TRAPV Instruction", false },
		{ 0x000260, "Privilege Violation", true },
		{ 0x000270, "Trace", false },
		{ 0x000280, "Line 1010 Emulator", true },
		{ 0x000290, "Line 1111 Emulator", true },
		{ 0x0003A0, "Spurious Interrupt", false },
		{ 0x0003B0, "Level 1 Interrupt", false },
		{ 0x0003C0, "Level 2 Interrupt", false },
		{ 0x0003D0, "Level 3 Interrupt", false },
		// 0x000068 is VBLANK - handled separately
		{ 0x0003F0, "Level 5 Interrupt", false },
		{ 0x000400, "Level 6 Interrupt", false },
		{ 0x000410, "Level 7 Interrupt", true }
	};
		
	// Install each handler
	for (const auto& handler : handlers) {
		// Register a CPU hook for this address
		m_mainCPU->RegisterHook(handler.address, [this, handler]() {
			// Log the exception
			m_logger->Warning("CPU", std::string("Exception occurred: ") + handler.name);
			
			// Reset system if this is a critical exception
			if (handler.shouldReset) {
				m_logger->Error("CPU", std::string("Critical exception, resetting system: ") + handler.name);
				this->Reset();
			}
		});
			
		// Write RTE instruction to the handler address
		// if not VBLANK (which has a custom handler)
		if (handler.address != 0x000068) {
			uint32_t addr = handler.address - 0x000000; // Convert to ROM-relative address
			romRegion->data[addr] = (RTE_INSTRUCTION >> 8) & 0xFF;
			romRegion->data[addr+1] = RTE_INSTRUCTION & 0xFF;
		}
	}
		
	// Special case for VBLANK
	// The hook is installed in ConnectSubsystems()
}

void System::UpdatePowerState(float deltaTime) {
    if (!m_powerManagementEnabled) {
        return;
    }

    // Get current time
    auto now = std::chrono::steady_clock::now();
    auto currentTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    // Check for activity (e.g., input events)
    bool hasActivity = false;
    if (m_inputSystem && m_inputSystem->GetActiveChannelCount() > 0) {
        hasActivity = true;
    }

    // Update last activity time if there's activity
    if (hasActivity) {
        m_lastActivityTime = currentTimeMs;
        
        // Wake up if we were in a low power state
        if (m_powerState != PowerState::FULL_POWER) {
            SetPowerState(PowerState::FULL_POWER);
        }
    }
    
    // Calculate idle time
    uint64_t idleTimeMs = currentTimeMs - m_lastActivityTime;
    
    // Transition to appropriate power state based on idle time
    if (m_powerState == PowerState::FULL_POWER && idleTimeMs > m_idleThresholdMs) {
        SetPowerState(PowerState::IDLE);
    } else if (m_powerState == PowerState::IDLE && idleTimeMs > m_sleepThresholdMs) {
        SetPowerState(PowerState::SLEEP);
    }
}

void System::SetPowerState(PowerState newState) {
    if (m_powerState == newState) {
        return;
    }
    
    m_logger->Info("System", "Changing power state from " + 
                   PowerStateToString(m_powerState) + " to " + 
                   PowerStateToString(newState));
    
    m_powerState = newState;
    
    // Apply power state to subsystems
    switch (m_powerState) {
        case PowerState::FULL_POWER:
            // Restore all components to full operation
            SetCPUPowerState(1.0f);
            SetGraphicsPowerState(1.0f);
            SetAudioPowerState(1.0f);
            break;
            
        case PowerState::IDLE:
            // Reduce some component speeds
            SetCPUPowerState(0.5f);    // Run CPUs at half speed
            SetGraphicsPowerState(0.8f); // Slightly reduce graphics processing
            SetAudioPowerState(1.0f);  // Keep audio at full quality
            break;
            
        case PowerState::LOW_POWER:
            // Significantly reduce most components
            SetCPUPowerState(0.25f);   // Run CPUs at quarter speed
            SetGraphicsPowerState(0.5f); // Halve graphics processing
            SetAudioPowerState(0.5f);  // Reduce audio quality
            break;
            
        case PowerState::SLEEP:
            // Minimum power needed to detect wake events
            SetCPUPowerState(0.1f);    // Minimal CPU for wake detection
            SetGraphicsPowerState(0.0f); // Turn off graphics processing
            SetAudioPowerState(0.0f);  // Mute audio
            break;
    }
}

std::string System::PowerStateToString(PowerState state) {
    switch (state) {
        case PowerState::FULL_POWER: return "FULL_POWER";
        case PowerState::IDLE: return "IDLE";
        case PowerState::LOW_POWER: return "LOW_POWER";
        case PowerState::SLEEP: return "SLEEP";
        default: return "UNKNOWN";
    }
}

void System::SetCPUPowerState(float powerFactor) {
    // Apply power scaling to CPU speeds
    if (m_mainCPU) {
        // Scale clock speed by the power factor
        uint32_t normalClockSpeed = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 
                                    16000000 : 20000000;
        uint32_t adjustedClockSpeed = static_cast<uint32_t>(normalClockSpeed * powerFactor);
        m_mainCPU->SetClockSpeed(adjustedClockSpeed);
    }
    
    if (m_audioCPU) {
        // Scale audio CPU clock similarly
        uint32_t normalClockSpeed = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 
                                    8000000 : 10000000;
        uint32_t adjustedClockSpeed = static_cast<uint32_t>(normalClockSpeed * powerFactor);
        m_audioCPU->SetClockSpeed(adjustedClockSpeed);
    }
}

void System::SetGraphicsPowerState(float powerFactor) {
    // Apply power management to graphics system
    if (m_graphicsSystem) {
        if (powerFactor <= 0.0f) {
            // Turn off display (blank screen) but keep minimal state
            m_graphicsSystem->SetDisplayEnabled(false);
        } else {
            // Adjust refresh rate or rendering quality based on power factor
            m_graphicsSystem->SetDisplayEnabled(true);
            m_graphicsSystem->SetPowerSavingMode(powerFactor < 0.9f);
            
            // Could also adjust sprite limit, effect quality, etc.
            if (powerFactor < 0.5f) {
                m_graphicsSystem->SetQualityReduction(true);
            } else {
                m_graphicsSystem->SetQualityReduction(false);
            }
        }
    }
}

void System::SetAudioPowerState(float powerFactor) {
    // Apply power management to audio system
    if (m_audioSystem) {
        if (powerFactor <= 0.0f) {
            // Mute audio completely
            m_audioSystem->SetMasterVolume(0);
            m_audioSystem->SetPaused(true);
        } else {
            // Adjust audio quality based on power factor
            m_audioSystem->SetPaused(false);
            
            // Could reduce sample rate or disable certain effects
            if (powerFactor < 0.7f) {
                m_audioSystem->SetQualityReduction(true);
                m_audioSystem->SetMasterVolume(static_cast<uint8_t>(192 * powerFactor)); // Scale volume
            } else {
                m_audioSystem->SetQualityReduction(false);
                m_audioSystem->SetMasterVolume(192); // Full volume (192 of 255)
            }
        }
    }
}

void System::EnablePowerManagement(bool enable) {
    m_powerManagementEnabled = enable;
    m_logger->Info("System", std::string("Power management ") + 
                   (enable ? "enabled" : "disabled"));
    
    // Reset to full power when changing the setting
    if (enable) {
        // Initialize power management
        auto now = std::chrono::steady_clock::now();
        m_lastActivityTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    }
    SetPowerState(PowerState::FULL_POWER);
}

bool System::IsPowerManagementEnabled() const {
    return m_powerManagementEnabled;
}

PowerState System::GetPowerState() const {
    return m_powerState;
}

void System::SetIdleThreshold(uint64_t milliseconds) {
    m_idleThresholdMs = milliseconds;
    m_logger->Info("System", "Idle threshold set to " + 
                   std::to_string(milliseconds) + " ms");
}

void System::SetSleepThreshold(uint64_t milliseconds) {
    m_sleepThresholdMs = milliseconds;
    m_logger->Info("System", "Sleep threshold set to " + 
                   std::to_string(milliseconds) + " ms");
}

} // namespace NiXX32
