/**
 * MemoryManager.cpp
 * Implementation of memory management system for NiXX-32 arcade board emulation
 */

 #include "MemoryManager.h"
 #include "NiXX32System.h"
 #include <stdexcept>
 #include <iostream>
 #include <algorithm>
 #include <cassert>
 
 namespace NiXX32 {
 
 MemoryManager::MemoryManager(System& system, Logger& logger)
	 : m_system(system),
	   m_logger(logger),
	   m_configuredVariant(HardwareVariant::NIXX32_ORIGINAL)
 {
	 m_logger.Info("MemoryManager", "Creating memory manager");
 }
 
 MemoryManager::~MemoryManager() {
	 m_logger.Info("MemoryManager", "Destroying memory manager");
 }
 
 bool MemoryManager::Initialize(HardwareVariant variant) {
	 m_logger.Info("MemoryManager", std::string("Initializing memory manager for variant: ") + 
				  (variant == HardwareVariant::NIXX32_ORIGINAL ? "Original" : "Plus"));
	 
	 try {
		 // Set configured variant
		 m_configuredVariant = variant;
		 
		 // Configure memory map based on hardware variant
		 ConfigureMemoryMap(variant);
		 
		 m_logger.Info("MemoryManager", "Memory manager initialized successfully");
		 return true;
	 }
	 catch (const std::exception& e) {
		 m_logger.Error("MemoryManager", std::string("Failed to initialize memory manager: ") + e.what());
		 return false;
	 }
 }
 
 void MemoryManager::Reset() {
	 m_logger.Info("MemoryManager", "Resetting memory manager");
	 
	 // Reset all RAM regions to zero but leave ROM intact
	 for (auto& region : m_regions) {
		 if (region.type != MemoryRegionType::ROM) {
			 std::fill(region.data.begin(), region.data.end(), 0);
		 }
	 }
 }
 
 uint8_t MemoryManager::Read8(uint32_t address) {
	 // Find the memory region containing this address
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 HandleIllegalAccess(address, false, 8);
		 return 0xFF; // Return 0xFF for unmapped memory (typical behavior in many systems)
	 }
	 
	 auto& region = m_regions[regionIndex];
	 
	 // Check if region is readable
	 if (region.access == MemoryAccess::WRITE_ONLY || region.access == MemoryAccess::NONE) {
		 HandleIllegalAccess(address, false, 8);
		 return 0xFF;
	 }
	 
	 // Calculate address relative to region start
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Check if this region uses custom read handlers
	 if (region.hasHandlers() && region.readHandler8) {
		 return region.readHandler8(address);
	 }
	 
	 // Direct memory access
	 if (relativeAddress < region.data.size()) {
		 return region.data[relativeAddress];
	 }
	 
	 // Should never reach here if regions are properly defined
	 HandleIllegalAccess(address, false, 8);
	 return 0xFF;
 }
 
 uint16_t MemoryManager::Read16(uint32_t address) {
	 // Check for unaligned access
	 if (address & 0x1) {
		 m_logger.Warning("MemoryManager", "Unaligned 16-bit read at 0x" + 
						 std::to_string(address));
		 // Some systems trigger exception on unaligned access, others handle it
		 // For now, just read the individual bytes
		 return (static_cast<uint16_t>(Read8(address)) << 8) | 
				static_cast<uint16_t>(Read8(address + 1));
	 }
	 
	 // Find the memory region containing this address
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 HandleIllegalAccess(address, false, 16);
		 return 0xFFFF;
	 }
	 
	 auto& region = m_regions[regionIndex];
	 
	 // Check if region is readable
	 if (region.access == MemoryAccess::WRITE_ONLY || region.access == MemoryAccess::NONE) {
		 HandleIllegalAccess(address, false, 16);
		 return 0xFFFF;
	 }
	 
	 // Calculate address relative to region start
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Check if this region uses custom read handlers
	 if (region.hasHandlers() && region.readHandler16) {
		 return region.readHandler16(address);
	 }
	 
	 // Check if address+1 is still within the same region (could span region boundary)
	 if (relativeAddress + 1 >= region.size) {
		 // Handle reads that cross region boundaries
		 return (static_cast<uint16_t>(Read8(address)) << 8) | 
				static_cast<uint16_t>(Read8(address + 1));
	 }
	 
	 // Direct memory access (big-endian format for 68000)
	 if (relativeAddress + 1 < region.data.size()) {
		 return (static_cast<uint16_t>(region.data[relativeAddress]) << 8) | 
				static_cast<uint16_t>(region.data[relativeAddress + 1]);
	 }
	 
	 // Should never reach here if regions are properly defined
	 HandleIllegalAccess(address, false, 16);
	 return 0xFFFF;
 }
 
 uint32_t MemoryManager::Read32(uint32_t address) {
	 // Check for unaligned access
	 if (address & 0x1) {
		 m_logger.Warning("MemoryManager", "Unaligned 32-bit read at 0x" + 
						 std::to_string(address));
		 // Read as individual bytes for unaligned access
		 return (static_cast<uint32_t>(Read8(address)) << 24) | 
				(static_cast<uint32_t>(Read8(address + 1)) << 16) | 
				(static_cast<uint32_t>(Read8(address + 2)) << 8) | 
				 static_cast<uint32_t>(Read8(address + 3));
	 }
	 
	 // For simplicity and to ensure correct behavior at region boundaries,
	 // implement 32-bit read as two 16-bit reads
	 uint16_t high = Read16(address);
	 uint16_t low = Read16(address + 2);
	 
	 return (static_cast<uint32_t>(high) << 16) | static_cast<uint32_t>(low);
 }
 
 void MemoryManager::Write8(uint32_t address, uint8_t value) {
	 // Find the memory region containing this address
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 HandleIllegalAccess(address, true, 8);
		 return;
	 }
	 
	 auto& region = m_regions[regionIndex];
	 
	 // Check if region is writable
	 if (region.access == MemoryAccess::READ_ONLY || region.access == MemoryAccess::NONE) {
		 HandleIllegalAccess(address, true, 8);
		 return;
	 }
	 
	 // Calculate address relative to region start
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Check if this region uses custom write handlers
	 if (region.hasHandlers() && region.writeHandler8) {
		 region.writeHandler8(address, value);
		 return;
	 }
	 
	 // Direct memory access
	 if (relativeAddress < region.data.size()) {
		 region.data[relativeAddress] = value;
		 return;
	 }
	 
	 // Should never reach here if regions are properly defined
	 HandleIllegalAccess(address, true, 8);
 }
 
 void MemoryManager::Write16(uint32_t address, uint16_t value) {
	 // Check for unaligned access
	 if (address & 0x1) {
		 m_logger.Warning("MemoryManager", "Unaligned 16-bit write at 0x" + 
						 std::to_string(address));
		 // Some systems trigger exception on unaligned access, others handle it
		 // For now, just write the individual bytes
		 Write8(address, (value >> 8) & 0xFF);
		 Write8(address + 1, value & 0xFF);
		 return;
	 }
	 
	 // Find the memory region containing this address
	 int regionIndex = FindRegionIndex(address);
	 
	 if (regionIndex < 0) {
		 HandleIllegalAccess(address, true, 16);
		 return;
	 }
	 
	 auto& region = m_regions[regionIndex];
	 
	 // Check if region is writable
	 if (region.access == MemoryAccess::READ_ONLY || region.access == MemoryAccess::NONE) {
		 HandleIllegalAccess(address, true, 16);
		 return;
	 }
	 
	 // Calculate address relative to region start
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Check if this region uses custom write handlers
	 if (region.hasHandlers() && region.writeHandler16) {
		 region.writeHandler16(address, value);
		 return;
	 }
	 
	 // Check if address+1 is still within the same region (could span region boundary)
	 if (relativeAddress + 1 >= region.size) {
		 // Handle writes that cross region boundaries
		 Write8(address, (value >> 8) & 0xFF);
		 Write8(address + 1, value & 0xFF);
		 return;
	 }
	 
	 // Direct memory access (big-endian format for 68000)
	 if (relativeAddress + 1 < region.data.size()) {
		 region.data[relativeAddress] = (value >> 8) & 0xFF;
		 region.data[relativeAddress + 1] = value & 0xFF;
		 return;
	 }
	 
	 // Should never reach here if regions are properly defined
	 HandleIllegalAccess(address, true, 16);
 }
 
 void MemoryManager::Write32(uint32_t address, uint32_t value) {
	 // Check for unaligned access
	 if (address & 0x1) {
		 m_logger.Warning("MemoryManager", "Unaligned 32-bit write at 0x" + 
						 std::to_string(address));
		 // Write as individual bytes for unaligned access
		 Write8(address, (value >> 24) & 0xFF);
		 Write8(address + 1, (value >> 16) & 0xFF);
		 Write8(address + 2, (value >> 8) & 0xFF);
		 Write8(address + 3, value & 0xFF);
		 return;
	 }
	 
	 // For simplicity and to ensure correct behavior at region boundaries,
	 // implement 32-bit write as two 16-bit writes
	 Write16(address, (value >> 16) & 0xFFFF);
	 Write16(address + 2, value & 0xFFFF);
 }
 
 bool MemoryManager::LoadROM(const std::vector<uint8_t>& romData, uint32_t baseAddress) {
	 // Find the memory region at the specified base address
	 int regionIndex = FindRegionIndex(baseAddress);
	 
	 if (regionIndex < 0) {
		 m_logger.Error("MemoryManager", "Failed to load ROM: No memory region at address 0x" +
					   std::to_string(baseAddress));
		 return false;
	 }
	 
	 auto& region = m_regions[regionIndex];
	 
	 if (region.type != MemoryRegionType::ROM) {
		 m_logger.Warning("MemoryManager", "Loading ROM data into non-ROM region at address 0x" +
						 std::to_string(baseAddress));
	 }
	 
	 // Calculate relative address within the region
	 uint32_t relativeAddress = GetRegionRelativeAddress(baseAddress, regionIndex);
	 
	 // Check if ROM data fits in the region
	 if (relativeAddress + romData.size() > region.size) {
		 m_logger.Error("MemoryManager", "ROM data size exceeds region size");
		 return false;
	 }
	 
	 // Copy ROM data to region
	 std::copy(romData.begin(), romData.end(), region.data.begin() + relativeAddress);
	 
	 m_logger.Info("MemoryManager", "ROM data loaded successfully: " + 
				  std::to_string(romData.size()) + " bytes at 0x" + 
				  std::to_string(baseAddress));
	 
	 return true;
 }
 
 MemoryRegion* MemoryManager::DefineRegion(const std::string& name, uint32_t startAddress, 
											uint32_t size, MemoryAccess access, 
											MemoryRegionType type) {
	 m_logger.Info("MemoryManager", "Defining memory region: " + name + " at 0x" + 
				  std::to_string(startAddress) + ", size: 0x" + std::to_string(size));
	 
	 // Check for overlapping regions
	 for (const auto& region : m_regions) {
		 // Check if regions overlap
		 bool overlaps = !(startAddress + size <= region.startAddress || 
						   startAddress >= region.startAddress + region.size);
		 
		 if (overlaps) {
			 m_logger.Error("MemoryManager", "Region overlaps with existing region: " + 
						   region.name);
			 return nullptr;
		 }
	 }
	 
	 // Create the new region
	 MemoryRegion region;
	 region.name = name;
	 region.startAddress = startAddress;
	 region.size = size;
	 region.access = access;
	 region.type = type;
	 
	 // Allocate memory for the region
	 region.data.resize(size, 0);
	 
	 // Add the region to our list
	 m_regions.push_back(region);
	 
	 // Add to name lookup map
	 size_t index = m_regions.size() - 1;
	 m_regionsByName[name] = index;
	 
	 return &m_regions[index];
 }
 
 MemoryRegion* MemoryManager::GetRegionByAddress(uint32_t address) {
	 int index = FindRegionIndex(address);
	 if (index < 0) {
		 return nullptr;
	 }
	 
	 return &m_regions[index];
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
	 auto it = m_regionsByName.find(regionName);
	 if (it == m_regionsByName.end()) {
		 m_logger.Error("MemoryManager", "Failed to set read handlers: Region not found: " + 
					   regionName);
		 return false;
	 }
	 
	 auto& region = m_regions[it->second];
	 region.readHandler8 = handler8;
	 region.readHandler16 = handler16;
	 
	 m_logger.Info("MemoryManager", "Read handlers set for region: " + regionName);
	 return true;
 }
 
 bool MemoryManager::SetWriteHandlers(const std::string& regionName,
									   std::function<void(uint32_t, uint8_t)> handler8,
									   std::function<void(uint32_t, uint16_t)> handler16) {
	 auto it = m_regionsByName.find(regionName);
	 if (it == m_regionsByName.end()) {
		 m_logger.Error("MemoryManager", "Failed to set write handlers: Region not found: " + 
					   regionName);
		 return false;
	 }
	 
	 auto& region = m_regions[it->second];
	 region.writeHandler8 = handler8;
	 region.writeHandler16 = handler16;
	 
	 m_logger.Info("MemoryManager", "Write handlers set for region: " + regionName);
	 return true;
 }
 
 uint8_t* MemoryManager::GetDirectPointer(uint32_t address, uint32_t size) {
	 int regionIndex = FindRegionIndex(address);
	 if (regionIndex < 0) {
		 return nullptr;
	 }
	 
	 auto& region = m_regions[regionIndex];
	 uint32_t relativeAddress = GetRegionRelativeAddress(address, regionIndex);
	 
	 // Check if the requested size fits within the region
	 if (relativeAddress + size > region.size) {
		 return nullptr;
	 }
	 
	 // Return pointer to the memory location
	 return &region.data[relativeAddress];
 }
 
 void MemoryManager::ConfigureMemoryMap(HardwareVariant variant) {
	 // Clear any existing regions
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
	 m_logger.Info("MemoryManager", "Configuring memory map for original NiXX-32 hardware");
	 
	 // Set memory sizes for original hardware
	 m_mainRamSize = 0x100000;   // 1MB
	 m_videoRamSize = 0x080000;  // 512KB
	 m_soundRamSize = 0x020000;  // 128KB
	 m_maxRomSize = 0x200000;    // 2MB
	 
	 // Define memory regions for original hardware
	 
	 // ROM (0x000000 - 0x1FFFFF)
	 DefineRegion("ROM", 0x000000, m_maxRomSize, MemoryAccess::READ_ONLY, MemoryRegionType::ROM);
	 
	 // Main RAM (0x200000 - 0x2FFFFF)
	 DefineRegion("MAIN_RAM", 0x200000, m_mainRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::MAIN_RAM);
	 
	 // Video RAM (0x300000 - 0x37FFFF)
	 DefineRegion("VIDEO_RAM", 0x300000, m_videoRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::VIDEO_RAM);
	 
	 // IO Registers (0x400000 - 0x40FFFF)
	 DefineRegion("IO_REGISTERS", 0x400000, 0x010000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Sound RAM (0x410000 - 0x42FFFF)
	 DefineRegion("SOUND_RAM", 0x410000, m_soundRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::SOUND_RAM);
 }
 
 void MemoryManager::ConfigurePlusHardware() {
	 m_logger.Info("MemoryManager", "Configuring memory map for enhanced NiXX-32+ hardware");
	 
	 // Set memory sizes for enhanced hardware
	 m_mainRamSize = 0x200000;   // 2MB
	 m_videoRamSize = 0x100000;  // 1MB
	 m_soundRamSize = 0x040000;  // 256KB
	 m_maxRomSize = 0x400000;    // 4MB
	 
	 // Define memory regions for enhanced hardware
	 
	 // ROM (0x000000 - 0x3FFFFF)
	 DefineRegion("ROM", 0x000000, m_maxRomSize, MemoryAccess::READ_ONLY, MemoryRegionType::ROM);
	 
	 // Main RAM (0x400000 - 0x5FFFFF)
	 DefineRegion("MAIN_RAM", 0x400000, m_mainRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::MAIN_RAM);
	 
	 // Video RAM (0x600000 - 0x6FFFFF)
	 DefineRegion("VIDEO_RAM", 0x600000, m_videoRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::VIDEO_RAM);
	 
	 // IO Registers (0x700000 - 0x70FFFF)
	 DefineRegion("IO_REGISTERS", 0x700000, 0x010000, MemoryAccess::READ_WRITE, MemoryRegionType::IO_REGISTERS);
	 
	 // Sound RAM (0x710000 - 0x74FFFF)
	 DefineRegion("SOUND_RAM", 0x710000, m_soundRamSize, MemoryAccess::READ_WRITE, MemoryRegionType::SOUND_RAM);
 }
 
 int MemoryManager::FindRegionIndex(uint32_t address) {
	 for (size_t i = 0; i < m_regions.size(); i++) {
		 if (address >= m_regions[i].startAddress && 
			 address < m_regions[i].startAddress + m_regions[i].size) {
			 return static_cast<int>(i);
		 }
	 }
	 
	 return -1;
 }
 
 uint32_t MemoryManager::GetRegionRelativeAddress(uint32_t address, int regionIndex) {
	 if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) {
		 return 0;
	 }
	 
	 return address - m_regions[regionIndex].startAddress;
 }
 
 void MemoryManager::HandleIllegalAccess(uint32_t address, bool isWrite, int size) {
	 std::string operation = isWrite ? "write" : "read";
	 std::string sizeStr = std::to_string(size) + "-bit";
	 
	 m_logger.Warning("MemoryManager", "Illegal memory " + operation + " (" + sizeStr + ") at 0x" + 
					 std::to_string(address));
	 
	 // For a full system, we might want to trigger a bus error exception in the CPU
	 // This would depend on how the System and CPU classes are implemented
	 // For now, just log the error
	 
	 // The 68000 would raise a bus error or address error exception
	 if (m_system.GetHardwareVariant() == HardwareVariant::NIXX32_ORIGINAL ||
		 m_system.GetHardwareVariant() == HardwareVariant::NIXX32_PLUS) {
		 
		 // Try to trigger an exception in the CPU
		 try {
			 // This assumes there's a method in the System class to handle this
			 // m_system.TriggerAddressException(address, isWrite, size);
			 
			 // If the above doesn't exist, we could directly access the CPU
			 // m_system.GetMainCPU().TriggerException(ExceptionType::BUS_ERROR);
		 }
		 catch (const std::exception& e) {
			 m_logger.Error("MemoryManager", std::string("Failed to trigger CPU exception: ") + e.what());
		 }
	 }
 }
 
 } // namespace NiXX32