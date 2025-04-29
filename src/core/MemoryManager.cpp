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
 #include <thread>
 #include <chrono>
 #include <mutex>
 #include <shared_mutex>
 #include <atomic>
 #include <random>
 #include <cstring>
 
 namespace NiXX32 {
 
 // Memory test patterns
 const uint8_t TEST_PATTERN_WALKING_ONES[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
 const uint8_t TEST_PATTERN_WALKING_ZEROS[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};
 const uint8_t TEST_PATTERN_CHECKERBOARD[] = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};
 const uint8_t TEST_PATTERN_RANDOM_SEED = 0x42;
 
 MemoryManager::MemoryManager(System& system, Logger& logger)
	 : m_system(system),
	   m_logger(logger),
	   m_configuredVariant(HardwareVariant::NIXX32_ORIGINAL),
	   m_refreshTimerMs(15),  // 15ms refresh interval (typical for DRAM)
	   m_lastRefreshTime(0),
	   m_refreshPending(false),
	   m_refreshRowCounter(0),
	   m_autoRefreshEnabled(true),
	   m_selfTestOnBoot(true),
	   m_memoryCorrupted(false),
	   m_refreshInProgress(false)
 {
	 m_logger.Info("MemoryManager", "Creating memory manager");
	 
	 // Initialize refresh timer
	 m_lastRefreshTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		 std::chrono::steady_clock::now().time_since_epoch()).count();
 }
 
 MemoryManager::~MemoryManager() {
	 m_logger.Info("MemoryManager", "Destroying memory manager");
	 
	 // If refresh thread is running, stop it
	 StopRefreshThread();
 }
 
 bool MemoryManager::Initialize(HardwareVariant variant) {
	 m_logger.Info("MemoryManager", std::string("Initializing memory manager for variant: ") + 
				  (variant == HardwareVariant::NIXX32_ORIGINAL ? "Original" : "Plus"));
	 
	 try {
		 // Set configured variant
		 m_configuredVariant = variant;
		 
		 // Configure memory map based on hardware variant
		 ConfigureMemoryMap(variant);
		 
		 // Configure region-specific properties
		 ConfigureRegionProperties();
		 
		 // Run memory self-test if enabled
		 if (m_selfTestOnBoot && !RunMemorySelfTest()) {
			 m_logger.Error("MemoryManager", "Memory self-test failed");
			 return false;
		 }
		 
		 // Start refresh thread for DRAM regions
		 StartRefreshThread();
		 
		 m_logger.Info("MemoryManager", "Memory manager initialized successfully");
		 return true;
	 }
	 catch (const std::exception& e) {
		 m_logger.Error("MemoryManager", std::string("Failed to initialize memory manager: ") + e.what());
		 return false;
	 }
 }
 
 void MemoryManager::Reset() {
	 std::unique_lock<std::shared_mutex> lock(m_memoryMutex);
	 
	 m_logger.Info("MemoryManager", "Resetting memory manager");
	 
	 // Reset all RAM regions to zero but leave ROM intact
	 for (auto& region : m_regions) {
		 if (region.type != MemoryRegionType::ROM) {
			 std::fill(region.data.begin(), region.data.end(), 0);
		 }
	 }
	 
	 // Reset refresh counters
	 m_refreshRowCounter = 0;
	 m_lastRefreshTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		 std::chrono::steady_clock::now().time_since_epoch()).count();
	 m_refreshPending = false;
	 
	 // Reset corruption flag
	 m_memoryCorrupted = false;
	 
	 // Run a quick validation check after reset
	 ValidateMemoryIntegrity();
 }
 
 uint8_t MemoryManager::Read8(uint32_t address) {
	 // Check if refresh is needed before proceeding
	 CheckAndRefreshIfNeeded();
	 
	 // Take a shared lock for reading (allows multiple concurrent readers)
	 std::shared_lock<std::shared_mutex> lock(m_memoryMutex);
	 
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
		 // Release the lock before calling the handler to avoid deadlocks
		 lock.unlock();
		 return region.readHandler8(address);
	 }
	 
	 // Direct memory access
	 if (relativeAddress < region.data.size()) {
		 // For DRAM regions, record the access for memory refreshing
		 if (region.needsRefresh) {
			 // Update access timestamp (not under lock to minimize contention)
			 lock.unlock();
			 UpdateRegionAccessTime(regionIndex);
		 }
		 
		 return region.data[relativeAddress];
	 }
	 
	 // Should never reach here if regions are properly defined
	 HandleIllegalAccess(address, false, 8);
	 return 0xFF;
 }
 
 uint16_t MemoryManager::Read16(uint32_t address) {
	 // Check if refresh is needed before proceeding
	 CheckAndRefreshIfNeeded();
	 
	 // Check for unaligned access
	 if (address & 0x1) {
		 m_logger.Warning("MemoryManager", "Unaligned 16-bit read at 0x" + 
						 std::to_string(address));
		 // Some systems trigger exception on unaligned access, others handle it
		 // For now, just read the individual bytes
		 return (static_cast<uint16_t>(Read8(address)) << 8) | 
				static_cast<uint16_t>(Read8(address + 1));
	 }
	 
	 // Take a shared lock for reading
	 std::shared_lock<std::shared_mutex> lock(m_memoryMutex);
	 
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
		 // Release the lock before calling the handler to avoid deadlocks
		 lock.unlock();
		 return region.readHandler16(address);
	 }
	 
	 // Check if address+1 is still within the same region (could span region boundary)
	 if (relativeAddress + 1 >= region.size) {
		 // Handle reads that cross region boundaries
		 lock.unlock();
		 return (static_cast<uint16_t>(Read8(address)) << 8) | 
				static_cast<uint16_t>(Read8(address + 1));
	 }
	 
	 // Direct memory access (big-endian format for 68000)
	 if (relativeAddress + 1 < region.data.size()) {
		 // For DRAM regions, record the access for memory refreshing
		 if (region.needsRefresh) {
			 // Update access timestamp (not under lock to minimize contention)
			 lock.unlock();
			 UpdateRegionAccessTime(regionIndex);
		 }
		 
		 return (static_cast<uint16_t>(region.data[relativeAddress]) << 8) | 
				static_cast<uint16_t>(region.data[relativeAddress + 1]);
	 }
	 
	 // Should never reach here if regions are properly defined
	 HandleIllegalAccess(address, false, 16);
	 return 0xFFFF;
 }
 
 uint32_t MemoryManager::Read32(uint32_t address) {
	 // Check if refresh is needed before proceeding
	 CheckAndRefreshIfNeeded();
	 
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
	 // Check if refresh is needed before proceeding
	 CheckAndRefreshIfNeeded();
	 
	 // Take an exclusive lock for writing
	 std::unique_lock<std::shared_mutex> lock(m_memoryMutex);
	 
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
		 // Release the lock before calling the handler to avoid deadlocks
		 lock.unlock();
		 region.writeHandler8(address, value);
		 return;
	 }
	 
	 // Direct memory access
	 if (relativeAddress < region.data.size()) {
		 region.data[relativeAddress] = value;
		 
		 // For DRAM regions, update the access timestamp
		 if (region.needsRefresh) {
			 // Calculate row based on address
			 uint32_t row = (relativeAddress / region.refreshRowSize) % region.refreshRowCount;
			 region.refreshMap[row] = true; // Mark this row as recently written
			 
			 // Update the checksum for this memory block for validation
			 UpdateMemoryChecksum(region, relativeAddress, 1);
		 }
		 return;
	 }
	 
	 // Should never reach here if regions are properly defined
	 HandleIllegalAccess(address, true, 8);
 }
 
 void MemoryManager::Write16(uint32_t address, uint16_t value) {
	 // Check if refresh is needed before proceeding
	 CheckAndRefreshIfNeeded();
	 
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
	 
	 // Take an exclusive lock for writing
	 std::unique_lock<std::shared_mutex> lock(m_memoryMutex);
	 
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
		 // Release the lock before calling the handler to avoid deadlocks
		 lock.unlock();
		 region.writeHandler16(address, value);
		 return;
	 }
	 
	 // Check if address+1 is still within the same region (could span region boundary)
	 if (relativeAddress + 1 >= region.size) {
		 // Handle writes that cross region boundaries
		 lock.unlock();
		 Write8(address, (value >> 8) & 0xFF);
		 Write8(address + 1, value & 0xFF);
		 return;
	 }
	 
	 // Direct memory access (big-endian format for 68000)
	 if (relativeAddress + 1 < region.data.size()) {
		 region.data[relativeAddress] = (value >> 8) & 0xFF;
		 region.data[relativeAddress + 1] = value & 0xFF;
		 
		 // For DRAM regions, update the access timestamp
		 if (region.needsRefresh) {
			 // Calculate row based on address
			 uint32_t row = (relativeAddress / region.refreshRowSize) % region.refreshRowCount;
			 region.refreshMap[row] = true; // Mark this row as recently written
			 
			 // Update the checksum for this memory block for validation
			 UpdateMemoryChecksum(region, relativeAddress, 2);
		 }
		 return;
	 }
	 
	 // Should never reach here if regions are properly defined
	 HandleIllegalAccess(address, true, 16);
 }
 
 void MemoryManager::Write32(uint32_t address, uint32_t value) {
	 // Check if refresh is needed before proceeding
	 CheckAndRefreshIfNeeded();
	 
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
	 // Take an exclusive lock for writing
	 std::unique_lock<std::shared_mutex> lock(m_memoryMutex);
	 
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
	 
	 // Calculate ROM checksum for validation
	 CalculateROMChecksum(region);
	 
	 m_logger.Info("MemoryManager", "ROM data loaded successfully: " + 
				  std::to_string(romData.size()) + " bytes at 0x" + 
				  std::to_string(baseAddress));
	 
	 return true;
 }
 
 MemoryRegion* MemoryManager::DefineRegion(const std::string& name, uint32_t startAddress, 
										  uint32_t size, MemoryAccess access, 
										  MemoryRegionType type) {
	 // Take an exclusive lock for writing
	 std::unique_lock<std::shared_mutex> lock(m_memoryMutex);
	 
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
	 
	 // Initialize memory refresh properties
	 region.needsRefresh = (type == MemoryRegionType::MAIN_RAM || 
						   type == MemoryRegionType::VIDEO_RAM || 
						   type == MemoryRegionType::SOUND_RAM);
	 
	 if (region.needsRefresh) {
		 // Configure DRAM refresh parameters
		 region.refreshRowSize = 512;  // Typical DRAM row size (bytes per row)
		 region.refreshRowCount = (size + region.refreshRowSize - 1) / region.refreshRowSize;
		 region.refreshIntervalMs = 15; // Standard refresh interval (15ms)
		 region.refreshMap.resize(region.refreshRowCount, false);
		 region.lastAccessTime = std::chrono::duration_cast<std::chrono::milliseconds>(
			 std::chrono::steady_clock::now().time_since_epoch()).count();
		 
		 // Allocate memory for checksums (1 per 4KB block)
		 region.checksumBlocks = (size + 4095) / 4096;
		 region.checksums.resize(region.checksumBlocks, 0);
	 }
	 
	 // Allocate memory for the region
	 region.data.resize(size, 0);
	 
	 // Add the region to our list
	 m_regions.push_back(region);
	 
	 // Add to name lookup map
	 size_t index = m_regions.size() - 1;
	 m_regionsByName[name] = index;
	 
	 return &m_regions[index];
 }
 
 // Additional methods for thread safety, validation, and refresh
 
 // Configure specific properties for each memory region
 void MemoryManager::ConfigureRegionProperties() {
	 for (auto& region : m_regions) {
		 if (region.type == MemoryRegionType::MAIN_RAM) {
			 // Main RAM typical parameters
			 region.needsRefresh = true;
			 region.refreshRowSize = 512;
			 region.refreshIntervalMs = 15; // 15ms (standard DRAM)
		 } 
		 else if (region.type == MemoryRegionType::VIDEO_RAM) {
			 // Video RAM may need more frequent refresh due to higher access patterns
			 region.needsRefresh = true;
			 region.refreshRowSize = 256; // Smaller rows for more granular refresh
			 region.refreshIntervalMs = 10; // 10ms (faster refresh for video)
		 }
		 else if (region.type == MemoryRegionType::SOUND_RAM) {
			 // Audio RAM
			 region.needsRefresh = true;
			 region.refreshRowSize = 512;
			 region.refreshIntervalMs = 15; // 15ms
		 }
		 else if (region.type == MemoryRegionType::ROM) {
			 // ROM doesn't need refresh
			 region.needsRefresh = false;
		 }
		 
		 // If this is a refresh-needing region, configure its refresh parameters
		 if (region.needsRefresh) {
			 region.refreshRowCount = (region.size + region.refreshRowSize - 1) / region.refreshRowSize;
			 region.refreshMap.resize(region.refreshRowCount, false);
			 
			 // Allocate memory for checksums (1 per 4KB block)
			 region.checksumBlocks = (region.size + 4095) / 4096;
			 region.checksums.resize(region.checksumBlocks, 0);
			 
			 // Calculate initial checksums
			 CalculateRegionChecksums(region);
		 }
	 }
 }
 
 // Start the refresh thread
 void MemoryManager::StartRefreshThread() {
	 if (m_refreshThread.joinable()) {
		 // Thread already running
		 return;
	 }
	 
	 m_refreshThreadRunning = true;
	 m_refreshThread = std::thread(&MemoryManager::RefreshThreadFunc, this);
	 
	 m_logger.Info("MemoryManager", "DRAM refresh thread started");
 }
 
 // Stop the refresh thread
 void MemoryManager::StopRefreshThread() {
	 if (m_refreshThread.joinable()) {
		 m_refreshThreadRunning = false;
		 m_refreshThread.join();
		 m_logger.Info("MemoryManager", "DRAM refresh thread stopped");
	 }
 }
 
 // Memory refresh thread function
 void MemoryManager::RefreshThreadFunc() {
	 while (m_refreshThreadRunning) {
		 // Sleep for a short period (1ms) to avoid busy waiting
		 std::this_thread::sleep_for(std::chrono::milliseconds(1));
		 
		 // Check if it's time to refresh
		 auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
			 std::chrono::steady_clock::now().time_since_epoch()).count();
		 
		 if (currentTime - m_lastRefreshTime >= m_refreshTimerMs) {
			 PerformMemoryRefresh();
			 m_lastRefreshTime = currentTime;
		 }
		 
		 // Periodically validate memory integrity
		 static uint64_t lastValidationTime = currentTime;
		 if (currentTime - lastValidationTime >= 1000) { // Every second
			 ValidateMemoryIntegrity();
			 lastValidationTime = currentTime;
		 }
	 }
 }
 
 // Perform memory refresh operation
 void MemoryManager::PerformMemoryRefresh() {
	 // Set flag to indicate refresh is in progress
	 m_refreshInProgress = true;
	 
	 // Take a unique lock for the refresh operation
	 std::unique_lock<std::shared_mutex> lock(m_memoryMutex);
	 
	 // For each region that needs refresh
	 for (auto& region : m_regions) {
		 if (!region.needsRefresh) continue;
		 
		 // Check if this region is due for refresh
		 auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
			 std::chrono::steady_clock::now().time_since_epoch()).count();
		 
		 if (currentTime - region.lastAccessTime >= region.refreshIntervalMs) {
			 // Refresh this region
			 // In real hardware, this would activate the DRAM refresh circuits
			 // In emulation, we just need to track which rows need refreshing
			 
			 // Refresh a batch of rows (e.g., 8 rows per refresh cycle)
			 const int ROWS_PER_REFRESH = 8;
			 for (int i = 0; i < ROWS_PER_REFRESH; i++) {
				 uint32_t row = (m_refreshRowCounter + i) % region.refreshRowCount;
				 
				 // In real hardware, this would perform the actual refresh
				 // In emulation, we just mark the row as refreshed
				 region.refreshMap[row] = true;
			 }
			 
			 // Update the refresh counter
			 m_refreshRowCounter = (m_refreshRowCounter + ROWS_PER_REFRESH) % region.refreshRowCount;
			 
			 // Update the last access time
			 region.lastAccessTime = currentTime;
		 }
	 }
	 
	 // Clear the refresh in progress flag
	 m_refreshInProgress = false;
 }
 
 // Check if refresh is needed and perform it if necessary
 void MemoryManager::CheckAndRefreshIfNeeded() {
	 if (!m_autoRefreshEnabled) return;
	 
	 auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		 std::chrono::steady_clock::now().time_since_epoch()).count();
	 
	 // If it's been too long since the last refresh, force one now
	 if (currentTime - m_lastRefreshTime >= m_refreshTimerMs) {
		 // Only do this if the refresh thread isn't already handling it
		 if (!m_refreshInProgress) {
			 PerformMemoryRefresh();
			 m_lastRefreshTime = currentTime;
		 }
	 }
 }
 
 // Update region access time
 void MemoryManager::UpdateRegionAccessTime(int regionIndex) {
	 if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) {
		 return;
	 }
	 
	 auto& region = m_regions[regionIndex];
	 if (!region.needsRefresh) return;
	 
	 // Update the last access time atomically
	 region.lastAccessTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		 std::chrono::steady_clock::now().time_since_epoch()).count();
 }
 
 // Calculate checksum for ROM validation
 void MemoryManager::CalculateROMChecksum(MemoryRegion& region) {
	 if (region.type != MemoryRegionType::ROM) return;
	 
	 // Simple 32-bit checksum algorithm
	 uint32_t checksum = 0;
	 for (size_t i = 0; i < region.data.size(); i++) {
		 checksum = ((checksum << 1) | (checksum >> 31)) ^ region.data[i];
	 }
	 
	 region.romChecksum = checksum;
	 m_logger.Info("MemoryManager", "ROM checksum calculated: 0x" + 
				  std::to_string(checksum) + " for region: " + region.name);
 }
 
 // Calculate checksums for each block in a region
 void MemoryManager::CalculateRegionChecksums(MemoryRegion& region) {
	 if (!region.needsRefresh) return;
	 
	 const uint32_t CHECKSUM_BLOCK_SIZE = 4096; // 4KB blocks
	 
	 for (uint32_t block = 0; block < region.checksumBlocks; block++) {
		 uint32_t startOffset = block * CHECKSUM_BLOCK_SIZE;
		 uint32_t endOffset = std::min(startOffset + CHECKSUM_BLOCK_SIZE, region.size);
		 
		 uint32_t checksum = 0;
		 for (uint32_t i = startOffset; i < endOffset; i++) {
			 checksum = ((checksum << 1) | (checksum >> 31)) ^ region.data[i];
		 }
		 
		 region.checksums[block] = checksum;
	 }
 }
 
