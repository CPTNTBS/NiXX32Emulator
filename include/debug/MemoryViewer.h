/**
 * MemoryViewer.h
 * Memory inspection and debugging system for NiXX-32 arcade board emulation
 * 
 * This file defines the memory viewing and editing functionality that allows
 * visualization and modification of system memory during debugging for the
 * NiXX-32 arcade hardware emulation.
 */

 #pragma once

 #include <cstdint>
 #include <string>
 #include <vector>
 #include <unordered_map>
 #include <functional>
 #include <memory>
 
 #include "MemoryManager.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class Debugger;
 
 /**
  * Memory view display formats
  */
 enum class MemoryDisplayFormat {
	 HEX_8BIT,        // 8-bit hexadecimal display
	 HEX_16BIT,       // 16-bit hexadecimal display
	 HEX_32BIT,       // 32-bit hexadecimal display
	 DECIMAL_8BIT,    // 8-bit decimal display
	 DECIMAL_16BIT,   // 16-bit decimal display
	 DECIMAL_32BIT,   // 32-bit decimal display
	 ASCII,           // ASCII character display
	 DISASSEMBLY      // Instruction disassembly
 };
 
 /**
  * Memory region definition for viewer
  */
 struct MemoryViewRegion {
	 std::string name;        // Region name
	 uint32_t startAddress;   // Start address
	 uint32_t size;           // Region size in bytes
	 bool readOnly;           // Whether region is read-only
	 std::string description; // Description of the region
 };
 
 /**
  * Memory dump options
  */
 struct MemoryDumpOptions {
	 MemoryDisplayFormat format;     // Display format
	 uint32_t bytesPerRow;           // Bytes to display per row
	 bool showAddresses;             // Whether to show addresses
	 bool showAscii;                 // Whether to show ASCII representation
	 bool showSymbols;               // Whether to show symbol names
 };
 
 /**
  * Memory comparison result
  */
 struct MemoryCompareResult {
	 uint32_t address;               // Address where difference was found
	 uint8_t value1;                 // Value from first memory area
	 uint8_t value2;                 // Value from second memory area
 };
 
 /**
  * Memory search patterns
  */
 enum class SearchPatternType {
	 BYTE_SEQUENCE,    // Sequence of bytes
	 TEXT_STRING,      // ASCII text string
	 VALUE_8BIT,       // 8-bit value
	 VALUE_16BIT,      // 16-bit value
	 VALUE_32BIT       // 32-bit value
 };
 
 /**
  * Memory search condition
  */
 enum class SearchCondition {
	 EQUALS,           // Exactly equals
	 NOT_EQUALS,       // Does not equal
	 GREATER_THAN,     // Greater than
	 LESS_THAN,        // Less than
	 CHANGED,          // Value changed
	 NOT_CHANGED,      // Value not changed
	 INCREASED,        // Value increased
	 DECREASED         // Value decreased
 };
 
 /**
  * Memory search parameters
  */
 struct SearchParams {
	 SearchPatternType patternType;  // Type of pattern to search for
	 SearchCondition condition;      // Search condition
	 std::vector<uint8_t> pattern;   // Pattern data
	 uint32_t startAddress;          // Start address for search
	 uint32_t endAddress;            // End address for search
	 bool caseSensitive;             // Case sensitivity for text search
 };
 
 /**
  * Memory search result
  */
 struct SearchResult {
	 uint32_t address;               // Address where match was found
	 std::vector<uint8_t> value;     // Value at the address
 };
 
 /**
  * Class for viewing and editing system memory
  */
 class MemoryViewer {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param debugger Reference to the debugger
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 MemoryViewer(System& system, Debugger& debugger,
				 MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~MemoryViewer();
	 
	 /**
	  * Initialize the memory viewer
	  * @return True if initialization was successful
	  */
	 bool Initialize();
	 
	 /**
	  * Reset the memory viewer state
	  */
	 void Reset();
	 
	 /**
	  * Update memory viewer state
	  */
	 void Update();
	 
	 /**
	  * View memory at specified address
	  * @param address Address to view
	  * @param size Number of bytes to view
	  * @param format Display format
	  * @return String representation of memory contents
	  */
	 std::string ViewMemory(uint32_t address, uint32_t size, 
						  MemoryDisplayFormat format = MemoryDisplayFormat::HEX_8BIT);
	 
	 /**
	  * Edit memory at specified address
	  * @param address Address to edit
	  * @param values Vector of byte values to write
	  * @return True if edit was successful
	  */
	 bool EditMemory(uint32_t address, const std::vector<uint8_t>& values);
	 
	 /**
	  * Fill memory with a pattern
	  * @param startAddress Start address of region to fill
	  * @param size Size of region in bytes
	  * @param pattern Pattern to fill with
	  * @return True if fill was successful
	  */
	 bool FillMemory(uint32_t startAddress, uint32_t size, 
				   const std::vector<uint8_t>& pattern);
	 
	 /**
	  * Copy memory from one region to another
	  * @param sourceAddress Source address
	  * @param destAddress Destination address
	  * @param size Number of bytes to copy
	  * @return True if copy was successful
	  */
	 bool CopyMemory(uint32_t sourceAddress, uint32_t destAddress, uint32_t size);
	 
	 /**
	  * Dump memory to a file
	  * @param startAddress Start address
	  * @param size Size in bytes
	  * @param filename File to save to
	  * @param options Dump format options
	  * @return True if dump was successful
	  */
	 bool DumpMemoryToFile(uint32_t startAddress, uint32_t size, 
						 const std::string& filename,
						 const MemoryDumpOptions& options = {});
	 
	 /**
	  * Load data from a file into memory
	  * @param address Destination address
	  * @param filename File to load from
	  * @return True if load was successful
	  */
	 bool LoadMemoryFromFile(uint32_t address, const std::string& filename);
	 
	 /**
	  * Compare two memory regions
	  * @param address1 First address
	  * @param address2 Second address
	  * @param size Size to compare
	  * @return Vector of differences
	  */
	 std::vector<MemoryCompareResult> CompareMemory(uint32_t address1, 
												  uint32_t address2, 
												  uint32_t size);
	 
	 /**
	  * Search memory for a pattern
	  * @param params Search parameters
	  * @return Vector of search results
	  */
	 std::vector<SearchResult> SearchMemory(const SearchParams& params);
	 
	 /**
	  * Continue search with refined parameters
	  * @param params New search parameters
	  * @return Vector of refined search results
	  */
	 std::vector<SearchResult> ContinueSearch(const SearchParams& params);
	 
	 /**
	  * Get memory map information
	  * @return Vector of memory regions
	  */
	 std::vector<MemoryViewRegion> GetMemoryMap() const;
	 
	 /**
	  * Get the memory region containing an address
	  * @param address Address to look up
	  * @return Memory region, or nullptr if not found
	  */
	 MemoryViewRegion* GetRegionForAddress(uint32_t address);
	 
	 /**
	  * Register a memory change callback
	  * @param callback Function to call when memory is changed
	  * @return Callback ID
	  */
	 int RegisterChangeCallback(std::function<void(uint32_t, uint32_t)> callback);
	 
	 /**
	  * Remove a memory change callback
	  * @param callbackId Callback ID
	  * @return True if successful
	  */
	 bool RemoveChangeCallback(int callbackId);
	 
	 /**
	  * Set memory bookmark
	  * @param address Address to bookmark
	  * @param name Bookmark name
	  * @param description Bookmark description
	  * @return True if successful
	  */
	 bool SetBookmark(uint32_t address, const std::string& name, 
					const std::string& description = "");
	 
	 /**
	  * Remove memory bookmark
	  * @param address Bookmarked address
	  * @return True if successful
	  */
	 bool RemoveBookmark(uint32_t address);
	 
	 /**
	  * Get all memory bookmarks
	  * @return Map of addresses to bookmark info (name, description)
	  */
	 std::unordered_map<uint32_t, std::pair<std::string, std::string>> GetBookmarks() const;
	 
	 /**
	  * Add memory watch
	  * @param address Address to watch
	  * @param size Size to watch (1, 2, or 4 bytes)
	  * @param name Watch name
	  * @return Watch ID if successful, -1 otherwise
	  */
	 int AddWatch(uint32_t address, uint8_t size, const std::string& name = "");
	 
	 /**
	  * Remove memory watch
	  * @param watchId Watch ID
	  * @return True if successful
	  */
	 bool RemoveWatch(int watchId);
	 
	 /**
	  * Get memory watch values
	  * @return Map of watch IDs to current values
	  */
	 std::unordered_map<int, uint32_t> GetWatchValues() const;
	 
	 /**
	  * Format memory value as string
	  * @param address Address to format
	  * @param size Value size (1, 2, or 4 bytes)
	  * @param format Display format
	  * @return Formatted string
	  */
	 std::string FormatMemoryValue(uint32_t address, uint8_t size, 
								MemoryDisplayFormat format) const;
	 
	 /**
	  * Parse string as memory value
	  * @param text Text to parse
	  * @param size Value size (1, 2, or 4 bytes)
	  * @param format Display format
	  * @return Parsed value
	  */
	 uint32_t ParseMemoryValue(const std::string& text, uint8_t size, 
							 MemoryDisplayFormat format) const;
	 
	 /**
	  * Get last viewed memory address
	  * @return Last viewed address
	  */
	 uint32_t GetLastViewedAddress() const;
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to debugger
	 Debugger& m_debugger;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Last viewed memory address
	 uint32_t m_lastViewedAddress;
	 
	 // Memory bookmarks
	 std::unordered_map<uint32_t, std::pair<std::string, std::string>> m_bookmarks;
	 
	 // Memory watches
	 struct WatchInfo {
		 int id;
		 uint32_t address;
		 uint8_t size;
		 std::string name;
		 uint32_t lastValue;
	 };
	 std::vector<WatchInfo> m_watches;
	 int m_nextWatchId;
	 
	 // Memory change callbacks
	 std::unordered_map<int, std::function<void(uint32_t, uint32_t)>> m_changeCallbacks;
	 int m_nextCallbackId;
	 
	 // Previous search results
	 std::vector<SearchResult> m_lastSearchResults;
	 
	 /**
	  * Get ASCII representation of a byte
	  * @param byte Byte value
	  * @return ASCII character or '.' if not printable
	  */
	 char GetAsciiChar(uint8_t byte) const;
	 
	 /**
	  * Format address as string
	  * @param address Address to format
	  * @return Formatted address string
	  */
	 std::string FormatAddress(uint32_t address) const;
	 
	 /**
	  * Check if memory is editable
	  * @param address Address to check
	  * @param size Size of region
	  * @return True if region is editable
	  */
	 bool IsMemoryEditable(uint32_t address, uint32_t size) const;
	 
	 /**
	  * Format memory dump row
	  * @param address Row start address
	  * @param data Row data
	  * @param options Display options
	  * @return Formatted row string
	  */
	 std::string FormatMemoryRow(uint32_t address, const std::vector<uint8_t>& data,
							  const MemoryDumpOptions& options) const;
	 
	 /**
	  * Notify callbacks of memory change
	  * @param address Changed address
	  * @param size Size of changed region
	  */
	 void NotifyChangeCallbacks(uint32_t address, uint32_t size);
	 
	 /**
	  * Update watches with current values
	  */
	 void UpdateWatches();
	 
	 /**
	  * Find watch by ID
	  * @param watchId Watch ID
	  * @return Index in watches vector, or -1 if not found
	  */
	 int FindWatchIndex(int watchId) const;
 };
 
 } // namespace NiXX32