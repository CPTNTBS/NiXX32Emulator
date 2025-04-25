/**
 * ROMLoader.cpp
 * Implementation of ROM loading and validation system for NiXX-32 arcade board emulation
 */

 #include "ROMLoader.h"
 #include "NiXX32System.h"
 #include "MemoryManager.h"
 #include "Logger.h"
 #include <fstream>
 #include <sstream>
 #include <algorithm>
 #include <cstring>
 #include <filesystem>
 #include <unordered_set>
 #include <iomanip>
 #include <functional>
 
 // Include compression library headers if available
 #ifdef HAVE_ZLIB
 #include <zlib.h>
 #endif
 
 #ifdef HAVE_LIBZIP
 #include <zip.h>
 #endif
 
 namespace NiXX32 {
 
 // CRC32 calculation table
 static const uint32_t crc32_table[256] = {
	 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	 0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	 0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
 };
 
 ROMLoader::ROMLoader(System& system, MemoryManager& memoryManager, Logger& logger)
	 : m_system(system),
	   m_memoryManager(memoryManager),
	   m_logger(logger),
	   m_defaultROMPath("roms"),
	   m_romDBPath("romdb.json"),
	   m_romLoaded(false),
	   m_nextCallbackId(1) {
	 
	 m_logger.Info("ROMLoader", "Initializing ROM loading system");
 }
 
 ROMLoader::~ROMLoader() {
	 m_logger.Info("ROMLoader", "Shutting down ROM loading system");
 }
 
 bool ROMLoader::Initialize(const std::string& romPath) {
	 m_logger.Info("ROMLoader", "Initializing ROM loader");
	 
	 if (!romPath.empty()) {
		 m_defaultROMPath = romPath;
	 }
	 
	 m_logger.Info("ROMLoader", "ROM path set to: " + m_defaultROMPath);
	 
	 // Create directories if they don't exist
	 std::filesystem::path dir(m_defaultROMPath);
	 try {
		 if (!std::filesystem::exists(dir)) {
			 std::filesystem::create_directories(dir);
			 m_logger.Info("ROMLoader", "Created ROM directory: " + m_defaultROMPath);
		 }
	 } catch (const std::exception& e) {
		 m_logger.Warning("ROMLoader", "Could not create ROM directory: " + std::string(e.what()));
	 }
	 
	 // Load ROM database
	 bool dbLoaded = LoadROMDatabase(m_romDBPath);
	 if (!dbLoaded) {
		 m_logger.Warning("ROMLoader", "ROM database not loaded. ROM validation may be limited.");
	 }
	 
	 m_logger.Info("ROMLoader", "ROM loader initialized successfully");
	 return true;
 }
 
 void ROMLoader::Reset() {
	 m_logger.Info("ROMLoader", "Resetting ROM loader");
	 
	 // Unload any loaded ROM
	 UnloadROM();
	 
	 m_logger.Info("ROMLoader", "ROM loader reset complete");
 }
 
 bool ROMLoader::LoadROM(const std::string& path, bool validateChecksum) {
	 m_logger.Info("ROMLoader", "Loading ROM from: " + path);
	 
	 // Update progress
	 UpdateProgress(path, 0, 100, ROMValidationStatus::UNKNOWN);
	 
	 // First, check if file exists
	 std::ifstream file(path, std::ios::binary);
	 if (!file.is_open()) {
		 SetError("Could not open ROM file: " + path);
		 UpdateProgress(path, 0, 100, ROMValidationStatus::MISSING_FILES);
		 return false;
	 }
	 
	 // Read file into memory
	 file.seekg(0, std::ios::end);
	 size_t fileSize = file.tellg();
	 file.seekg(0, std::ios::beg);
	 
	 std::vector<uint8_t> fileData(fileSize);
	 file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
	 file.close();
	 
	 UpdateProgress(path, fileSize, fileSize, ROMValidationStatus::UNKNOWN);
	 
	 // Detect format
	 ROMFormat format = DetectFormat(fileData.data(), fileSize);
	 
	 // Get ROM name from path
	 std::filesystem::path filePath(path);
	 std::string romName = filePath.stem().string();
	 
	 // Extract files if compressed
	 std::unordered_map<std::string, std::vector<uint8_t>> files;
	 if (format == ROMFormat::BIN) {
		 // Single file, add to files map
		 files[filePath.filename().string()] = fileData;
	 } else {
		 // Extract files from compressed ROM
		 files = ExtractFiles(fileData.data(), fileSize, format);
		 
		 if (files.empty()) {
			 SetError("Failed to extract files from compressed ROM");
			 UpdateProgress(path, fileSize, fileSize, ROMValidationStatus::INVALID_FORMAT);
			 return false;
		 }
		 
		 m_logger.Info("ROMLoader", "Extracted " + std::to_string(files.size()) + " files from " + path);
	 }
	 
	 // Validate ROM against database
	 ROMValidationStatus status = ValidateROMFiles(files, romName, validateChecksum);
	 
	 // Update progress
	 UpdateProgress(path, fileSize, fileSize, status);
	 
	 // If validation failed, return false
	 if (status != ROMValidationStatus::VALID) {
		 SetError("ROM validation failed: " + std::to_string(static_cast<int>(status)));
		 return false;
	 }
	 
	 // Load ROM data into memory
	 if (!LoadROMToMemory(files, romName)) {
		 SetError("Failed to load ROM data into memory");
		 return false;
	 }
	 
	 // Set ROM loaded flag
	 m_romLoaded = true;
	 
	 // Update ROM info
	 m_loadedROMInfo.name = romName;
	 m_loadedROMInfo.status = status;
	 m_loadedROMInfo.totalSize = 0;
	 
	 for (const auto& file : files) {
		 m_loadedROMInfo.totalSize += file.second.size();
	 }
	 
	 m_logger.Info("ROMLoader", "ROM loaded successfully: " + romName);
	 return true;
 }
 
 bool ROMLoader::LoadROMFromMemory(const uint8_t* data, size_t size, 
								  const std::string& name, 
								  bool validateChecksum) {
	 m_logger.Info("ROMLoader", "Loading ROM from memory: " + name);
	 
	 // Update progress
	 UpdateProgress(name, 0, size, ROMValidationStatus::UNKNOWN);
	 
	 // Detect format
	 ROMFormat format = DetectFormat(data, size);
	 
	 // Extract files if compressed
	 std::unordered_map<std::string, std::vector<uint8_t>> files;
	 if (format == ROMFormat::BIN) {
		 // Single file, add to files map
		 std::vector<uint8_t> fileData(data, data + size);
		 files[name] = fileData;
	 } else {
		 // Extract files from compressed ROM
		 files = ExtractFiles(data, size, format);
		 
		 if (files.empty()) {
			 SetError("Failed to extract files from compressed ROM");
			 UpdateProgress(name, size, size, ROMValidationStatus::INVALID_FORMAT);
			 return false;
		 }
		 
		 m_logger.Info("ROMLoader", "Extracted " + std::to_string(files.size()) + " files from memory");
	 }
	 
	 // Validate ROM against database
	 ROMValidationStatus status = ValidateROMFiles(files, name, validateChecksum);
	 
	 // Update progress
	 UpdateProgress(name, size, size, status);
	 
	 // If validation failed, return false
	 if (status != ROMValidationStatus::VALID) {
		 SetError("ROM validation failed: " + std::to_string(static_cast<int>(status)));
		 return false;
	 }
	 
	 // Load ROM data into memory
	 if (!LoadROMToMemory(files, name)) {
		 SetError("Failed to load ROM data into memory");
		 return false;
	 }
	 
	 // Set ROM loaded flag
	 m_romLoaded = true;
	 
	 // Update ROM info
	 m_loadedROMInfo.name = name;
	 m_loadedROMInfo.status = status;
	 m_loadedROMInfo.totalSize = 0;
	 
	 for (const auto& file : files) {
		 m_loadedROMInfo.totalSize += file.second.size();
	 }
	 
	 m_logger.Info("ROMLoader", "ROM loaded successfully: " + name);
	 return true;
 }
 
 ROMValidationStatus ROMLoader::ValidateROM(const std::string& path) {
	 m_logger.Info("ROMLoader", "Validating ROM: " + path);
	 
	 // First, check if file exists
	 std::ifstream file(path, std::ios::binary);
	 if (!file.is_open()) {
		 m_logger.Error("ROMLoader", "Could not open ROM file: " + path);
		 return ROMValidationStatus::MISSING_FILES;
	 }
	 
	 // Read file into memory
	 file.seekg(0, std::ios::end);
	 size_t fileSize = file.tellg();
	 file.seekg(0, std::ios::beg);
	 
	 std::vector<uint8_t> fileData(fileSize);
	 file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
	 file.close();
	 
	 // Detect format
	 ROMFormat format = DetectFormat(fileData.data(), fileSize);
	 
	 // Get ROM name from path
	 std::filesystem::path filePath(path);
	 std::string romName = filePath.stem().string();
	 
	 // Extract files if compressed
	 std::unordered_map<std::string, std::vector<uint8_t>> files;
	 if (format == ROMFormat::BIN) {
		 // Single file, add to files map
		 files[filePath.filename().string()] = fileData;
	 } else {
		 // Extract files from compressed ROM
		 files = ExtractFiles(fileData.data(), fileSize, format);
		 
		 if (files.empty()) {
			 m_logger.Error("ROMLoader", "Failed to extract files from compressed ROM");
			 return ROMValidationStatus::INVALID_FORMAT;
		 }
	 }
	 
	 // Validate ROM against database
	 ROMValidationStatus status = ValidateROMFiles(files, romName, true);
	 
	 m_logger.Info("ROMLoader", "ROM validation result: " + std::to_string(static_cast<int>(status)));
	 return status;
 }
 
 ROMInfo ROMLoader::GetROMInfo(const std::string& path) {
	 ROMInfo info;
	 
	 // Get ROM name from path
	 std::filesystem::path filePath(path);
	 info.name = filePath.stem().string();
	 info.description = ""; // Default to empty
	 info.version = "";     // Default to empty
	 info.releaseDate = ""; // Default to empty
	 info.manufacturer = ""; // Default to empty
	 info.variant = HardwareVariant::NIXX32_ORIGINAL; // Default to original hardware
	 info.totalSize = 0;    // Will be calculated
	 info.checksum = "";    // Will be calculated
	 info.status = ROMValidationStatus::UNKNOWN;
	 
	 // Check if file exists
	 std::ifstream file(path, std::ios::binary);
	 if (!file.is_open()) {
		 info.status = ROMValidationStatus::MISSING_FILES;
		 return info;
	 }
	 
	 // Read file into memory
	 file.seekg(0, std::ios::end);
	 size_t fileSize = file.tellg();
	 file.seekg(0, std::ios::beg);
	 
	 std::vector<uint8_t> fileData(fileSize);
	 file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
	 file.close();
	 
	 // Set total size
	 info.totalSize = fileSize;
	 
	 // Calculate checksum
	 info.checksum = CalculateChecksum(fileData.data(), fileSize, "CRC32");
	 
	 // Detect format
	 ROMFormat format = DetectFormat(fileData.data(), fileSize);
	 
	 // Extract files if compressed
	 std::unordered_map<std::string, std::vector<uint8_t>> files;
	 if (format == ROMFormat::BIN) {
		 // Single file, add to files map
		 files[filePath.filename().string()] = fileData;
	 } else {
		 // Extract files from compressed ROM
		 files = ExtractFiles(fileData.data(), fileSize, format);
		 
		 if (files.empty()) {
			 info.status = ROMValidationStatus::INVALID_FORMAT;
			 return info;
		 }
		 
		 // Recalculate total size
		 info.totalSize = 0;
		 for (const auto& file : files) {
			 info.totalSize += file.second.size();
		 }
	 }
	 
	 // Check if ROM is in database
	 auto it = m_romDatabase.find(info.name);
	 if (it != m_romDatabase.end()) {
		 // Found in database, validate against expected files
		 bool missingFiles = false;
		 bool wrongSize = false;
		 bool wrongChecksum = false;
		 
		 // Check all required files exist
		 for (const auto& expectedFile : it->second) {
			 auto fileIt = files.find(expectedFile.filename);
			 if (fileIt == files.end()) {
				 missingFiles = true;
				 break;
			 }
			 
			 // Check size
			 if (fileIt->second.size() != expectedFile.size) {
				 wrongSize = true;
				 break;
			 }
			 
			 // Check CRC32
			 uint32_t crc = CalculateCRC32(fileIt->second.data(), fileIt->second.size());
			 if (crc != expectedFile.crc32) {
				 wrongChecksum = true;
				 break;
			 }
		 }
		 
		 // Set status based on validation results
		 if (missingFiles) {
			 info.status = ROMValidationStatus::MISSING_FILES;
		 } else if (wrongSize) {
			 info.status = ROMValidationStatus::INVALID_SIZE;
		 } else if (wrongChecksum) {
			 info.status = ROMValidationStatus::INVALID_CHECKSUM;
		 } else {
			 info.status = ROMValidationStatus::VALID;
		 }
		 
		 // If valid, set additional info from database
		 if (info.status == ROMValidationStatus::VALID) {
			 // Fill in missing info from database if available
			 // In a real implementation, this would pull from a structured database
			 // Here we're just using placeholder logic
			 info.description = "Telefunk Arcade Game";
			 info.version = "1.0";
			 info.releaseDate = "1989";
			 info.manufacturer = "NiXX Systems";
			 
			 // Determine hardware variant from ROM files
			 bool isEnhancedHardware = false;
			 
			 // Logic to determine if ROM is for enhanced hardware
			 // (In real implementation, this would be based on database info)
			 for (const auto& file : files) {
				 // Example: check for specific enhanced hardware marker in ROM
				 if (file.second.size() > 16) {
					 // Check for "NIXX32+" string at some known offset
					 const char* enhancedMarker = "NIXX32+";
					 if (std::search(file.second.begin(), file.second.end(), 
									 enhancedMarker, enhancedMarker + 7) != file.second.end()) {
						 isEnhancedHardware = true;
						 break;
					 }
				 }
			 }
			 
			 info.variant = isEnhancedHardware ? 
						  HardwareVariant::NIXX32_PLUS : 
						  HardwareVariant::NIXX32_ORIGINAL;
		 }
	 } else {
		 // Not in database, basic validation
		 // Check ROM header for validity (simplified)
		 bool validHeader = false;
		 
		 // Logic to check ROM header validity
		 // (In real implementation, this would examine ROM headers for validity)
		 for (const auto& file : files) {
			 if (file.second.size() > 16) {
				 // Example: check for "NIXX" signature at the beginning of ROM
				 if (file.second[0] == 'N' && file.second[1] == 'I' && 
					 file.second[2] == 'X' && file.second[3] == 'X') {
					 validHeader = true;
					 break;
				 }
			 }
		 }
		 
		 info.status = validHeader ? 
					 ROMValidationStatus::VALID : 
					 ROMValidationStatus::INVALID_FORMAT;
	 }
	 
	 return info;
 }
 
 ROMInfo ROMLoader::GetLoadedROMInfo() const {
	 return m_loadedROMInfo;
 }
 
 bool ROMLoader::IsROMLoaded() const {
	 return m_romLoaded;
 }
 
 bool ROMLoader::UnloadROM() {
	 if (!m_romLoaded) {
		 return true; // Nothing to unload
	 }
	 
	 m_logger.Info("ROMLoader", "Unloading ROM: " + m_loadedROMInfo.name);
	 
	 // Clear loaded ROM info
	 m_loadedROMInfo = ROMInfo();
	 
	 // Clear loaded ROM files
	 m_loadedROMFiles.clear();
	 
	 // Set ROM loaded flag to false
	 m_romLoaded = false;
	 
	 m_logger.Info("ROMLoader", "ROM unloaded successfully");
	 return true;
 }
 
 std::vector<ROMFileInfo> ROMLoader::GetROMFiles(const std::string& path) {
	 std::vector<ROMFileInfo> files;
	 
	 // Check if file exists
	 std::ifstream file(path, std::ios::binary);
	 if (!file.is_open()) {
		 m_logger.Error("ROMLoader", "Could not open ROM file: " + path);
		 return files;
	 }
	 
	 // Read file into memory
	 file.seekg(0, std::ios::end);
	 size_t fileSize = file.tellg();
	 file.seekg(0, std::ios::beg);
	 
	 std::vector<uint8_t> fileData(fileSize);
	 file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
	 file.close();
	 
	 // Detect format
	 ROMFormat format = DetectFormat(fileData.data(), fileSize);
	 
	 // Extract files if compressed
	 std::unordered_map<std::string, std::vector<uint8_t>> extractedFiles;
	 if (format == ROMFormat::BIN) {
		 // Single file, add to files map
		 std::filesystem::path filePath(path);
		 extractedFiles[filePath.filename().string()] = fileData;
	 } else {
		 // Extract files from compressed ROM
		 extractedFiles = ExtractFiles(fileData.data(), fileSize, format);
		 
		 if (extractedFiles.empty()) {
			 m_logger.Error("ROMLoader", "Failed to extract files from compressed ROM");
			 return files;
		 }
	 }
	 
	 // Get ROM name from path
	 std::filesystem::path filePath(path);
	 std::string romName = filePath.stem().string();
	 
	 // Check if ROM is in database
	 bool inDatabase = m_romDatabase.find(romName) != m_romDatabase.end();
	 
	 // Create ROMFileInfo for each file
	 uint32_t baseAddress = 0;
	 for (const auto& extractedFile : extractedFiles) {
		 ROMFileInfo fileInfo;
		 fileInfo.filename = extractedFile.first;
		 fileInfo.size = extractedFile.second.size();
		 fileInfo.crc32 = CalculateCRC32(extractedFile.second.data(), extractedFile.second.size());
		 fileInfo.md5 = CalculateMD5(extractedFile.second.data(), extractedFile.second.size());
		 fileInfo.sha1 = CalculateSHA1(extractedFile.second.data(), extractedFile.second.size());
		 fileInfo.loadAddress = baseAddress;
		 fileInfo.format = format;
		 fileInfo.required = true; // Assume all files are required
		 fileInfo.region = "ROM";  // Default region
		 
		 // Determine if file is required and get load address from database
		 if (inDatabase) {
			 auto& dbFiles = m_romDatabase[romName];
			 for (const auto& dbFile : dbFiles) {
				 if (dbFile.filename == fileInfo.filename && dbFile.crc32 == fileInfo.crc32) {
					 fileInfo.loadAddress = dbFile.loadAddress;
					 fileInfo.region = dbFile.region;
					 break;
				 }
			 }
		 } else {
			 // If not in database, use heuristics to determine load address
			 // For example, assign sequential addresses based on file order
			 baseAddress += fileInfo.size;
		 }
		 
		 files.push_back(fileInfo);
	 }
	 
	 return files;
 }
 
 int ROMLoader::RegisterProgressCallback(std::function<void(const ROMLoadProgress&)> callback) {
	 int callbackId = m_nextCallbackId++;
	 m_progressCallbacks[callbackId] = callback;
	 return callbackId;
 }
 
 bool ROMLoader::RemoveProgressCallback(int callbackId) {
	 auto it = m_progressCallbacks.find(callbackId);
	 if (it == m_progressCallbacks.end()) {
		 return false;
	 }
	 
	 m_progressCallbacks.erase(it);
	 return true;
 }
 
 std::string ROMLoader::GetROMDBPath() const {
	 return m_romDBPath;
 }
 
 void ROMLoader::SetROMDBPath(const std::string& path) {
	 m_romDBPath = path;
	 
	 // Try to load the new database
	 if (LoadROMDatabase(m_romDBPath)) {
		 m_logger.Info("ROMLoader", "ROM database loaded from new path: " + m_romDBPath);
	 } else {
		 m_logger.Warning("ROMLoader", "Failed to load ROM database from new path: " + m_romDBPath);
	 }
 }
 
 std::string ROMLoader::GetDefaultROMPath() const {
	 return m_defaultROMPath;
 }
 
 void ROMLoader::SetDefaultROMPath(const std::string& path) {
	 m_defaultROMPath = path;
	 m_logger.Info("ROMLoader", "Default ROM path set to: " + m_defaultROMPath);
 }
 
 std::string ROMLoader::CalculateChecksum(const uint8_t* data, size_t size, 
										const std::string& type) {
	 if (type == "CRC32") {
		 uint32_t crc = CalculateCRC32(data, size);
		 std::stringstream ss;
		 ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << crc;
		 return ss.str();
	 } else if (type == "MD5") {
		 return CalculateMD5(data, size);
	 } else if (type == "SHA1") {
		 return CalculateSHA1(data, size);
	 } else {
		 m_logger.Warning("ROMLoader", "Unknown checksum type: " + type);
		 return "";
	 }
 }
 
 std::string ROMLoader::GetLastError() const {
	 return m_lastError;
 }
 
 ROMFormat ROMLoader::DetectFormat(const uint8_t* data, size_t size) {
	 if (size < 4) {
		 return ROMFormat::UNKNOWN;
	 }
	 
	 // Check for ZIP format
	 if (data[0] == 0x50 && data[1] == 0x4B && data[2] == 0x03 && data[3] == 0x04) {
		 return ROMFormat::ZIP;
	 }
	 
	 // Check for GZIP format
	 if (data[0] == 0x1F && data[1] == 0x8B) {
		 return ROMFormat::GZIP;
	 }
	 
	 // Check for CHD format
	 if (memcmp(data, "MComprHD", 8) == 0) {
		 return ROMFormat::CHD;
	 }
	 
	 // Default to binary
	 return ROMFormat::BIN;
 }
 
 ROMFormat ROMLoader::DetectFormatFromFile(const std::string& path) {
	 // Open file and read header
	 std::ifstream file(path, std::ios::binary);
	 if (!file.is_open()) {
		 return ROMFormat::UNKNOWN;
	 }
	 
	 char header[8] = {0};
	 file.read(header, 8);
	 file.close();
	 
	 if (!file) {
		 return ROMFormat::UNKNOWN;
	 }
	 
	 return DetectFormat(reinterpret_cast<const uint8_t*>(header), 8);
 }
 
 std::unordered_map<std::string, std::vector<uint8_t>> ROMLoader::ExtractFiles(
	 const uint8_t* data, size_t size, ROMFormat format) {
	 
	 std::unordered_map<std::string, std::vector<uint8_t>> files;
	 
	 switch (format) {
		 case ROMFormat::BIN:
			 // Binary format, just copy data as single file
			 {
				 std::vector<uint8_t> fileData(data, data + size);
				 files["rom.bin"] = fileData;
			 }
			 break;
			 
		 case ROMFormat::ZIP:
			 // Extract from ZIP archive
			 #ifdef HAVE_LIBZIP
			 {
				 // Use libzip to extract files
				 zip_error_t error;
				 zip_source_t* src = zip_source_buffer_create(data, size, 0, &error);
				 if (src == nullptr) {
					 m_logger.Error("ROMLoader", "Failed to create ZIP source");
					 return files;
				 }
				 
				 zip_t* archive = zip_open_from_source(src, 0, &error);
				 if (archive == nullptr) {
					 zip_source_free(src);
					 m_logger.Error("ROMLoader", "Failed to open ZIP archive");
					 return files;
				 }
				 
				 zip_int64_t numEntries = zip_get_num_entries(archive, 0);
				 for (zip_int64_t i = 0; i < numEntries; i++) {
					 const char* name = zip_get_name(archive, i, 0);
					 if (name == nullptr) {
						 continue;
					 }
					 
					 // Skip directories
					 if (name[strlen(name) - 1] == '/') {
						 continue;
					 }
					 
					 // Open file in archive
					 zip_file_t* file = zip_fopen_index(archive, i, 0);
					 if (file == nullptr) {
						 continue;
					 }
					 
					 // Get file size
					 zip_stat_t stat;
					 if (zip_stat_index(archive, i, 0, &stat) != 0 || !(stat.valid & ZIP_STAT_SIZE)) {
						 zip_fclose(file);
						 continue;
					 }
					 
					 // Read file data
					 std::vector<uint8_t> fileData(stat.size);
					 zip_int64_t bytesRead = zip_fread(file, fileData.data(), stat.size);
					 zip_fclose(file);
					 
					 if (bytesRead > 0) {
						 // Add file to results
						 files[name] = fileData;
					 }
				 }
				 
				 zip_close(archive);
			 }
			 #else
			 // Simplified ZIP extraction without libzip
			 {
				 m_logger.Warning("ROMLoader", "ZIP extraction not fully supported without libzip");
				 
				 // Very basic ZIP parsing to extract files
				 // Find local file headers
				 for (size_t i = 0; i < size - 30; ++i) {
					 if (data[i] == 0x50 && data[i + 1] == 0x4B && data[i + 2] == 0x03 && data[i + 3] == 0x04) {
						 // Found a local file header
						 uint16_t nameLen = data[i + 26] | (data[i + 27] << 8);
						 uint16_t extraLen = data[i + 28] | (data[i + 29] << 8);
						 uint32_t compSize = data[i + 18] | (data[i + 19] << 8) | 
										   (data[i + 20] << 16) | (data[i + 21] << 24);
						 uint32_t uncompSize = data[i + 22] | (data[i + 23] << 8) | 
											 (data[i + 24] << 16) | (data[i + 25] << 24);
						 uint16_t method = data[i + 8] | (data[i + 9] << 8);
						 
						 // Extract filename
						 std::string filename(reinterpret_cast<const char*>(&data[i + 30]), nameLen);
						 
						 // Skip directories
						 if (filename.empty() || filename.back() == '/') {
							 continue;
						 }
						 
						 // Data starts after header
						 size_t dataOffset = i + 30 + nameLen + extraLen;
						 
						 if (method == 0) {
							 // Store method, no compression
							 std::vector<uint8_t> fileData(&data[dataOffset], &data[dataOffset + uncompSize]);
							 files[filename] = fileData;
						 } else {
							 // Compression method not supported without zlib
							 m_logger.Warning("ROMLoader", "Compressed file not extracted (zlib not available): " + filename);
						 }
					 }
				 }
			 }
			 #endif
			 break;
			 
		 case ROMFormat::GZIP:
			 // Extract from GZIP archive
			 #ifdef HAVE_ZLIB
			 {
				 // Use zlib to decompress GZIP data
				 z_stream strm;
				 memset(&strm, 0, sizeof(strm));
				 
				 if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
					 m_logger.Error("ROMLoader", "Failed to initialize zlib");
					 return files;
				 }
				 
				 // Set input data
				 strm.next_in = const_cast<Bytef*>(data);
				 strm.avail_in = size;
				 
				 // Allocate output buffer (start with 2x input size)
				 std::vector<uint8_t> output(size * 2);
				 strm.next_out = output.data();
				 strm.avail_out = output.size();
				 
				 // Decompress
				 int result = inflate(&strm, Z_FINISH);
				 while (result == Z_OK || result == Z_BUF_ERROR) {
					 // Need more output space
					 size_t oldSize = output.size();
					 output.resize(oldSize * 2);
					 strm.next_out = output.data() + oldSize - strm.avail_out;
					 strm.avail_out += oldSize;
					 
					 result = inflate(&strm, Z_FINISH);
				 }
				 
				 inflateEnd(&strm);
				 
				 if (result == Z_STREAM_END) {
					 // Trim output buffer to actual decompressed size
					 output.resize(output.size() - strm.avail_out);
					 
					 // Add as single file
					 files["rom.bin"] = output;
				 } else {
					 m_logger.Error("ROMLoader", "Failed to decompress GZIP data");
				 }
			 }
			 #else
			 m_logger.Error("ROMLoader", "GZIP extraction not supported without zlib");
			 #endif
			 break;
			 
		 case ROMFormat::CHD:
			 // CHD extraction not implemented
			 m_logger.Error("ROMLoader", "CHD extraction not implemented");
			 break;
			 
		 default:
			 m_logger.Error("ROMLoader", "Unknown format for extraction");
			 break;
	 }
	 
	 return files;
 }
 
 bool ROMLoader::LoadROMDatabase(const std::string& path) {
	 m_logger.Info("ROMLoader", "Loading ROM database from: " + path);
	 
	 // Check if file exists
	 std::ifstream file(path);
	 if (!file.is_open()) {
		 m_logger.Warning("ROMLoader", "ROM database file not found: " + path);
		 return false;
	 }
	 
	 // Clear existing database
	 m_romDatabase.clear();
	 
	 // Parse JSON database (simplified)
	 std::string line;
	 std::string currentROM;
	 
	 while (std::getline(file, line)) {
		 // Trim whitespace
		 line.erase(0, line.find_first_not_of(" \t\r\n"));
		 line.erase(line.find_last_not_of(" \t\r\n") + 1);
		 
		 if (line.empty() || line[0] == '#') {
			 continue; // Skip empty lines and comments
		 }
		 
		 if (line[0] == '[' && line.back() == ']') {
			 // ROM name
			 currentROM = line.substr(1, line.size() - 2);
		 } else if (!currentROM.empty() && line.find('=') != std::string::npos) {
			 // ROM file info
			 size_t equalPos = line.find('=');
			 std::string key = line.substr(0, equalPos);
			 std::string value = line.substr(equalPos + 1);
			 
			 // Trim key and value
			 key.erase(0, key.find_first_not_of(" \t"));
			 key.erase(key.find_last_not_of(" \t") + 1);
			 value.erase(0, value.find_first_not_of(" \t"));
			 value.erase(value.find_last_not_of(" \t") + 1);
			 
			 // Parse file info
			 if (key == "file") {
				 // Format: filename,size,crc32,region,address
				 std::vector<std::string> parts;
				 std::stringstream ss(value);
				 std::string part;
				 
				 while (std::getline(ss, part, ',')) {
					 // Trim part
					 part.erase(0, part.find_first_not_of(" \t"));
					 part.erase(part.find_last_not_of(" \t") + 1);
					 parts.push_back(part);
				 }
				 
				 if (parts.size() >= 3) {
					 ROMFileInfo fileInfo;
					 fileInfo.filename = parts[0];
					 fileInfo.size = std::stoul(parts[1]);
					 
					 // Parse CRC32 from hex string
					 std::stringstream crcss(parts[2]);
					 crcss >> std::hex >> fileInfo.crc32;
					 
					 // Parse region
					 fileInfo.region = (parts.size() > 3) ? parts[3] : "ROM";
					 
					 // Parse load address
					 if (parts.size() > 4) {
						 std::stringstream addrss(parts[4]);
						 addrss >> std::hex >> fileInfo.loadAddress;
					 } else {
						 fileInfo.loadAddress = 0;
					 }
					 
					 fileInfo.required = true;
					 fileInfo.format = ROMFormat::BIN;
					 
					 m_romDatabase[currentROM].push_back(fileInfo);
				 }
			 }
		 }
	 }
	 
	 m_logger.Info("ROMLoader", "Loaded " + std::to_string(m_romDatabase.size()) + " ROM entries");
	 return !m_romDatabase.empty();
 }
 
 ROMValidationStatus ROMLoader::ValidateROMFiles(
	 const std::unordered_map<std::string, std::vector<uint8_t>>& files, 
	 const std::string& romName, 
	 bool validateChecksum) {
	 
	 m_logger.Info("ROMLoader", "Validating ROM files for: " + romName);
	 
	 // Check if ROM is in database
	 auto it = m_romDatabase.find(romName);
	 if (it != m_romDatabase.end()) {
		 // Found in database, validate against expected files
		 bool missingFiles = false;
		 bool wrongSize = false;
		 bool wrongChecksum = false;
		 
		 // Check all required files exist
		 for (const auto& expectedFile : it->second) {
			 if (!expectedFile.required) {
				 continue; // Skip optional files
			 }
			 
			 auto fileIt = files.find(expectedFile.filename);
			 if (fileIt == files.end()) {
				 m_logger.Warning("ROMLoader", "Missing required file: " + expectedFile.filename);
				 missingFiles = true;
				 break;
			 }
			 
			 // Check size
			 if (fileIt->second.size() != expectedFile.size) {
				 m_logger.Warning("ROMLoader", "File size mismatch for " + expectedFile.filename + 
							   ": expected " + std::to_string(expectedFile.size) + 
							   ", got " + std::to_string(fileIt->second.size()));
				 wrongSize = true;
				 break;
			 }
			 
			 // Check checksum if requested
			 if (validateChecksum) {
				 uint32_t crc = CalculateCRC32(fileIt->second.data(), fileIt->second.size());
				 if (crc != expectedFile.crc32) {
					 m_logger.Warning("ROMLoader", "Checksum mismatch for " + expectedFile.filename);
					 wrongChecksum = true;
					 break;
				 }
			 }
		 }
		 
		 // Set validation status
		 if (missingFiles) {
			 return ROMValidationStatus::MISSING_FILES;
		 } else if (wrongSize) {
			 return ROMValidationStatus::INVALID_SIZE;
		 } else if (wrongChecksum) {
			 return ROMValidationStatus::INVALID_CHECKSUM;
		 } else {
			 return ROMValidationStatus::VALID;
		 }
	 } else {
		 // Not in database, perform basic validation
		 m_logger.Info("ROMLoader", "ROM not found in database, performing basic validation");
		 
		 // Check if any files exist
		 if (files.empty()) {
			 return ROMValidationStatus::MISSING_FILES;
		 }
		 
		 // Check minimum file size (basic sanity check)
		 bool hasSufficientSize = false;
		 for (const auto& file : files) {
			 if (file.second.size() >= 256) { // Arbitrary minimum size
				 hasSufficientSize = true;
				 break;
			 }
		 }
		 
		 if (!hasSufficientSize) {
			 return ROMValidationStatus::INVALID_SIZE;
		 }
		 
		 // Check for ROM header validity
		 bool validHeader = false;
		 for (const auto& file : files) {
			 if (file.second.size() > 16) {
				 // Simple validation: check for "NIXX" marker at start of ROM
				 // In a real implementation, this would be a more sophisticated check
				 const char* marker = "NIXX";
				 if (std::search(file.second.begin(), file.second.begin() + 16, 
							   marker, marker + 4) != file.second.begin() + 16) {
					 validHeader = true;
					 break;
				 }
			 }
		 }
		 
		 // If no valid header found, check file extensions and names
		 if (!validHeader) {
			 std::unordered_set<std::string> validExtensions = {".bin", ".rom", ".32x"};
			 
			 for (const auto& file : files) {
				 std::string ext;
				 size_t dotPos = file.first.find_last_of(".");
				 if (dotPos != std::string::npos) {
					 ext = file.first.substr(dotPos);
					 std::transform(ext.begin(), ext.end(), ext.begin(), 
								  [](unsigned char c) { return std::tolower(c); });
					 
					 if (validExtensions.find(ext) != validExtensions.end()) {
						 validHeader = true;
						 break;
					 }
				 }
			 }
		 }
		 
		 if (!validHeader) {
			 return ROMValidationStatus::INVALID_FORMAT;
		 }
		 
		 return ROMValidationStatus::VALID;
	 }
 }
 
 bool ROMLoader::LoadROMToMemory(
	 const std::unordered_map<std::string, std::vector<uint8_t>>& files,
	 const std::string& romName) {
	 
	 m_logger.Info("ROMLoader", "Loading ROM data into memory for: " + romName);
	 
	 // Clear loaded ROM files
	 m_loadedROMFiles.clear();
	 
	 // Check if ROM is in database
	 auto it = m_romDatabase.find(romName);
	 if (it != m_romDatabase.end()) {
		 // Found in database, use load addresses from database
		 for (const auto& expectedFile : it->second) {
			 auto fileIt = files.find(expectedFile.filename);
			 if (fileIt == files.end()) {
				 if (expectedFile.required) {
					 m_logger.Error("ROMLoader", "Missing required file: " + expectedFile.filename);
					 return false;
				 }
				 continue; // Skip missing optional files
			 }
			 
			 // Create ROM file info
			 ROMFileInfo fileInfo;
			 fileInfo.filename = expectedFile.filename;
			 fileInfo.size = fileIt->second.size();
			 fileInfo.crc32 = CalculateCRC32(fileIt->second.data(), fileIt->second.size());
			 fileInfo.md5 = CalculateMD5(fileIt->second.data(), fileIt->second.size());
			 fileInfo.sha1 = CalculateSHA1(fileIt->second.data(), fileIt->second.size());
			 fileInfo.loadAddress = expectedFile.loadAddress;
			 fileInfo.format = ROMFormat::BIN;
			 fileInfo.required = expectedFile.required;
			 fileInfo.region = expectedFile.region;
			 
			 // Load data into memory
			 if (!m_memoryManager.LoadROM(fileIt->second, fileInfo.loadAddress)) {
				 m_logger.Error("ROMLoader", "Failed to load ROM data at address " + 
							 std::to_string(fileInfo.loadAddress));
				 return false;
			 }
			 
			 // Add to loaded files list
			 m_loadedROMFiles.push_back(fileInfo);
			 
			 m_logger.Info("ROMLoader", "Loaded " + fileInfo.filename + " at address " + 
						std::to_string(fileInfo.loadAddress));
		 }
	 } else {
		 // Not in database, determine load addresses heuristically
		 uint32_t baseAddress = 0; // Start at address 0
		 
		 for (const auto& file : files) {
			 // Create ROM file info
			 ROMFileInfo fileInfo;
			 fileInfo.filename = file.first;
			 fileInfo.size = file.second.size();
			 fileInfo.crc32 = CalculateCRC32(file.second.data(), file.second.size());
			 fileInfo.md5 = CalculateMD5(file.second.data(), file.second.size());
			 fileInfo.sha1 = CalculateSHA1(file.second.data(), file.second.size());
			 fileInfo.loadAddress = baseAddress;
			 fileInfo.format = ROMFormat::BIN;
			 fileInfo.required = true;
			 fileInfo.region = "ROM";
			 
			 // Load data into memory
			 if (!m_memoryManager.LoadROM(file.second, fileInfo.loadAddress)) {
				 m_logger.Error("ROMLoader", "Failed to load ROM data at address " + 
							 std::to_string(fileInfo.loadAddress));
				 return false;
			 }
			 
			 // Add to loaded files list
			 m_loadedROMFiles.push_back(fileInfo);
			 
			 m_logger.Info("ROMLoader", "Loaded " + fileInfo.filename + " at address " + 
						std::to_string(fileInfo.loadAddress));
			 
			 // Increment base address for next file
			 baseAddress += file.second.size();
		 }
	 }
	 
	 m_logger.Info("ROMLoader", "ROM data loaded successfully");
	 return true;
 }
 
 uint32_t ROMLoader::CalculateCRC32(const uint8_t* data, size_t size) {
	 uint32_t crc = 0xFFFFFFFF;
	 
	 for (size_t i = 0; i < size; ++i) {
		 crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ data[i]];
	 }
	 
	 return crc ^ 0xFFFFFFFF;
 }
 
 std::string ROMLoader::CalculateMD5(const uint8_t* data, size_t size) {
	 // This is a placeholder for a real MD5 implementation
	 // In a real implementation, this would use a proper MD5 library
	 
	 // Generate a simple hash for demonstration
	 uint32_t hash = 0;
	 for (size_t i = 0; i < size; ++i) {
		 hash = ((hash << 5) + hash) + data[i];
	 }
	 
	 std::stringstream ss;
	 ss << std::hex << std::setfill('0') << std::setw(8) << hash;
	 ss << std::hex << std::setfill('0') << std::setw(8) << (hash ^ 0xFFFFFFFF);
	 ss << std::hex << std::setfill('0') << std::setw(8) << (hash >> 16);
	 ss << std::hex << std::setfill('0') << std::setw(8) << (hash << 16);
	 
	 return ss.str();
 }
 
 std::string ROMLoader::CalculateSHA1(const uint8_t* data, size_t size) {
	 // This is a placeholder for a real SHA1 implementation
	 // In a real implementation, this would use a proper SHA1 library
	 
	 // Generate a simple hash for demonstration
	 uint32_t hash = 0x67452301;
	 for (size_t i = 0; i < size; ++i) {
		 hash = ((hash << 5) + hash) + data[i];
	 }
	 
	 std::stringstream ss;
	 ss << std::hex << std::setfill('0') << std::setw(8) << hash;
	 ss << std::hex << std::setfill('0') << std::setw(8) << (hash ^ 0xFFFFFFFF);
	 ss << std::hex << std::setfill('0') << std::setw(8) << (hash >> 16);
	 ss << std::hex << std::setfill('0') << std::setw(8) << (hash << 16);
	 ss << std::hex << std::setfill('0') << std::setw(8) << (~hash);
	 
	 return ss.str();
 }
 
 void ROMLoader::NotifyProgressCallbacks(const ROMLoadProgress& progress) {
	 for (const auto& callback : m_progressCallbacks) {
		 callback.second(progress);
	 }
 }
 
 void ROMLoader::UpdateProgress(const std::string& currentFile, uint32_t bytesLoaded,
							  uint32_t totalBytes, ROMValidationStatus status) {
	 ROMLoadProgress progress;
	 progress.currentFile = currentFile;
	 progress.bytesLoaded = bytesLoaded;
	 progress.totalBytes = totalBytes;
	 progress.percentage = (totalBytes > 0) ? 
						 (static_cast<float>(bytesLoaded) / totalBytes * 100.0f) : 0.0f;
	 progress.status = status;
	 
	 NotifyProgressCallbacks(progress);
 }
 
 void ROMLoader::SetError(const std::string& error) {
	 m_lastError = error;
	 m_logger.Error("ROMLoader", error);
 }
 
 } // namespace NiXX32