// Update checksum for a specific memory block
void MemoryManager::UpdateMemoryChecksum(MemoryRegion& region, uint32_t offset, uint32_t size) {
    if (!region.needsRefresh) return;
    
    const uint32_t CHECKSUM_BLOCK_SIZE = 4096; // 4KB blocks
    
    // Determine which block(s) are affected
    uint32_t startBlock = offset / CHECKSUM_BLOCK_SIZE;
    uint32_t endBlock = (offset + size - 1) / CHECKSUM_BLOCK_SIZE;
    
    // Recalculate checksums for affected blocks
    for (uint32_t block = startBlock; block <= endBlock && block < region.checksumBlocks; block++) {
        uint32_t blockStartOffset = block * CHECKSUM_BLOCK_SIZE;
        uint32_t blockEndOffset = std::min(blockStartOffset + CHECKSUM_BLOCK_SIZE, region.size);
        
        uint32_t checksum = 0;
        for (uint32_t i = blockStartOffset; i < blockEndOffset; i++) {
            checksum = ((checksum << 1) | (checksum >> 31)) ^ region.data[i];
        }
        
        region.checksums[block] = checksum;
    }
}

// Validate memory integrity
bool MemoryManager::ValidateMemoryIntegrity() {
    // Take a shared lock for reading
    std::shared_lock<std::shared_mutex> lock(m_memoryMutex);
    
    bool allValid = true;
    
    for (auto& region : m_regions) {
        // Skip regions that don't need validation
        if (region.type == MemoryRegionType::ROM) {
            // Validate ROM checksum
            uint32_t currentChecksum = 0;
            for (size_t i = 0; i < region.data.size(); i++) {
                currentChecksum = ((currentChecksum << 1) | (currentChecksum >> 31)) ^ region.data[i];
            }
            
            if (currentChecksum != region.romChecksum) {
                m_logger.Error("MemoryManager", "ROM checksum validation failed for region: " + 
                              region.name);
                allValid = false;
                m_memoryCorrupted = true;
            }
        }
        else if (region.needsRefresh) {
            // Validate RAM checksums
            const uint32_t CHECKSUM_BLOCK_SIZE = 4096;
            
            for (uint32_t block = 0; block < region.checksumBlocks; block++) {
                uint32_t blockStartOffset = block * CHECKSUM_BLOCK_SIZE;
                uint32_t blockEndOffset = std::min(blockStartOffset + CHECKSUM_BLOCK_SIZE, region.size);
                
                uint32_t currentChecksum = 0;
                for (uint32_t i = blockStartOffset; i < blockEndOffset; i++) {
                    currentChecksum = ((currentChecksum << 1) | (currentChecksum >> 31)) ^ region.data[i];
                }
                
                if (currentChecksum != region.checksums[block]) {
                    m_logger.Error("MemoryManager", "RAM checksum validation failed for region: " + 
                                  region.name + ", block: " + std::to_string(block));
                    allValid = false;
                    m_memoryCorrupted = true;
                    
                    // Attempt to correct the corruption if possible
                    if (m_autoCorrectErrors) {
                        // In a real system, this might use ECC to correct errors
                        // For this emulation, we just update the checksum
                        region.checksums[block] = currentChecksum;
                        m_logger.Warning("MemoryManager", "Updated checksum for block: " + 
                                       std::to_string(block) + " in region: " + region.name);
                    }
                }
            }
        }
    }
    
    return allValid;
}

