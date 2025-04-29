/**
 * MemoryManager.h
 * Memory management system for NiXX-32 arcade board emulation
 */

 #pragma once

 #include <cstdint>
 #include <memory>
 #include <vector>
 #include <unordered_map>
 #include <string>
 #include <functional>
 #include <thread>
 #include <atomic>
 #include <shared_mutex>
 
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 enum class HardwareVariant;

 /**
  * Memory access permissions
  */
 enum class MemoryAccess {
     READ_ONLY,      // Memory can only be read
     WRITE_ONLY,     // Memory can only be written to
     READ_WRITE,     // Memory can be both read and written
     NONE            // Memory cannot be accessed (unmapped or prohibited)
 };
 
 /**
  * Memory region types
  */
 enum class MemoryRegionType {
     ROM,            // Read-only program memory
     MAIN_RAM,       // Main system RAM
     VIDEO_RAM,      // Video memory for graphics processing
     SOUND_RAM,      // Audio subsystem memory
     IO_REGISTERS,   // Hardware I/O registers
     EXPANSION       // Expansion or custom hardware memory
 };

 /**
  * Memory access types for callbacks
  */
 enum class MemoryAccessType {
     READ,          // Memory read operation
     WRITE,         // Memory write operation
     REFRESH,       // Memory refresh operation
     ERROR          // Memory error or corruption
 };

 /**
  * Memory statistics structure
  */
 struct MemoryStats {
     uint64_t totalMemorySize;        // Total memory size in bytes
     uint64_t totalRefreshableMemory; // Total memory that needs refresh
     uint64_t refreshCycleCount;      // Number of refresh cycles performed
     uint64_t memoryErrors;           // Number of memory errors detected
     bool memoryCorrupted;            // Whether memory corruption was detected
 };
 
 /**
  * Defines a memory region in the system
  */
 struct MemoryRegion {
     std::string name;              // Name of the region for debugging
     uint32_t startAddress;         // Start address in the address space
     uint32_t size;                 // Size of the region in bytes
     MemoryAccess access;           // Access permissions
     MemoryRegionType type;         // Type of memory region
     std::vector<uint8_t> data;     // Actual memory data
     
     // Optional handlers for memory-mapped I/O
     std::function<uint8_t(uint32_t)> readHandler8;       // 8-bit read handler
     std::function<uint16_t(uint32_t)> readHandler16;     // 16-bit read handler
     std::function<void(uint32_t, uint8_t)> writeHandler8;    // 8-bit write handler
     std::function<void(uint32_t, uint16_t)> writeHandler16;  // 16-bit write handler
     
     // Memory refresh properties
     bool needsRefresh;               // Whether this region needs refreshing (DRAM)
     uint32_t refreshRowSize;         // Size of each refresh row in bytes
     uint32_t refreshRowCount;        // Number of refresh rows in this region
     uint32_t refreshIntervalMs;      // Refresh interval in milliseconds
     std::vector<bool> refreshMap;    // Map of rows that need refreshing
     uint64_t lastAccessTime;         // Last time this region was accessed
     
     // Memory validation and checksums
     uint32_t romChecksum;            // Checksum for ROM regions
     uint32_t checksumBlocks;         // Number of checksum blocks
     std::vector<uint32_t> checksums; // Checksums for each block
     
     bool hasHandlers() const {
         return readHandler8 || readHandler16 || writeHandler8 || writeHandler16;
     }
 };
 
 /**
  * Memory manager class that handles all memory operations
  */
 class MemoryManager {
 public:
     /**
      * Constructor
      * @param system Reference to the parent system
      * @param logger Reference to the system logger
      */
     MemoryManager(System& system, Logger& logger);
     
     /**
      * Destructor
      */
     ~MemoryManager();
     
     /**
      * Initialize the memory system for the specified hardware variant
      * @param variant Hardware variant to configure for
      * @return True if initialization was successful
      */
     bool Initialize(HardwareVariant variant);
     
     /**
      * Reset all memory to initial state
      */
     void Reset();
     
     /**
      * Read 8-bit value from memory
      * @param address Memory address to read from
      * @return 8-bit value at the address
      */
     uint8_t Read8(uint32_t address);
     
     /**
      * Read 16-bit value from memory
      * @param address Memory address to read from (should be even)
      * @return 16-bit value at the address
      */
     uint16_t Read16(uint32_t address);
     
     /**
      * Read 32-bit value from memory
      * @param address Memory address to read from (should be even)
      * @return 32-bit value at the address
      */
     uint32_t Read32(uint32_t address);
     
     /**
      * Write 8-bit value to memory
      * @param address Memory address to write to
      * @param value 8-bit value to write
      */
     void Write8(uint32_t address, uint8_t value);
     
     /**
      * Write 16-bit value to memory
      * @param address Memory address to write to (should be even)
      * @param value 16-bit value to write
      */
     void Write16(uint32_t address, uint16_t value);
     
     /**
      * Write 32-bit value to memory
      * @param address Memory address to write to (should be even)
      * @param value 32-bit value to write
      */
     void Write32(uint32_t address, uint32_t value);
     
     /**
      * Load ROM data into memory
      * @param romData Vector containing ROM data
      * @param baseAddress Address to load the ROM at
      * @return True if ROM was loaded successfully
      */
     bool LoadROM(const std::vector<uint8_t>& romData, uint32_t baseAddress);
     
     /**
      * Define a new memory region
      * @param name Name of the region for debugging
      * @param startAddress Start address in the address space
      * @param size Size of the region in bytes
      * @param access Access permissions
      * @param type Type of memory region
      * @return Pointer to the created region, or nullptr if failed
      */
     MemoryRegion* DefineRegion(const std::string& name, uint32_t startAddress, 
                               uint32_t size, MemoryAccess access, 
                               MemoryRegionType type);
     
     /**
      * Get a memory region by address
      * @param address Address within the region
      * @return Pointer to the region, or nullptr if not found
      */
     MemoryRegion* GetRegionByAddress(uint32_t address);
     
     /**
      * Get a memory region by name
      * @param name Name of the region
      * @return Pointer to the region, or nullptr if not found
      */
     MemoryRegion* GetRegionByName(const std::string& name);
     
     /**
      * Set a read handler for a memory region
      * @param regionName Name of the region
      * @param handler8 8-bit read handler function
      * @param handler16 16-bit read handler function
      * @return True if handlers were set successfully
      */
     bool SetReadHandlers(const std::string& regionName,
                        std::function<uint8_t(uint32_t)> handler8,
                        std::function<uint16_t(uint32_t)> handler16);
     
     /**
      * Set a write handler for a memory region
      * @param regionName Name of the region
      * @param handler8 8-bit write handler function
      * @param handler16 16-bit write handler function
      * @return True if handlers were set successfully
      */
     bool SetWriteHandlers(const std::string& regionName,
                         std::function<void(uint32_t, uint8_t)> handler8,
                         std::function<void(uint32_t, uint16_t)> handler16);
     
     /**
      * Get direct pointer to memory at specified address
      * @param address Memory address
      * @param size Size of requested memory block
      * @return Pointer to memory, or nullptr if invalid
      */
     uint8_t* GetDirectPointer(uint32_t address, uint32_t size);
     
     /**
      * Create memory configuration for specific hardware variant
      * @param variant Hardware variant to configure for
      */
     void ConfigureMemoryMap(HardwareVariant variant);

     /**
      * Run memory self-test
      * @return True if all tests pass
      */
     bool RunMemorySelfTest();
     
     /**
      * Validate memory integrity
      * @return True if all memory is valid
      */
     bool ValidateMemoryIntegrity();
     
     /**
      * Set refresh parameters
      * @param refreshIntervalMs Refresh interval in milliseconds
      * @param autoRefresh Whether to enable automatic refresh
      */
     void SetRefreshParameters(uint32_t refreshIntervalMs, bool autoRefresh);
     
     /**
      * Enable/disable auto-correction of memory errors
      * @param enable True to enable, false to disable
      */
     void SetAutoCorrectErrors(bool enable);
     
     /**
      * Check if memory is corrupted
      * @return True if memory corruption was detected
      */
     bool IsMemoryCorrupted() const;
     
     /**
      * Get memory statistics
      * @return Memory statistics structure
      */
     MemoryStats GetMemoryStats() const;
     
     /**
      * Register memory access callback
      * @param type Type of memory access to monitor
      * @param callback Function to call on memory access
      * @return True if successful
      */
     bool RegisterMemoryAccessCallback(MemoryAccessType type, 
                                    std::function<void(uint32_t, uint8_t)> callback);
     
     /**
      * Check for memory leaks (for debugging)
      */
     void CheckMemoryLeaks();

 private:
     // Reference to parent system
     System& m_system;
     
     // Reference to logger
     Logger& m_logger;
     
     // Memory regions by address space
     std::vector<MemoryRegion> m_regions;
     
     // Quick lookup by name
     std::unordered_map<std::string, size_t> m_regionsByName;
     
     // Hardware variant currently configured for
     HardwareVariant m_configuredVariant;
     
     // Memory limits based on variant
     uint32_t m_mainRamSize;
     uint32_t m_videoRamSize;
     uint32_t m_soundRamSize;
     uint32_t m_maxRomSize;
     
     // Thread-safety mutex
     mutable std::shared_mutex m_memoryMutex;
     
     // Memory refresh parameters
     uint32_t m_refreshTimerMs;
     uint64_t m_lastRefreshTime;
     std::atomic<bool> m_refreshPending;
     uint32_t m_refreshRowCounter;
     std::atomic<bool> m_autoRefreshEnabled;
     std::atomic<bool> m_autoCorrectErrors;
     
     // Self-test and validation
     bool m_selfTestOnBoot;
     std::atomic<bool> m_memoryCorrupted;
     
     // Refresh thread
     std::thread m_refreshThread;
     std::atomic<bool> m_refreshThreadRunning;
     std::atomic<bool> m_refreshInProgress;
     
     // Memory access callbacks
     std::vector<std::function<void(uint32_t, uint8_t)>> m_readCallbacks;
     std::vector<std::function<void(uint32_t, uint8_t)>> m_writeCallbacks;
     std::vector<std::function<void(uint32_t, uint8_t)>> m_refreshCallbacks;
     
     /**
      * Configure original NiXX-32 hardware
      */
     void ConfigureOriginalHardware();
     
     /**
      * Configure enhanced NiXX-32+ hardware
      */
     void ConfigurePlusHardware();
     
     /**
      * Configure region-specific properties
      */
     void ConfigureRegionProperties();
     
     /**
      * Find a memory region that contains the specified address
      * @param address Memory address to look up
      * @return Index of the region, or -1 if not found
      */
     int FindRegionIndex(uint32_t address);
     
     /**
      * Convert system address to region-relative address
      * @param address System address
      * @param regionIndex Index of the region
      * @return Address relative to region start
      */
     uint32_t GetRegionRelativeAddress(uint32_t address, int regionIndex);
     
     /**
      * Handle illegal memory access
      * @param address Accessed address
      * @param isWrite True if it was a write operation
      * @param size Size of the access (8, 16, or 32 bits)
      */
     void HandleIllegalAccess(uint32_t address, bool isWrite, int size);
     
     /**
      * Start the refresh thread
      */
     void StartRefreshThread();
     
     /**
      * Stop the refresh thread
      */
     void StopRefreshThread();
     
     /**
      * Memory refresh thread function
      */
     void RefreshThreadFunc();
     
     /**
      * Perform memory refresh operation
      */
     void PerformMemoryRefresh();
     
     /**
      * Check if refresh is needed and perform it if necessary
      */
     void CheckAndRefreshIfNeeded();
     
     /**
      * Update region access time
      * @param regionIndex Index of the region
      */
     void UpdateRegionAccessTime(int regionIndex);
     
     /**
      * Calculate checksum for ROM validation
      * @param region Memory region
      */
     void CalculateROMChecksum(MemoryRegion& region);
     
     /**
      * Calculate checksums for each block in a region
      * @param region Memory region
      */
     void CalculateRegionChecksums(MemoryRegion& region);
     
     /**
      * Update checksum for a specific memory block
      * @param region Memory region
      * @param offset Offset within the region
      * @param size Size of the updated area
      */
     void UpdateMemoryChecksum(MemoryRegion& region, uint32_t offset, uint32_t size);
     
     /**
      * Test pattern: Walking ones
      * @param region Memory region to test
      * @return True if test passes
      */
     bool TestPatternWalkingOnes(MemoryRegion& region);
     
     /**
      * Test pattern: Walking zeros
      * @param region Memory region to test
      * @return True if test passes
      */
     bool TestPatternWalkingZeros(MemoryRegion& region);
	
	/**
      * Test pattern: Checkerboard
      * @param region Memory region to test
      * @return True if test passes
      */
     bool TestPatternCheckerboard(MemoryRegion& region);
     
     /**
      * Test pattern: Address in cell
      * @param region Memory region to test
      * @return True if test passes
      */
     bool TestPatternAddressInCell(MemoryRegion& region);
     
     /**
      * Test pattern: Random pattern
      * @param region Memory region to test
      * @return True if test passes
      */
     bool TestRandomPattern(MemoryRegion& region);
     
     /**
      * Notify callbacks for memory operations
      * @param type Type of memory access
      * @param address Memory address
      * @param value Data value
      */
     void NotifyCallbacks(MemoryAccessType type, uint32_t address, uint8_t value);
 };
 
} // namespace NiXX32