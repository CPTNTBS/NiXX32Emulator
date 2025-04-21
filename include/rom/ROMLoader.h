/**
 * ROMLoader.h
 * ROM loading and validation system for NiXX-32 arcade board emulation
 * 
 * This file defines the ROM loading system that handles loading, validation,
 * and management of ROM data for the NiXX-32 arcade system emulation.
 * It supports loading ROMs for both the original and enhanced hardware variants.
 */

 #pragma once

 #include <cstdint>
 #include <string>
 #include <vector>
 #include <unordered_map>
 #include <memory>
 #include <functional>
 
 #include "MemoryManager.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 
 /**
  * ROM file formats supported by the loader
  */
 enum class ROMFormat {
	 BIN,        // Raw binary format
	 ZIP,        // ZIP compressed format
	 GZIP,       // GZIP compressed format
	 CHD,        // MAME CHD format
	 UNKNOWN     // Unknown format
 };
 
 /**
  * ROM validation status
  */
 enum class ROMValidationStatus {
	 VALID,               // ROM is valid
	 INVALID_CHECKSUM,    // Checksum verification failed
	 INVALID_SIZE,        // ROM size is incorrect
	 INVALID_FORMAT,      // ROM format is invalid
	 MISSING_FILES,       // Required files are missing
	 WRONG_VERSION,       // ROM version is incompatible
	 UNKNOWN              // Unknown validation status
 };
 
 /**
  * ROM metadata structure
  */
 struct ROMInfo {
	 std::string name;             // ROM set name
	 std::string description;      // ROM description
	 std::string version;          // ROM version
	 std::string releaseDate;      // Release date
	 std::string manufacturer;     // Manufacturer
	 HardwareVariant variant;      // Hardware variant required
	 uint32_t totalSize;           // Total size in bytes
	 std::string checksum;         // Overall checksum
	 ROMValidationStatus status;   // Validation status
 };
 
 /**
  * Individual ROM file information
  */
 struct ROMFileInfo {
	 std::string filename;          // File name
	 uint32_t size;                 // Size in bytes
	 uint32_t crc32;               // CRC32 checksum
	 std::string md5;              // MD5 checksum
	 std::string sha1;             // SHA1 checksum
	 uint32_t loadAddress;         // Load address in memory
	 ROMFormat format;             // File format
	 bool required;                // Whether the file is required
	 std::string region;           // Memory region for this ROM
 };
 
 /**
  * ROM loading progress information
  */
 struct ROMLoadProgress {
	 std::string currentFile;      // Current file being processed
	 uint32_t bytesLoaded;         // Bytes loaded so far
	 uint32_t totalBytes;          // Total bytes to load
	 float percentage;             // Progress percentage (0-100)
	 ROMValidationStatus status;   // Current validation status
 };
 
 /**
  * Main class for ROM loading and management
  */
 class ROMLoader {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 ROMLoader(System& system, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~ROMLoader();
	 
	 /**
	  * Initialize the ROM loader
	  * @param romPath Default path for ROM files
	  * @return True if initialization was successful
	  */
	 bool Initialize(const std::string& romPath = "");
	 
	 /**
	  * Reset the ROM loader state
	  */
	 void Reset();
	 
	 /**
	  * Load ROM set from path
	  * @param path Path to ROM file or directory
	  * @param validateChecksum Whether to validate checksums
	  * @return True if loading was successful
	  */
	 bool LoadROM(const std::string& path, bool validateChecksum = true);
	 
	 /**
	  * Load ROM set from memory
	  * @param data ROM data buffer
	  * @param size Buffer size
	  * @param name ROM set name
	  * @param validateChecksum Whether to validate checksums
	  * @return True if loading was successful
	  */
	 bool LoadROMFromMemory(const uint8_t* data, size_t size, 
						   const std::string& name, 
						   bool validateChecksum = true);
	 
	 /**
	  * Validate a ROM set
	  * @param path Path to ROM file or directory
	  * @return Validation status
	  */
	 ROMValidationStatus ValidateROM(const std::string& path);
	 
	 /**
	  * Get information about a ROM set
	  * @param path Path to ROM file or directory
	  * @return ROM information
	  */
	 ROMInfo GetROMInfo(const std::string& path);
	 
	 /**
	  * Get information about the currently loaded ROM
	  * @return ROM information
	  */
	 ROMInfo GetLoadedROMInfo() const;
	 
	 /**
	  * Check if a ROM is currently loaded
	  * @return True if ROM is loaded
	  */
	 bool IsROMLoaded() const;
	 
	 /**
	  * Unload the current ROM
	  * @return True if successful
	  */
	 bool UnloadROM();
	 
	 /**
	  * Get the list of files in a ROM set
	  * @param path Path to ROM file or directory
	  * @return Vector of ROM file information
	  */
	 std::vector<ROMFileInfo> GetROMFiles(const std::string& path);
	 
	 /**
	  * Register a progress callback
	  * @param callback Function to call during ROM loading
	  * @return Callback ID
	  */
	 int RegisterProgressCallback(std::function<void(const ROMLoadProgress&)> callback);
	 
	 /**
	  * Remove a progress callback
	  * @param callbackId Callback ID
	  * @return True if successful
	  */
	 bool RemoveProgressCallback(int callbackId);
	 
	 /**
	  * Get the ROM database path
	  * @return ROM database path
	  */
	 std::string GetROMDBPath() const;
	 
	 /**
	  * Set the ROM database path
	  * @param path ROM database path
	  */
	 void SetROMDBPath(const std::string& path);
	 
	 /**
	  * Get the default ROM path
	  * @return Default ROM path
	  */
	 std::string GetDefaultROMPath() const;
	 
	 /**
	  * Set the default ROM path
	  * @param path Default ROM path
	  */
	 void SetDefaultROMPath(const std::string& path);
	 
	 /**
	  * Calculate checksum for ROM data
	  * @param data Data buffer
	  * @param size Buffer size
	  * @param type Checksum type (CRC32, MD5, SHA1)
	  * @return Checksum string
	  */
	 std::string CalculateChecksum(const uint8_t* data, size_t size, 
								const std::string& type = "CRC32");
	 
	 /**
	  * Get last error message
	  * @return Last error message
	  */
	 std::string GetLastError() const;
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Default ROM path
	 std::string m_defaultROMPath;
	 
	 // ROM database path
	 std::string m_romDBPath;
	 
	 // Currently loaded ROM information
	 ROMInfo m_loadedROMInfo;
	 
	 // Loaded ROM files
	 std::vector<ROMFileInfo> m_loadedROMFiles;
	 
	 // ROM database (ROM name -> expected files)
	 std::unordered_map<std::string, std::vector<ROMFileInfo>> m_romDatabase;
	 
	 // ROM loaded flag
	 bool m_romLoaded;
	 
	 // Last error message
	 std::string m_lastError;
	 
	 // Progress callbacks
	 std::unordered_map<int, std::function<void(const ROMLoadProgress&)>> m_progressCallbacks;
	 int m_nextCallbackId;
	 
	 /**
	  * Detect ROM format from data
	  * @param data Data buffer
	  * @param size Buffer size
	  * @return Detected format
	  */
	 ROMFormat DetectFormat(const uint8_t* data, size_t size);
	 
	 /**
	  * Detect ROM format from file
	  * @param path File path
	  * @return Detected format
	  */
	 ROMFormat DetectFormatFromFile(const std::string& path);
	 
	 /**
	  * Extract files from compressed ROM
	  * @param data Compressed data buffer
	  * @param size Buffer size
	  * @param format Compression format
	  * @return Map of filenames to data buffers
	  */
	 std::unordered_map<std::string, std::vector<uint8_t>> ExtractFiles(
		 const uint8_t* data, size_t size, ROMFormat format);
	 
	 /**
	  * Load ROM database
	  * @param path Database file path
	  * @return True if successful
	  */
	 bool LoadROMDatabase(const std::string& path);
	 
	 /**
	  * Validate ROM files against database
	  * @param files ROM files
	  * @param romName ROM set name
	  * @param validateChecksum Whether to validate checksums
	  * @return Validation status
	  */
	 ROMValidationStatus ValidateROMFiles(
		 const std::unordered_map<std::string, std::vector<uint8_t>>& files, 
		 const std::string& romName, 
		 bool validateChecksum);
	 
	 /**
	  * Load ROM data into memory
	  * @param files ROM files
	  * @param romName ROM set name
	  * @return True if successful
	  */
	 bool LoadROMToMemory(
		 const std::unordered_map<std::string, std::vector<uint8_t>>& files,
		 const std::string& romName);
	 
	 /**
	  * Calculate CRC32 checksum
	  * @param data Data buffer
	  * @param size Buffer size
	  * @return CRC32 checksum
	  */
	 uint32_t CalculateCRC32(const uint8_t* data, size_t size);
	 
	 /**
	  * Calculate MD5 checksum
	  * @param data Data buffer
	  * @param size Buffer size
	  * @return MD5 checksum string
	  */
	 std::string CalculateMD5(const uint8_t* data, size_t size);
	 
	 /**
	  * Calculate SHA1 checksum
	  * @param data Data buffer
	  * @param size Buffer size
	  * @return SHA1 checksum string
	  */
	 std::string CalculateSHA1(const uint8_t* data, size_t size);
	 
	 /**
	  * Notify progress callbacks
	  * @param progress Progress information
	  */
	 void NotifyProgressCallbacks(const ROMLoadProgress& progress);
	 
	 /**
	  * Update load progress
	  * @param currentFile Current file being processed
	  * @param bytesLoaded Bytes loaded so far
	  * @param totalBytes Total bytes to load
	  * @param status Current validation status
	  */
	 void UpdateProgress(const std::string& currentFile, uint32_t bytesLoaded,
					   uint32_t totalBytes, ROMValidationStatus status);
	 
	 /**
	  * Set error message
	  * @param error Error message
	  */
	 void SetError(const std::string& error);
 };
 
 } // namespace NiXX32