// Run memory self-test
bool MemoryManager::RunMemorySelfTest() {
    m_logger.Info("MemoryManager", "Running memory self-test");
    
    bool allTestsPassed = true;
    
    // Take an exclusive lock for the self-test
    std::unique_lock<std::shared_mutex> lock(m_memoryMutex);
    
    // For each writable region
    for (auto& region : m_regions) {
        if (region.access != MemoryAccess::READ_WRITE) {
            continue; // Skip read-only regions
        }
        
        m_logger.Info("MemoryManager", "Testing region: " + region.name);
        
        // Save original content to restore after testing
        std::vector<uint8_t> originalContent;
        if (region.size > 0) {
            originalContent = region.data;
        }
        
        // Run test patterns
        if (!TestPatternWalkingOnes(region)) {
            m_logger.Error("MemoryManager", "Walking ones test failed for region: " + region.name);
            allTestsPassed = false;
        }
        
        if (!TestPatternWalkingZeros(region)) {
            m_logger.Error("MemoryManager", "Walking zeros test failed for region: " + region.name);
            allTestsPassed = false;
        }
        
        if (!TestPatternCheckerboard(region)) {
            m_logger.Error("MemoryManager", "Checkerboard test failed for region: " + region.name);
            allTestsPassed = false;
        }
        
        if (!TestPatternAddressInCell(region)) {
            m_logger.Error("MemoryManager", "Address-in-cell test failed for region: " + region.name);
            allTestsPassed = false;
        }
        
        if (!TestRandomPattern(region)) {
            m_logger.Error("MemoryManager", "Random pattern test failed for region: " + region.name);
            allTestsPassed = false;
        }
        
        // Restore original content
        if (region.size > 0 && originalContent.size() == region.size) {
            region.data = originalContent;
        }
        
        // Calculate checksums for the region
        if (region.needsRefresh) {
            CalculateRegionChecksums(region);
        }
    }
    
    if (allTestsPassed) {
        m_logger.Info("MemoryManager", "All memory tests passed successfully");
    } else {
        m_logger.Error("MemoryManager", "Memory tests failed");
    }
    
    return allTestsPassed;
}

