/**
 * NiXX32System.cpp
 * Implementation of the core system architecture for NiXX-32 arcade board emulation
 */

 #include "NiXX32System.h"
 #include "M68000CPU.h"
 #include "Z80CPU.h"
 #include "GraphicsSystem.h"
 #include "AudioSystem.h"
 #include "InputSystem.h"
 #include "ROMLoader.h"
 #include "Config.h"
 #include "Logger.h"
 #include "Debugger.h"
 #include <stdexcept>
 
 namespace NiXX32 {
 
 System::System(HardwareVariant variant, const std::string& configPath) 
	 : m_variant(variant),
	   m_initialized(false),
	   m_paused(false) {
	 
	 // Create logger first so we can log initialization
	 m_logger = std::make_unique<Logger>();
	 m_logger->Info("System", "Initializing NiXX-32 Emulator System");
	 
	 // Create configuration
	 m_config = std::make_unique<Config>(*this, *m_logger, configPath);
	 
	 // Create memory manager
	 m_memoryManager = std::make_unique<MemoryManager>(*this, *m_logger);
	 
	 // Create ROM loader
	 m_romLoader = std::make_unique<ROMLoader>(*this, *m_memoryManager, *m_logger);
	 
	 // Create AudioSystem before Z80CPU because Z80CPU constructor requires AudioSystem reference
	 m_audioSystem = std::make_unique<AudioSystem>(*this, *m_memoryManager, *m_logger);
	 
	 // Log variant
	 if (variant == HardwareVariant::NIXX32_ORIGINAL) {
		 m_logger->Info("System", "Hardware Variant: NiXX-32 Original (1989)");
	 } else {
		 m_logger->Info("System", "Hardware Variant: NiXX-32 Plus (1992)");
	 }
 }
 
 System::~System() {
	 m_logger->Info("System", "Shutting down NiXX-32 Emulator System");
	 
	 // Destroy components in reverse order of initialization/dependencies
	 m_debugger.reset();
	 m_inputSystem.reset();
	 m_graphicsSystem.reset();
	 m_audioCPU.reset();
	 m_mainCPU.reset();
	 m_audioSystem.reset();  // Must be after audioCPU is reset
	 m_memoryManager.reset();
	 m_romLoader.reset();
	 m_config.reset();
	 m_logger.reset();
 }
 
 bool System::Initialize(const std::string& romPath) {
	 if (m_initialized) {
		 m_logger->Warning("System", "System already initialized");
		 return true;
	 }
	 
	 try {
		 // Initialize configuration
		 if (!m_config->Initialize()) {
			 m_logger->Error("System", "Failed to initialize configuration");
			 return false;
		 }
		 
		 // Initialize memory manager
		 if (!m_memoryManager->Initialize(m_variant)) {
			 m_logger->Error("System", "Failed to initialize memory manager");
			 return false;
		 }
		 
		 // Configure system based on hardware variant
		 ConfigureForVariant();
		 
		 // Set up memory mappings
		 SetupMemoryMap();
		 
		 // Initialize AudioSystem
		 AudioHardwareVariant audioVariant = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ?
			 AudioHardwareVariant::NIXX32_ORIGINAL : AudioHardwareVariant::NIXX32_PLUS;
			 
		 if (!m_audioSystem->Initialize(audioVariant)) {
			 m_logger->Error("System", "Failed to initialize Audio System");
			 return false;
		 }
		 
		 // Create CPUs
		 m_mainCPU = std::make_unique<M68000CPU>(*this, *m_memoryManager, *m_logger);
		 m_audioCPU = std::make_unique<Z80CPU>(*this, *m_memoryManager, *m_logger, *m_audioSystem);
		 
		 // Initialize CPUs
		 uint32_t mainCpuClock = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 16000000 : 20000000;
		 uint32_t audioCpuClock = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 8000000 : 10000000;
		 
		 if (!m_mainCPU->Initialize(mainCpuClock)) {
			 m_logger->Error("System", "Failed to initialize Main CPU");
			 return false;
		 }
		 
		 if (!m_audioCPU->Initialize(audioCpuClock)) {
			 m_logger->Error("System", "Failed to initialize Audio CPU");
			 return false;
		 }
		 
		 // Create remaining subsystems
		 GraphicsHardwareVariant gfxVariant = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ?
			 GraphicsHardwareVariant::NIXX32_ORIGINAL : GraphicsHardwareVariant::NIXX32_PLUS;
		 
		 // Create graphics and input systems
		 m_graphicsSystem = std::make_unique<GraphicsSystem>(*this, *m_memoryManager, *m_logger);
		 m_inputSystem = std::make_unique<InputSystem>(*this, *m_memoryManager, *m_logger);
		 
		 // Initialize graphics and input systems
		 if (!m_graphicsSystem->Initialize(gfxVariant)) {
			 m_logger->Error("System", "Failed to initialize Graphics System");
			 return false;
		 }
		 
		 if (!m_inputSystem->Initialize(m_variant)) {
			 m_logger->Error("System", "Failed to initialize Input System");
			 return false;
		 }
		 
		 // Connect subsystems
		 ConnectSubsystems();
		 
		 // Load ROM if specified
		 if (!romPath.empty()) {
			 if (!m_romLoader->Initialize()) {
				 m_logger->Error("System", "Failed to initialize ROM Loader");
				 return false;
			 }
			 
			 if (!m_romLoader->LoadROM(romPath)) {
				 m_logger->Error("System", "Failed to load ROM: " + romPath);
				 return false;
			 }
			 
			 m_logger->Info("System", "ROM loaded successfully: " + romPath);
		 }
		 
		 m_initialized = true;
		 m_logger->Info("System", "NiXX-32 System initialized successfully");
		 return true;
	 }
	 catch (const std::exception& e) {
		 m_logger->Fatal("System", "Exception during initialization: " + std::string(e.what()));
		 return false;
	 }
	 catch (...) {
		 m_logger->Fatal("System", "Unknown exception during initialization");
		 return false;
	 }
 }
 
 void System::RunCycle(float deltaTime) {
	 if (!m_initialized) {
		 m_logger->Error("System", "Cannot run cycle: System not initialized");
		 return;
	 }
	 
	 if (m_paused) {
		 return;
	 }
	 
	 try {
		 // If a debugger is attached, let it handle CPU execution
		 if (m_debugger) {
			 m_debugger->Update();
			 if (m_debugger->IsActive()) {
				 return;
			 }
		 }
		 
		 // Execute CPU cycles
		 // Calculate cycles to execute based on deltaTime and CPU clock speed
		 int mainCpuCycles = static_cast<int>(m_mainCPU->GetClockSpeed() * 1000 * deltaTime);
		 int audioCpuCycles = static_cast<int>(m_audioCPU->GetClockSpeed() * 1000 * deltaTime);
		 
		 // Execute main CPU first
		 m_mainCPU->Execute(mainCpuCycles);
		 
		 // Then execute audio CPU
		 m_audioCPU->Execute(audioCpuCycles);
		 
		 // Update subsystems
		 m_graphicsSystem->Update(deltaTime);
		 m_audioSystem->Update(deltaTime);
		 m_inputSystem->Update(deltaTime);
	 }
	 catch (const std::exception& e) {
		 m_logger->Error("System", "Exception during cycle execution: " + std::string(e.what()));
		 SetPaused(true);
	 }
	 catch (...) {
		 m_logger->Error("System", "Unknown exception during cycle execution");
		 SetPaused(true);
	 }
 }
 
 void System::Reset() {
	 if (!m_initialized) {
		 m_logger->Error("System", "Cannot reset: System not initialized");
		 return;
	 }
	 
	 m_logger->Info("System", "Resetting system");
	 
	 // Reset CPUs
	 m_mainCPU->Reset();
	 m_audioCPU->Reset();
	 
	 // Reset subsystems
	 m_memoryManager->Reset();
	 m_graphicsSystem->Reset();
	 m_audioSystem->Reset();
	 m_inputSystem->Reset();
	 
	 // Reset state
	 m_paused = false;
	 
	 m_logger->Info("System", "System reset complete");
 }
 
 void System::SetPaused(bool paused) {
	 if (m_paused == paused) {
		 return;
	 }
	 
	 m_paused = paused;
	 
	 if (m_paused) {
		 m_logger->Info("System", "System paused");
	 } else {
		 m_logger->Info("System", "System resumed");
	 }
 }
 
 bool System::IsPaused() const {
	 return m_paused;
 }
 
 HardwareVariant System::GetHardwareVariant() const {
	 return m_variant;
 }
 
 MemoryManager& System::GetMemoryManager() {
	 return *m_memoryManager;
 }
 
 M68000CPU& System::GetMainCPU() {
	 return *m_mainCPU;
 }
 
 Z80CPU& System::GetAudioCPU() {
	 return *m_audioCPU;
 }
 
 GraphicsSystem& System::GetGraphicsSystem() {
	 return *m_graphicsSystem;
 }
 
 AudioSystem& System::GetAudioSystem() {
	 return *m_audioSystem;
 }
 
 InputSystem& System::GetInputSystem() {
	 return *m_inputSystem;
 }
 
 void System::AttachDebugger(std::shared_ptr<Debugger> debugger) {
	 m_debugger = debugger;
	 m_logger->Info("System", "Debugger attached to system");
 }
 
 Config& System::GetConfig() {
	 return *m_config;
 }
 
 Logger& System::GetLogger() {
	 return *m_logger;
 }
 
 void System::ConfigureForVariant() {
	 m_logger->Info("System", "Configuring system for hardware variant");
	 
	 // The memory manager already has variant-specific configuration in its Initialize method
	 
	 // Additional variant-specific configuration can be done here
	 // This would include things like setting up interrupts, timers, etc.
	 if (m_variant == HardwareVariant::NIXX32_ORIGINAL) {
		 // Original NiXX-32 hardware configuration
		 m_logger->Info("System", "Configuring Original NiXX-32 hardware (1989)");
		 // Specific configuration for original hardware
	 } else {
		 // Enhanced NiXX-32+ hardware configuration
		 m_logger->Info("System", "Configuring Enhanced NiXX-32+ hardware (1992)");
		 // Specific configuration for enhanced hardware
	 }
 }
 
 void System::ConnectSubsystems() {
	 m_logger->Info("System", "Connecting subsystems");
	 
	 // Connect Audio CPU to Audio System
	 m_audioSystem->SetAudioCPU(m_audioCPU.get());
	 
	 // Set up memory regions for I/O
	 // The header files don't contain direct RegisterReadHandler/RegisterWriteHandler methods
	 // Instead, we'll use the memory regions defined in SetupMemoryMap
	 
	 // For the graphics registers region, set up handlers for reads and writes
	 MemoryRegion* gfxRegion = m_memoryManager->GetRegionByName("GraphicsRegisters");
	 if (gfxRegion) {
		 // Set read/write handlers for graphics memory region
		 // Using methods from the MemoryManager.h header
		 m_memoryManager->SetReadHandlers("GraphicsRegisters",
			 // 8-bit read handler
			 [this](uint32_t address) -> uint8_t {
				 // Graphics system typically doesn't support 8-bit reads
				 return 0;
			 },
			 // 16-bit read handler
			 [this](uint32_t address) -> uint16_t {
				 // Calculate address relative to the start of the region
				 uint32_t relativeAddr = address - 0x800000;
				 // Forward to graphics system (this would call the appropriate method)
				 // In a real implementation, this would use an actual method from GraphicsSystem
				 return 0; // Placeholder
			 }
		 );
 
		 m_memoryManager->SetWriteHandlers("GraphicsRegisters",
			 // 8-bit write handler
			 [this](uint32_t address, uint8_t value) {
				 // Graphics system typically doesn't support 8-bit writes
			 },
			 // 16-bit write handler
			 [this](uint32_t address, uint16_t value) {
				 // Calculate address relative to the start of the region
				 uint32_t relativeAddr = address - 0x800000;
				 // Forward to graphics system (this would call the appropriate method)
				 // In a real implementation, this would use an actual method from GraphicsSystem
			 }
		 );
	 }
	 
	 // Similarly for audio and input systems
	 MemoryRegion* audioRegion = m_memoryManager->GetRegionByName("AudioRegisters");
	 if (audioRegion) {
		 // Set up handlers for audio memory region
		 m_memoryManager->SetReadHandlers("AudioRegisters",
			 // 8-bit read handler for audio
			 [this](uint32_t address) -> uint8_t {
				 // Calculate address relative to the start of the region
				 uint32_t relativeAddr = address - 0x900000;
				 // In a real implementation, this would call a method from AudioSystem
				 return 0; // Placeholder
			 },
			 // Audio typically doesn't need 16-bit read handler
			 nullptr
		 );
 
		 m_memoryManager->SetWriteHandlers("AudioRegisters",
			 // 8-bit write handler for audio
			 [this](uint32_t address, uint8_t value) {
				 // Calculate address relative to the start of the region
				 uint32_t relativeAddr = address - 0x900000;
				 // In a real implementation, this would call a method from AudioSystem
			 },
			 // Audio typically doesn't need 16-bit write handler
			 nullptr
		 );
	 }
	 
	 MemoryRegion* inputRegion = m_memoryManager->GetRegionByName("InputRegisters");
	 if (inputRegion) {
		 // Set up handlers for input memory region
		 m_memoryManager->SetReadHandlers("InputRegisters",
			 // 8-bit read handler for input
			 [this](uint32_t address) -> uint8_t {
				 // Calculate address relative to the start of the region
				 uint32_t relativeAddr = address - 0xA00000;
				 // In a real implementation, this would call a method from InputSystem
				 return 0; // Placeholder
			 },
			 // Input typically doesn't need 16-bit read handler
			 nullptr
		 );
 
		 m_memoryManager->SetWriteHandlers("InputRegisters",
			 // 8-bit write handler for input
			 [this](uint32_t address, uint8_t value) {
				 // Calculate address relative to the start of the region
				 uint32_t relativeAddr = address - 0xA00000;
				 // In a real implementation, this would call a method from InputSystem
			 },
			 // Input typically doesn't need 16-bit write handler
			 nullptr
		 );
	 }
	 
	 // Register Z80 I/O ports for Audio System
	 // Z80CPU.h includes RegisterPortHooks for I/O
	 for (uint8_t port = 0; port < 0xFF; port++) {
		 m_audioCPU->RegisterPortHooks(port,
			 []() -> uint8_t {
				 // Z80 port read
				 return 0; // Default value
			 },
			 [](uint8_t value) {
				 // Z80 port write
				 // No operation by default
			 }
		 );
	 }
	 
	 // Register specific audio ports
	 m_audioCPU->RegisterPortHooks(0x00,
		 []() -> uint8_t {
			 // YM2151 status read
			 return 0; // Default implementation
		 },
		 [](uint8_t value) {
			 // YM2151 address write
			 // Default implementation
		 }
	 );
	 
	 m_audioCPU->RegisterPortHooks(0x01,
		 []() -> uint8_t {
			 // YM2151 data read
			 return 0;
		 },
		 [](uint8_t value) {
			 // YM2151 data write
			 // Default implementation
		 }
	 );
	 
	 // Register port handler for audio subsystem
	 m_audioSystem->RegisterPortHandler(0x00,
		 []() -> uint8_t {
			 // Audio port read
			 return 0;
		 },
		 [](uint8_t value) {
			 // Audio port write
			 // Default implementation
		 }
	 );
	 
	 m_logger->Info("System", "Subsystems connected successfully");
 }
 
 void System::SetupMemoryMap() {
	 m_logger->Info("System", "Setting up memory map");
	 
	 // Main memory regions common to both original and enhanced variants
	 
	 // ROM area (0x000000-0x07FFFF) - Up to 512KB
	 uint32_t romSize = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 0x40000 : 0x80000;
	 m_memoryManager->DefineRegion("ROM", 0x000000, romSize, MemoryAccess::READ_ONLY, MemoryRegionType::ROM);
	 
	 // Main RAM (0x100000-0x1FFFFF) - Up to 1MB
	 uint32_t mainRamSize = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 0x80000 : 0x100000;
	 m_memoryManager->DefineRegion("MainRAM", 0x100000, mainRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::MAIN_RAM);
	 
	 // Video RAM (0x200000-0x2FFFFF) - Up to 1MB
	 uint32_t videoRamSize = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 0x80000 : 0x100000;
	 m_memoryManager->DefineRegion("VideoRAM", 0x200000, videoRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::VIDEO_RAM);
	 
	 // Sound RAM (0x300000-0x30FFFF) - Up to 64KB
	 uint32_t soundRamSize = (m_variant == HardwareVariant::NIXX32_ORIGINAL) ? 0x8000 : 0x10000;
	 m_memoryManager->DefineRegion("SoundRAM", 0x300000, soundRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::SOUND_RAM);
	 
	 // Memory-mapped I/O registers
	 
	 // Graphics System registers (0x800000-0x80FFFF)
	 m_memoryManager->DefineRegion("GraphicsRegisters", 0x800000, 0x10000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Audio System registers (0x900000-0x900FFF)
	 m_memoryManager->DefineRegion("AudioRegisters", 0x900000, 0x1000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Input System registers (0xA00000-0xA000FF)
	 m_memoryManager->DefineRegion("InputRegisters", 0xA00000, 0x100, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Enhanced NiXX-32+ specific regions
	 if (m_variant == HardwareVariant::NIXX32_PLUS) {
		 // Additional expansion RAM if present
		 m_memoryManager->DefineRegion("ExpansionRAM", 0x400000, 0x100000, MemoryAccess::READ_WRITE, MemoryRegionType::EXPANSION);
		 
		 // Expansion I/O if present
		 m_memoryManager->DefineRegion("ExpansionIO", 0xB00000, 0x10000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 }
	 
	 m_logger->Info("System", "Memory map setup complete");
 }
 
 } // namespace NiXX32