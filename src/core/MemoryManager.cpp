/**
 * MemoryManager.cpp
 * Implementation of the memory management system for NiXX-32 arcade board emulation
 */

 #include "MemoryManager.h"
 #include "NiXX32System.h"
 #include <algorithm>
 #include <stdexcept>
 #include <sstream>
 
 namespace NiXX32 {
 
 MemoryManager::MemoryManager(System& system, Logger& logger)
	 : m_system(system),
	   m_logger(logger),
	   m_configuredVariant(HardwareVariant::NIXX32_ORIGINAL),
	   m_mainRamSize(0),
	   m_videoRamSize(0),
	   m_soundRamSize(0),
	   m_maxRomSize(0) {
	 
	 m_logger.Info("MemoryManager", "Initializing Memory Manager");
 }
 
 MemoryManager::~MemoryManager() {
	 m_logger.Info("MemoryManager", "Shutting down Memory Manager");
 }
 
 bool MemoryManager::Initialize(HardwareVariant variant) {
	 m_logger.Info("MemoryManager", "Initializing memory system");
	 
	 // Store the variant we're configuring for
	 m_configuredVariant = variant;
	 
	 // Configure memory map for the specified hardware variant
	 ConfigureMemoryMap(variant);
	 
	 m_logger.Info("MemoryManager", "Memory system initialized successfully");
	 return true;
 }
 
 void MemoryManager::Reset() {
	 m_logger.Info("MemoryManager", "Resetting memory system");
	 
	 // Reset all RAM regions to zero
	 for (auto& region : m_regions) {
		 if (region.type != MemoryRegionType::ROM) {
			 // Only clear RAM, not ROM
			 std::fill(region.data.begin(), region.data.end(), 0);
		 }
	 }
	 
	 m_logger.Info("MemoryManager", "Memory system reset complete");
 }
 
 uint8_t MemoryManager::Read8(uint32_t address) {
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 HandleIllegalAccess(address, false, 8);
		 return 0xFF; // Return 0xFF for unmapped memory
	 }
	 
	 MemoryRegion& region = m_regions[regionIndex];
	 
	 // Check if region is readable
	 if (region.access == MemoryAccess::WRITE_ONLY || region.access == MemoryAccess::NONE) {
		 HandleIllegalAccess(address, false, 8);
		 return 0xFF;
	 }
	 
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Check if we have a custom read handler for this region
	 if (region.hasHandlers() && region.readHandler8) {
		 return region.readHandler8(relativeAddress);
	 }
	 
	 // Bounds check
	 if (relativeAddress >= region.size) {
		 HandleIllegalAccess(address, false, 8);
		 return 0xFF;
	 }
	 
	 // Return data from memory
	 return region.data[relativeAddress];
 }
 
 uint16_t MemoryManager::Read16(uint32_t address) {
	 // 68000 requires 16-bit values to be aligned on even addresses
	 if (address & 1) {
		 m_logger.Warning("MemoryManager", "Unaligned 16-bit read at " + std::to_string(address));
		 // Some systems would throw an address error exception here
		 // For now, we'll handle it by reading from the aligned address
		 address &= ~1;
	 }
	 
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 HandleIllegalAccess(address, false, 16);
		 return 0xFFFF; // Return 0xFFFF for unmapped memory
	 }
	 
	 MemoryRegion& region = m_regions[regionIndex];
	 
	 // Check if region is readable
	 if (region.access == MemoryAccess::WRITE_ONLY || region.access == MemoryAccess::NONE) {
		 HandleIllegalAccess(address, false, 16);
		 return 0xFFFF;
	 }
	 
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Check if we have a custom read handler for this region
	 if (region.hasHandlers() && region.readHandler16) {
		 return region.readHandler16(relativeAddress);
	 }
	 
	 // Bounds check
	 if (relativeAddress + 1 >= region.size) {
		 HandleIllegalAccess(address, false, 16);
		 return 0xFFFF;
	 }
	 
	 // Return data from memory (68000 is big-endian)
	 return (region.data[relativeAddress] << 8) | region.data[relativeAddress + 1];
 }
 
 uint32_t MemoryManager::Read32(uint32_t address) {
	 // 68000 requires 32-bit values to be aligned on even addresses
	 if (address & 1) {
		 m_logger.Warning("MemoryManager", "Unaligned 32-bit read at " + std::to_string(address));
		 // Some systems would throw an address error exception here
		 // For now, we'll handle it by reading from the aligned address
		 address &= ~1;
	 }
	 
	 // Read two 16-bit values and combine them
	 uint32_t high = Read16(address);
	 uint32_t low = Read16(address + 2);
	 
	 return (high << 16) | low;
 }
 
 void MemoryManager::Write8(uint32_t address, uint8_t value) {
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 HandleIllegalAccess(address, true, 8);
		 return;
	 }
	 
	 MemoryRegion& region = m_regions[regionIndex];
	 
	 // Check if region is writable
	 if (region.access == MemoryAccess::READ_ONLY || region.access == MemoryAccess::NONE) {
		 HandleIllegalAccess(address, true, 8);
		 return;
	 }
	 
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Check if we have a custom write handler for this region
	 if (region.hasHandlers() && region.writeHandler8) {
		 region.writeHandler8(relativeAddress, value);
		 return;
	 }
	 
	 // Bounds check
	 if (relativeAddress >= region.size) {
		 HandleIllegalAccess(address, true, 8);
		 return;
	 }
	 
	 // Write data to memory
	 region.data[relativeAddress] = value;
 }
 
 void MemoryManager::Write16(uint32_t address, uint16_t value) {
	 // 68000 requires 16-bit values to be aligned on even addresses
	 if (address & 1) {
		 m_logger.Warning("MemoryManager", "Unaligned 16-bit write at " + std::to_string(address));
		 // Some systems would throw an address error exception here
		 // For now, we'll handle it by writing to the aligned address
		 address &= ~1;
	 }
	 
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 HandleIllegalAccess(address, true, 16);
		 return;
	 }
	 
	 MemoryRegion& region = m_regions[regionIndex];
	 
	 // Check if region is writable
	 if (region.access == MemoryAccess::READ_ONLY || region.access == MemoryAccess::NONE) {
		 HandleIllegalAccess(address, true, 16);
		 return;
	 }
	 
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Check if we have a custom write handler for this region
	 if (region.hasHandlers() && region.writeHandler16) {
		 region.writeHandler16(relativeAddress, value);
		 return;
	 }
	 
	 // Bounds check
	 if (relativeAddress + 1 >= region.size) {
		 HandleIllegalAccess(address, true, 16);
		 return;
	 }
	 
	 // Write data to memory (68000 is big-endian)
	 region.data[relativeAddress] = (value >> 8) & 0xFF;
	 region.data[relativeAddress + 1] = value & 0xFF;
 }
 
 void MemoryManager::Write32(uint32_t address, uint32_t value) {
	 // 68000 requires 32-bit values to be aligned on even addresses
	 if (address & 1) {
		 m_logger.Warning("MemoryManager", "Unaligned 32-bit write at " + std::to_string(address));
		 // Some systems would throw an address error exception here
		 // For now, we'll handle it by writing to the aligned address
		 address &= ~1;
	 }
	 
	 // Split into two 16-bit writes
	 Write16(address, (value >> 16) & 0xFFFF);
	 Write16(address + 2, value & 0xFFFF);
 }
 
 bool MemoryManager::LoadROM(const std::vector<uint8_t>& romData, uint32_t baseAddress) {
	 m_logger.Info("MemoryManager", "Loading ROM data to address " + std::to_string(baseAddress));
	 
	 int regionIndex = FindRegionIndex(baseAddress);
	 
	 if (regionIndex < 0) {
		 m_logger.Error("MemoryManager", "Invalid ROM base address: " + std::to_string(baseAddress));
		 return false;
	 }
	 
	 MemoryRegion& region = m_regions[regionIndex];
	 
	 // Verify this is a ROM region
	 if (region.type != MemoryRegionType::ROM) {
		 m_logger.Error("MemoryManager", "Cannot load ROM into non-ROM region at " + std::to_string(baseAddress));
		 return false;
	 }
	 
	 uint32_t relativeAddress = GetRegionRelativeAddress(baseAddress, regionIndex);
	 
	 // Check if ROM fits in the region
	 if (relativeAddress + romData.size() > region.size) {
		 m_logger.Error("MemoryManager", "ROM data too large for region at " + std::to_string(baseAddress) + 
						". Size: " + std::to_string(romData.size()) + 
						", Available: " + std::to_string(region.size - relativeAddress));
		 return false;
	 }
	 
	 // Copy ROM data to memory
	 std::copy(romData.begin(), romData.end(), region.data.begin() + relativeAddress);
	 
	 m_logger.Info("MemoryManager", "ROM data loaded successfully. Size: " + std::to_string(romData.size()) + " bytes");
	 return true;
 }
 
 MemoryRegion* MemoryManager::DefineRegion(const std::string& name, uint32_t startAddress, 
											uint32_t size, MemoryAccess access, 
											MemoryRegionType type) {
	 m_logger.Info("MemoryManager", "Defining memory region: " + name + 
				   " at " + std::to_string(startAddress) + 
				   ", size: " + std::to_string(size) + " bytes");
	 
	 // Check for overlapping regions
	 for (const auto& region : m_regions) {
		 // Check if this new region overlaps with an existing one
		 if ((startAddress >= region.startAddress && 
			  startAddress < region.startAddress + region.size) ||
			 (startAddress + size > region.startAddress && 
			  startAddress + size <= region.startAddress + region.size) ||
			 (startAddress <= region.startAddress && 
			  startAddress + size >= region.startAddress + region.size)) {
				 
			 m_logger.Warning("MemoryManager", "New region " + name + " overlaps with existing region " + region.name);
		 }
	 }
	 
	 // Create the new region
	 MemoryRegion newRegion;
	 newRegion.name = name;
	 newRegion.startAddress = startAddress;
	 newRegion.size = size;
	 newRegion.access = access;
	 newRegion.type = type;
	 
	 // Allocate memory for the region
	 newRegion.data.resize(size, 0);
	 
	 // Add to regions list
	 m_regions.push_back(newRegion);
	 
	 // Add to lookup map
	 m_regionsByName[name] = m_regions.size() - 1;
	 
	 return &m_regions.back();
 }
 
 MemoryRegion* MemoryManager::GetRegionByAddress(uint32_t address) {
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 return nullptr;
	 }
	 
	 return &m_regions[regionIndex];
 }
 
 MemoryRegion* MemoryManager::GetRegionByName(const std::string& name) {
	 auto it = m_regionsByName.find(name);
	 
	 if (it == m_regionsByName.end()) {
		 return nullptr;
	 }
	 
	 return &m_regions[it->second];
 }
 
 bool MemoryManager::SetReadHandlers(const std::string& regionName,
									 std::function<uint8_t(uint32_t)> handler8,
									 std::function<uint16_t(uint32_t)> handler16) {
	 auto region = GetRegionByName(regionName);
	 
	 if (!region) {
		 m_logger.Error("MemoryManager", "Cannot set read handlers for unknown region: " + regionName);
		 return false;
	 }
	 
	 region->readHandler8 = handler8;
	 region->readHandler16 = handler16;
	 
	 m_logger.Info("MemoryManager", "Read handlers set for region: " + regionName);
	 return true;
 }
 
 bool MemoryManager::SetWriteHandlers(const std::string& regionName,
									  std::function<void(uint32_t, uint8_t)> handler8,
									  std::function<void(uint32_t, uint16_t)> handler16) {
	 auto region = GetRegionByName(regionName);
	 
	 if (!region) {
		 m_logger.Error("MemoryManager", "Cannot set write handlers for unknown region: " + regionName);
		 return false;
	 }
	 
	 region->writeHandler8 = handler8;
	 region->writeHandler16 = handler16;
	 
	 m_logger.Info("MemoryManager", "Write handlers set for region: " + regionName);
	 return true;
 }
 
 uint8_t* MemoryManager::GetDirectPointer(uint32_t address, uint32_t size) {
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 return nullptr;
	 }
	 
	 MemoryRegion& region = m_regions[regionIndex];
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Bounds check
	 if (relativeAddress + size > region.size) {
		 return nullptr;
	 }
	 
	 // Return pointer to data
	 return &region.data[relativeAddress];
 }
 
 void MemoryManager::ConfigureMemoryMap(HardwareVariant variant) {
	 m_logger.Info("MemoryManager", "Configuring memory map for hardware variant");
	 
	 // Reset existing regions
	 m_regions.clear();
	 m_regionsByName.clear();
	 
	 // Set memory limits based on variant
	 if (variant == HardwareVariant::NIXX32_ORIGINAL) {
		 ConfigureOriginalHardware();
	 } else {
		 ConfigurePlusHardware();
	 }
 }
 
 void MemoryManager::ConfigureOriginalHardware() {
	 m_logger.Info("MemoryManager", "Configuring memory for original NiXX-32 hardware (1989)");
	 
	 // Set memory limits for original hardware
	 m_mainRamSize = 1 * 1024 * 1024;  // 1MB
	 m_videoRamSize = 512 * 1024;     // 512KB
	 m_soundRamSize = 128 * 1024;     // 128KB
	 m_maxRomSize = 2 * 1024 * 1024;  // 2MB
	 
	 // Define memory regions - these are the same as in NiXX32System.cpp but
	 // we're implementing them here for completeness
	 
	 // ROM area (0x000000-0x1FFFFF) - Up to 2MB
	 DefineRegion("ROM", 0x000000, m_maxRomSize, MemoryAccess::READ_ONLY, MemoryRegionType::ROM);
	 
	 // Main RAM (0x200000-0x2FFFFF) - 1MB
	 DefineRegion("MainRAM", 0x200000, m_mainRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::MAIN_RAM);
	 
	 // Video RAM (0x300000-0x37FFFF) - 512KB
	 DefineRegion("VideoRAM", 0x300000, m_videoRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::VIDEO_RAM);
	 
	 // Sound RAM (0x380000-0x39FFFF) - 128KB
	 DefineRegion("SoundRAM", 0x380000, m_soundRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::SOUND_RAM);
	 
	 // Memory-mapped I/O regions
	 
	 // Graphics System registers (0x800000-0x80FFFF)
	 DefineRegion("GraphicsRegisters", 0x800000, 0x10000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Audio System registers (0x900000-0x900FFF)
	 DefineRegion("AudioRegisters", 0x900000, 0x1000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Input System registers (0xA00000-0xA000FF)
	 DefineRegion("InputRegisters", 0xA00000, 0x100, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
 }
 
 void MemoryManager::ConfigurePlusHardware() {
	 m_logger.Info("MemoryManager", "Configuring memory for enhanced NiXX-32+ hardware (1992)");
	 
	 // Set memory limits for enhanced hardware
	 m_mainRamSize = 2 * 1024 * 1024;  // 2MB
	 m_videoRamSize = 1 * 1024 * 1024; // 1MB
	 m_soundRamSize = 256 * 1024;      // 256KB
	 m_maxRomSize = 4 * 1024 * 1024;   // 4MB
	 
	 // Define memory regions - these are the same as in NiXX32System.cpp but
	 // we're implementing them here for completeness
	 
	 // ROM area (0x000000-0x3FFFFF) - Up to 4MB
	 DefineRegion("ROM", 0x000000, m_maxRomSize, MemoryAccess::READ_ONLY, MemoryRegionType::ROM);
	 
	 // Main RAM (0x400000-0x5FFFFF) - 2MB
	 DefineRegion("MainRAM", 0x400000, m_mainRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::MAIN_RAM);
	 
	 // Video RAM (0x600000-0x6FFFFF) - 1MB
	 DefineRegion("VideoRAM", 0x600000, m_videoRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::VIDEO_RAM);
	 
	 // Sound RAM (0x700000-0x73FFFF) - 256KB
	 DefineRegion("SoundRAM", 0x700000, m_soundRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::SOUND_RAM);
	 
	 // Memory-mapped I/O regions
	 
	 // Graphics System registers (0x800000-0x80FFFF)
	 DefineRegion("GraphicsRegisters", 0x800000, 0x10000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Audio System registers (0x900000-0x900FFF)
	 DefineRegion("AudioRegisters", 0x900000, 0x1000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Input System registers (0xA00000-0xA000FF)
	 DefineRegion("InputRegisters", 0xA00000, 0x100, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Enhanced NiXX-32+ specific regions
	 
	 // Additional expansion RAM
	 DefineRegion("ExpansionRAM", 0xB00000, 0x100000, MemoryAccess::READ_WRITE, MemoryRegionType::EXPANSION);
	 
	 // Expansion I/O
	 DefineRegion("ExpansionIO", 0xC00000, 0x10000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
 }
 
 int MemoryManager::FindRegionIndex(uint32_t address) {
	 for (size_t i = 0; i < m_regions.size(); i++) {
		 const MemoryRegion& region = m_regions[i];
		 
		 if (address >= region.startAddress && 
			 address < region.startAddress + region.size) {
			 return static_cast<int>(i);
		 }
	 }
	 
	 return -1; // Region not found
 }
 
 uint32_t MemoryManager::GetRegionRelativeAddress(uint32_t address, int regionIndex) {
	 if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) {
		 return 0;
	 }
	 
	 return address - m_regions[regionIndex].startAddress;
 }
 
 void MemoryManager::HandleIllegalAccess(uint32_t address, bool isWrite, int size) {
	 std::stringstream ss;
	 ss << "Illegal memory " << (isWrite ? "write" : "read") << " of " 
		<< size << " bits at address 0x" << std::hex << address;
	 
	 m_logger.Warning("MemoryManager", ss.str());
	 
	 // In a real system, this might trigger a bus error or other exception
	 // For now, we just log the error
 }
 
 } // namespace NiXX32