// Test pattern: Walking ones
bool MemoryManager::TestPatternWalkingOnes(MemoryRegion& region) {
    // Skip empty regions
    if (region.size == 0) return true;
    
    for (size_t patternIndex = 0; patternIndex < 8; patternIndex++) {
        uint8_t pattern = TEST_PATTERN_WALKING_ONES[patternIndex];
        
        // Write pattern
        std::fill(region.data.begin(), region.data.end(), pattern);
        
        // Verify pattern
        for (size_t i = 0; i < region.size; i++) {
            if (region.data[i] != pattern) {
                m_logger.Error("MemoryManager", "Walking ones test failed at offset: " + 
                              std::to_string(i) + ", expected: " + std::to_string(pattern) + 
                              ", actual: " + std::to_string(region.data[i]));
                return false;
            }
        }
    }
    
    return true;
}

// Test pattern: Walking zeros
bool MemoryManager::TestPatternWalkingZeros(MemoryRegion& region) {
    // Skip empty regions
    if (region.size == 0) return true;
    
    for (size_t patternIndex = 0; patternIndex < 8; patternIndex++) {
        uint8_t pattern = TEST_PATTERN_WALKING_ZEROS[patternIndex];
        
        // Write pattern
        std::fill(region.data.begin(), region.data.end(), pattern);
        
        // Verify pattern
        for (size_t i = 0; i < region.size; i++) {
            if (region.data[i] != pattern) {
                m_logger.Error("MemoryManager", "Walking zeros test failed at offset: " + 
                              std::to_string(i) + ", expected: " + std::to_string(pattern) + 
                              ", actual: " + std::to_string(region.data[i]));
                return false;
            }
        }
    }
    
    return true;
}

