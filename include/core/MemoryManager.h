/**
 * MemoryManager.h
 * Memory management system for NiXX-32 arcade board emulation
 * 
 * This file defines the memory management system that handles allocation,
 * mapping, and access to the various memory regions of the NiXX-32 hardware.
 * It supports both the original and enhanced hardware variants.
 */

 #pragma once

 #include <cstdint>
 #include <memory>
 #include <vector>
 #include <unordered_map>
 #include <string>
 #include <functional>
 
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
	 
	 /**
	  * Configure memory for original NiXX-32 hardware
	  */
	 void ConfigureOriginalHardware();
	 
	 /**
	  * Configure memory for enhanced NiXX-32+ hardware
	  */
	 void ConfigurePlusHardware();
	 
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
 };
 
 } // namespace NiXX32