// Test pattern: Checkerboard
bool MemoryManager::TestPatternCheckerboard(MemoryRegion& region) {
    // Skip empty regions
    if (region.size == 0) return true;
    
    // First pattern: 0x55 (01010101)
    std::fill(region.data.begin(), region.data.end(), 0x55);
    
    // Verify first pattern
    for (size_t i = 0; i < region.size; i++) {
        if (region.data[i] != 0x55) {
            m_logger.Error("MemoryManager", "Checkerboard test (0x55) failed at offset: " + 
                          std::to_string(i) + ", expected: 0x55, actual: " + 
                          std::to_string(region.data[i]));
            return false;
        }
    }
    
    // Second pattern: 0xAA (10101010)
    std::fill(region.data.begin(), region.data.end(), 0xAA);
    
    // Verify second pattern
    for (size_t i = 0; i < region.size; i++) {
        if (region.data[i] != 0xAA) {
            m_logger.Error("MemoryManager", "Checkerboard test (0xAA) failed at offset: " + 
                          std::to_string(i) + ", expected: 0xAA, actual: " + 
                          std::to_string(region.data[i]));
            return false;
        }
    }
    
    return true;
}

// Test pattern: Address in cell
bool MemoryManager::TestPatternAddressInCell(MemoryRegion& region) {
    // Skip empty regions
    if (region.size == 0) return true;
    
    // Write address to each cell
    for (size_t i = 0; i < region.size; i++) {
        region.data[i] = i & 0xFF; // Lower 8 bits of address
    }
    
    // Verify pattern
    for (size_t i = 0; i < region.size; i++) {
        if (region.data[i] != (i & 0xFF)) {
            m_logger.Error("MemoryManager", "Address-in-cell test failed at offset: " + 
                          std::to_string(i) + ", expected: " + std::to_string(i & 0xFF) + 
                          ", actual: " + std::to_string(region.data[i]));
            return false;
        }
    }
    
    return true;
}

// Test pattern: Random pattern
bool MemoryManager::TestRandomPattern(MemoryRegion& region) {
    // Skip empty regions
    if (region.size == 0) return true;
    
    // Create a random number generator with fixed seed for reproducibility
    std::mt19937 rng(TEST_PATTERN_RANDOM_SEED);
    std::uniform_int_distribution<> dist(0, 255);
    
    // Generate and store random values
    std::vector<uint8_t> expectedValues(region.size);
    for (size_t i = 0; i < region.size; i++) {
        uint8_t randValue = static_cast<uint8_t>(dist(rng));
        region.data[i] = randValue;
        expectedValues[i] = randValue;
    }
    
    // Verify pattern
    for (size_t i = 0; i < region.size; i++) {
        if (region.data[i] != expectedValues[i]) {
            m_logger.Error("MemoryManager", "Random pattern test failed at offset: " + 
                          std::to_string(i) + ", expected: " + std::to_string(expectedValues[i]) + 
                          ", actual: " + std::to_string(region.data[i]));
            return false;
        }
    }
    
    return true;
}

// Set refresh parameters
void MemoryManager::SetRefreshParameters(uint32_t refreshIntervalMs, bool autoRefresh) {
    m_refreshTimerMs = refreshIntervalMs;
    m_autoRefreshEnabled = autoRefresh;
    
    m_logger.Info("MemoryManager", "Refresh parameters updated: interval=" + 
                 std::to_string(refreshIntervalMs) + "ms, autoRefresh=" + 
                 (autoRefresh ? "true" : "false"));
}

// Enable/disable auto-correction of memory errors
void MemoryManager::SetAutoCorrectErrors(bool enable) {
    m_autoCorrectErrors = enable;
    
    m_logger.Info("MemoryManager", std::string("Auto-correct memory errors ") + 
                 (enable ? "enabled" : "disabled"));
}

// Check if memory is corrupted
bool MemoryManager::IsMemoryCorrupted() const {
    return m_memoryCorrupted;
}

// Get statistics on memory accesses and refresh operations
MemoryStats MemoryManager::GetMemoryStats() const {
    MemoryStats stats;
    
    // Calculate statistics
    stats.totalMemorySize = 0;
    stats.totalRefreshableMemory = 0;
    stats.refreshCycleCount = 0;
    stats.memoryErrors = 0;
    
    for (const auto& region : m_regions) {
        stats.totalMemorySize += region.size;
        
        if (region.needsRefresh) {
            stats.totalRefreshableMemory += region.size;
        }
    }
    
    stats.memoryCorrupted = m_memoryCorrupted;
    
    return stats;
}

// Memory leak detection (for debugging the emulator itself)
void MemoryManager::CheckMemoryLeaks() {
    m_logger.Info("MemoryManager", "Checking for memory leaks in emulator");
    
    // This would implement checks for memory leaks in the emulator itself
    // For simplicity, we just report the current memory usage
    
    size_t totalMemoryUsage = 0;
    for (const auto& region : m_regions) {
        totalMemoryUsage += region.data.size();
        totalMemoryUsage += region.refreshMap.size();
        totalMemoryUsage += region.checksums.size() * sizeof(uint32_t);
    }
    
    m_logger.Info("MemoryManager", "Total memory usage: " + 
                 std::to_string(totalMemoryUsage) + " bytes");
}

// Implement memory manager callback registration methods

bool MemoryManager::RegisterMemoryAccessCallback(MemoryAccessType type, 
                                              std::function<void(uint32_t, uint8_t)> callback) {
    switch (type) {
        case MemoryAccessType::READ:
            m_readCallbacks.push_back(callback);
            break;
        case MemoryAccessType::WRITE:
            m_writeCallbacks.push_back(callback);
            break;
        case MemoryAccessType::REFRESH:
            m_refreshCallbacks.push_back(callback);
            break;
        default:
            return false;
    }
    
    return true;
}

void MemoryManager::NotifyCallbacks(MemoryAccessType type, uint32_t address, uint8_t value) {
    switch (type) {
        case MemoryAccessType::READ:
            for (const auto& callback : m_readCallbacks) {
                callback(address, value);
            }
            break;
        case MemoryAccessType::WRITE:
            for (const auto& callback : m_writeCallbacks) {
                callback(address, value);
            }
            break;
        case MemoryAccessType::REFRESH:
            for (const auto& callback : m_refreshCallbacks) {
                callback(address, value);
            }
            break;
    }
}

} // namespace NiXX32