/**
 * FileSystem.cpp
 * Implementation of file system access utilities for NiXX-32 arcade board emulation
 */

 #include "FileSystem.h"
 #include "NiXX32System.h"
 #include <fstream>
 #include <sstream>
 #include <algorithm>
 #include <filesystem>
 #include <cstring>
 #include <random>
 #include <chrono>
 #include <stdexcept>
 
 #ifdef _WIN32
 #include <direct.h>
 #include <windows.h>
 #define PATH_SEPARATOR "\\"
 #define mkdir(dir, mode) _mkdir(dir)
 #else
 #include <unistd.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #define PATH_SEPARATOR "/"
 #endif
 
 namespace fs = std::filesystem;
 
 namespace NiXX32 {
 
 FileSystem::FileSystem(System& system, Logger& logger)
	 : m_system(system),
	   m_logger(logger) {
	 
	 m_logger.Info("FileSystem", "Initializing file system utilities");
 }
 
 FileSystem::~FileSystem() {
	 m_logger.Info("FileSystem", "Shutting down file system utilities");
	 
	 // Close any open file handles
	 for (auto& handle : m_fileHandles) {
		 if (handle.valid && handle.stream) {
			 handle.stream->close();
			 handle.valid = false;
		 }
	 }
 }
 
 bool FileSystem::Initialize() {
	 m_logger.Info("FileSystem", "Initializing file system");
	 
	 // Initialize directory paths
	 InitializePaths();
	 
	 // Create required directories if they don't exist
	 CreateDirectory(m_applicationDir, true);
	 CreateDirectory(m_userDataDir, true);
	 CreateDirectory(m_romDir, true);
	 CreateDirectory(m_saveStateDir, true);
	 
	 m_logger.Info("FileSystem", "File system initialized successfully");
	 return true;
 }
 
 int FileSystem::OpenFile(const std::string& path, FileMode mode) {
	 m_logger.Debug("FileSystem", "Opening file: " + path + " with mode " + std::to_string(static_cast<int>(mode)));
	 
	 // Get next available file handle
	 int handle = GetNextFileHandle();
	 if (handle < 0) {
		 m_logger.Error("FileSystem", "Failed to open file: Too many open files");
		 return -1;
	 }
	 
	 // Convert FileMode to std::ios_base::openmode
	 std::ios_base::openmode iosMode = ConvertFileMode(mode);
	 
	 // Create file stream
	 auto stream = std::make_shared<std::fstream>();
	 stream->open(path, iosMode);
	 
	 if (!stream->is_open()) {
		 m_logger.Error("FileSystem", "Failed to open file: " + path);
		 return -1;
	 }
	 
	 // Store file handle info
	 m_fileHandles[handle].stream = stream;
	 m_fileHandles[handle].path = path;
	 m_fileHandles[handle].mode = mode;
	 m_fileHandles[handle].valid = true;
	 
	 m_logger.Debug("FileSystem", "File opened successfully with handle " + std::to_string(handle));
	 return handle;
 }
 
 bool FileSystem::CloseFile(int handle) {
	 if (!IsValidHandle(handle)) {
		 m_logger.Error("FileSystem", "Invalid file handle: " + std::to_string(handle));
		 return false;
	 }
	 
	 m_logger.Debug("FileSystem", "Closing file with handle " + std::to_string(handle));
	 
	 // Close the file stream
	 m_fileHandles[handle].stream->close();
	 m_fileHandles[handle].valid = false;
	 
	 return true;
 }
 
 int64_t FileSystem::ReadFile(int handle, void* buffer, uint64_t size) {
	 if (!IsValidHandle(handle)) {
		 m_logger.Error("FileSystem", "Invalid file handle: " + std::to_string(handle));
		 return -1;
	 }
	 
	 // Check if file is readable
	 if (m_fileHandles[handle].mode != FileMode::READ && 
		 m_fileHandles[handle].mode != FileMode::READ_WRITE) {
		 m_logger.Error("FileSystem", "File not opened for reading");
		 return -1;
	 }
	 
	 // Read data from file
	 m_fileHandles[handle].stream->read(static_cast<char*>(buffer), size);
	 
	 // Return number of bytes read
	 return m_fileHandles[handle].stream->gcount();
 }
 
 int64_t FileSystem::WriteFile(int handle, const void* buffer, uint64_t size) {
	 if (!IsValidHandle(handle)) {
		 m_logger.Error("FileSystem", "Invalid file handle: " + std::to_string(handle));
		 return -1;
	 }
	 
	 // Check if file is writable
	 if (m_fileHandles[handle].mode != FileMode::WRITE && 
		 m_fileHandles[handle].mode != FileMode::APPEND && 
		 m_fileHandles[handle].mode != FileMode::READ_WRITE && 
		 m_fileHandles[handle].mode != FileMode::CREATE) {
		 m_logger.Error("FileSystem", "File not opened for writing");
		 return -1;
	 }
	 
	 // Store current position
	 std::streampos startPos = m_fileHandles[handle].stream->tellp();
	 
	 // Write data to file
	 m_fileHandles[handle].stream->write(static_cast<const char*>(buffer), size);
	 
	 if (m_fileHandles[handle].stream->fail()) {
		 m_logger.Error("FileSystem", "Write operation failed");
		 return -1;
	 }
	 
	 // Calculate number of bytes written
	 std::streampos endPos = m_fileHandles[handle].stream->tellp();
	 return static_cast<int64_t>(endPos - startPos);
 }
 
 int64_t FileSystem::SeekFile(int handle, int64_t offset, SeekOrigin origin) {
	 if (!IsValidHandle(handle)) {
		 m_logger.Error("FileSystem", "Invalid file handle: " + std::to_string(handle));
		 return -1;
	 }
	 
	 // Convert SeekOrigin to std::ios_base::seekdir
	 std::ios_base::seekdir dir;
	 switch (origin) {
		 case SeekOrigin::BEGIN:
			 dir = std::ios_base::beg;
			 break;
		 case SeekOrigin::CURRENT:
			 dir = std::ios_base::cur;
			 break;
		 case SeekOrigin::END:
			 dir = std::ios_base::end;
			 break;
		 default:
			 m_logger.Error("FileSystem", "Invalid seek origin");
			 return -1;
	 }
	 
	 // Seek to position
	 if (m_fileHandles[handle].mode == FileMode::READ || 
		 m_fileHandles[handle].mode == FileMode::READ_WRITE) {
		 m_fileHandles[handle].stream->seekg(offset, dir);
		 if (m_fileHandles[handle].stream->fail()) {
			 m_logger.Error("FileSystem", "Seek operation failed");
			 return -1;
		 }
		 return m_fileHandles[handle].stream->tellg();
	 } else {
		 m_fileHandles[handle].stream->seekp(offset, dir);
		 if (m_fileHandles[handle].stream->fail()) {
			 m_logger.Error("FileSystem", "Seek operation failed");
			 return -1;
		 }
		 return m_fileHandles[handle].stream->tellp();
	 }
 }
 
 int64_t FileSystem::GetFilePosition(int handle) {
	 if (!IsValidHandle(handle)) {
		 m_logger.Error("FileSystem", "Invalid file handle: " + std::to_string(handle));
		 return -1;
	 }
	 
	 if (m_fileHandles[handle].mode == FileMode::READ || 
		 m_fileHandles[handle].mode == FileMode::READ_WRITE) {
		 return m_fileHandles[handle].stream->tellg();
	 } else {
		 return m_fileHandles[handle].stream->tellp();
	 }
 }
 
 int64_t FileSystem::GetFileSize(int handle) {
	 if (!IsValidHandle(handle)) {
		 m_logger.Error("FileSystem", "Invalid file handle: " + std::to_string(handle));
		 return -1;
	 }
	 
	 // Save current position
	 std::streampos currentPos;
	 if (m_fileHandles[handle].mode == FileMode::READ || 
		 m_fileHandles[handle].mode == FileMode::READ_WRITE) {
		 currentPos = m_fileHandles[handle].stream->tellg();
	 } else {
		 currentPos = m_fileHandles[handle].stream->tellp();
	 }
	 
	 // Seek to end to get file size
	 m_fileHandles[handle].stream->seekg(0, std::ios::end);
	 std::streampos size = m_fileHandles[handle].stream->tellg();
	 
	 // Restore original position
	 if (m_fileHandles[handle].mode == FileMode::READ || 
		 m_fileHandles[handle].mode == FileMode::READ_WRITE) {
		 m_fileHandles[handle].stream->seekg(currentPos);
	 } else {
		 m_fileHandles[handle].stream->seekp(currentPos);
	 }
	 
	 return size;
 }
 
 bool FileSystem::FlushFile(int handle) {
	 if (!IsValidHandle(handle)) {
		 m_logger.Error("FileSystem", "Invalid file handle: " + std::to_string(handle));
		 return false;
	 }
	 
	 m_fileHandles[handle].stream->flush();
	 return !m_fileHandles[handle].stream->fail();
 }
 
 bool FileSystem::FileExists(const std::string& path) {
	 try {
		 return fs::exists(path);
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception checking if file exists: " + std::string(e.what()));
		 return false;
	 }
 }
 
 bool FileSystem::CreateDirectory(const std::string& path, bool recursive) {
	 if (path.empty()) {
		 m_logger.Error("FileSystem", "Empty directory path");
		 return false;
	 }
	 
	 try {
		 // Check if directory already exists
		 if (fs::exists(path) && fs::is_directory(path)) {
			 return true;
		 }
		 
		 if (recursive) {
			 return CreateDirectoryStructure(path);
		 } else {
			 return fs::create_directory(path);
		 }
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception creating directory: " + std::string(e.what()));
		 return false;
	 }
 }
 
 bool FileSystem::RemoveFile(const std::string& path) {
	 try {
		 if (!fs::exists(path)) {
			 m_logger.Warning("FileSystem", "File doesn't exist: " + path);
			 return false;
		 }
		 
		 if (fs::is_directory(path)) {
			 m_logger.Error("FileSystem", "Cannot remove directory with RemoveFile: " + path);
			 return false;
		 }
		 
		 return fs::remove(path);
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception removing file: " + std::string(e.what()));
		 return false;
	 }
 }
 
 bool FileSystem::RemoveDirectory(const std::string& path, bool recursive) {
	 try {
		 if (!fs::exists(path)) {
			 m_logger.Warning("FileSystem", "Directory doesn't exist: " + path);
			 return false;
		 }
		 
		 if (!fs::is_directory(path)) {
			 m_logger.Error("FileSystem", "Not a directory: " + path);
			 return false;
		 }
		 
		 if (recursive) {
			 return RemoveDirectoryContents(path) && fs::remove(path);
		 } else {
			 return fs::remove(path);
		 }
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception removing directory: " + std::string(e.what()));
		 return false;
	 }
 }
 
 bool FileSystem::Rename(const std::string& oldPath, const std::string& newPath) {
	 try {
		 if (!fs::exists(oldPath)) {
			 m_logger.Error("FileSystem", "Source path doesn't exist: " + oldPath);
			 return false;
		 }
		 
		 fs::rename(oldPath, newPath);
		 return true;
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception renaming file: " + std::string(e.what()));
		 return false;
	 }
 }
 
 bool FileSystem::CopyFile(const std::string& sourcePath, const std::string& destPath, bool overwrite) {
	 try {
		 if (!fs::exists(sourcePath)) {
			 m_logger.Error("FileSystem", "Source file doesn't exist: " + sourcePath);
			 return false;
		 }
		 
		 if (fs::exists(destPath) && !overwrite) {
			 m_logger.Error("FileSystem", "Destination file already exists: " + destPath);
			 return false;
		 }
		 
		 fs::copy_file(sourcePath, destPath, overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none);
		 return true;
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception copying file: " + std::string(e.what()));
		 return false;
	 }
 }
 
 FileInfo FileSystem::GetFileInfo(const std::string& path) {
	 FileInfo info;
	 info.name = GetFileName(path);
	 info.path = GetAbsolutePath(path);
	 
	 try {
		 if (!fs::exists(path)) {
			 info.type = FileType::UNKNOWN;
			 return info;
		 }
		 
		 auto status = fs::status(path);
		 
		 // Determine file type
		 if (fs::is_regular_file(status)) {
			 info.type = FileType::REGULAR;
		 } else if (fs::is_directory(status)) {
			 info.type = FileType::DIRECTORY;
		 } else if (fs::is_symlink(status)) {
			 info.type = FileType::SYMBOLIC_LINK;
		 } else {
			 info.type = FileType::SPECIAL;
		 }
		 
		 // Get file size (only valid for regular files)
		 if (info.type == FileType::REGULAR) {
			 info.size = fs::file_size(path);
		 } else {
			 info.size = 0;
		 }
		 
		 // Get file times
		 auto lastWriteTime = fs::last_write_time(path);
		 auto lastWriteTimePoint = fs::file_time_type::clock::to_time_t(lastWriteTime);
		 info.modifyTime = static_cast<uint64_t>(lastWriteTimePoint);
		 
		 // Some file systems don't support these, but we'll try
		 try {
			 auto fileStatus = fs::status(path);
			 info.isReadOnly = ((fileStatus.permissions() & fs::perms::owner_write) == fs::perms::none);
		 } catch (...) {
			 info.isReadOnly = false;
		 }
		 
		 // Determine if file is hidden
		 #ifdef _WIN32
		 DWORD attributes = GetFileAttributesA(path.c_str());
		 info.isHidden = (attributes != INVALID_FILE_ATTRIBUTES) && 
						  (attributes & FILE_ATTRIBUTE_HIDDEN);
		 #else
		 // On Unix-like systems, files starting with . are hidden
		 info.isHidden = (info.name.size() > 0 && info.name[0] == '.');
		 #endif
		 
		 // Set other times (not all file systems support these)
		 info.accessTime = info.modifyTime; // Default to modification time
		 info.createTime = info.modifyTime; // Default to modification time
		 
		 if (GetFileTime(path, info.accessTime, info.modifyTime, info.createTime)) {
			 // Successfully got file times
		 }
		 
	 } catch (const std::exception& e) {
		 m_logger.Warning("FileSystem", "Exception getting file info: " + std::string(e.what()));
	 }
	 
	 return info;
 }
 
 std::vector<FileInfo> FileSystem::ListDirectory(const std::string& path, const std::string& pattern) {
	 std::vector<FileInfo> files;
	 
	 try {
		 if (!fs::exists(path) || !fs::is_directory(path)) {
			 m_logger.Error("FileSystem", "Directory doesn't exist or is not a directory: " + path);
			 return files;
		 }
		 
		 for (const auto& entry : fs::directory_iterator(path)) {
			 // Apply pattern matching if specified
			 if (!pattern.empty() && pattern != "*") {
				 std::string filename = entry.path().filename().string();
				 if (!MatchesPattern(filename, pattern)) {
					 continue;
				 }
			 }
			 
			 // Get file info
			 FileInfo info = GetFileInfo(entry.path().string());
			 files.push_back(info);
		 }
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception listing directory: " + std::string(e.what()));
	 }
	 
	 return files;
 }
 
 std::string FileSystem::GetCurrentDirectory() {
	 try {
		 return fs::current_path().string();
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception getting current directory: " + std::string(e.what()));
		 return "";
	 }
 }
 
 bool FileSystem::SetCurrentDirectory(const std::string& path) {
	 try {
		 if (!fs::exists(path) || !fs::is_directory(path)) {
			 m_logger.Error("FileSystem", "Directory doesn't exist or is not a directory: " + path);
			 return false;
		 }
		 
		 fs::current_path(path);
		 return true;
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception setting current directory: " + std::string(e.what()));
		 return false;
	 }
 }
 
 std::string FileSystem::GetAbsolutePath(const std::string& path) {
	 try {
		 return fs::absolute(path).string();
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception getting absolute path: " + std::string(e.what()));
		 return path; // Return original path on error
	 }
 }
 
 std::string FileSystem::NormalizePath(const std::string& path) {
	 try {
		 return fs::path(path).lexically_normal().string();
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception normalizing path: " + std::string(e.what()));
		 return path; // Return original path on error
	 }
 }
 
 std::string FileSystem::JoinPaths(const std::string& path1, const std::string& path2) {
	 try {
		 fs::path result = fs::path(path1) / path2;
		 return result.string();
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception joining paths: " + std::string(e.what()));
		 // Fallback implementation
		 std::string separator = PATH_SEPARATOR;
		 std::string result = path1;
		 
		 // Ensure path1 ends with separator
		 if (!result.empty() && result.back() != separator[0]) {
			 result += separator;
		 }
		 
		 // Remove leading separator from path2 if present
		 std::string p2 = path2;
		 if (!p2.empty() && p2.front() == separator[0]) {
			 p2 = p2.substr(1);
		 }
		 
		 result += p2;
		 return result;
	 }
 }
 
 std::vector<std::string> FileSystem::SplitPath(const std::string& path) {
	 std::vector<std::string> components;
	 
	 try {
		 fs::path fsPath(path);
		 for (const auto& part : fsPath) {
			 components.push_back(part.string());
		 }
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception splitting path: " + std::string(e.what()));
		 
		 // Fallback implementation
		 std::string separator = PATH_SEPARATOR;
		 std::string p = path;
		 size_t pos = 0;
		 
		 while ((pos = p.find(separator)) != std::string::npos) {
			 std::string token = p.substr(0, pos);
			 if (!token.empty()) {
				 components.push_back(token);
			 }
			 p = p.substr(pos + separator.length());
		 }
		 
		 if (!p.empty()) {
			 components.push_back(p);
		 }
	 }
	 
	 return components;
 }
 
 std::string FileSystem::GetDirectoryName(const std::string& path) {
	 try {
		 fs::path fsPath(path);
		 return fsPath.parent_path().string();
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception getting directory name: " + std::string(e.what()));
		 
		 // Fallback implementation
		 std::string separator = PATH_SEPARATOR;
		 size_t pos = path.find_last_of(separator);
		 
		 if (pos == std::string::npos) {
			 return "";
		 }
		 
		 return path.substr(0, pos);
	 }
 }
 
 std::string FileSystem::GetFileName(const std::string& path) {
	 try {
		 fs::path fsPath(path);
		 return fsPath.filename().string();
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception getting file name: " + std::string(e.what()));
		 
		 // Fallback implementation
		 std::string separator = PATH_SEPARATOR;
		 size_t pos = path.find_last_of(separator);
		 
		 if (pos == std::string::npos) {
			 return path;
		 }
		 
		 return path.substr(pos + 1);
	 }
 }
 
 std::string FileSystem::GetFileExtension(const std::string& path) {
	 try {
		 fs::path fsPath(path);
		 return fsPath.extension().string();
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception getting file extension: " + std::string(e.what()));
		 
		 // Fallback implementation
		 std::string filename = GetFileName(path);
		 size_t pos = filename.find_last_of(".");
		 
		 if (pos == std::string::npos) {
			 return "";
		 }
		 
		 return filename.substr(pos);
	 }
 }
 
 bool FileSystem::SetFileAttributes(const std::string& path, bool readOnly, bool hidden) {
	 try {
		 if (!fs::exists(path)) {
			 m_logger.Error("FileSystem", "File doesn't exist: " + path);
			 return false;
		 }
		 
		 #ifdef _WIN32
		 DWORD attributes = GetFileAttributesA(path.c_str());
		 if (attributes == INVALID_FILE_ATTRIBUTES) {
			 m_logger.Error("FileSystem", "Failed to get file attributes");
			 return false;
		 }
		 
		 if (readOnly) {
			 attributes |= FILE_ATTRIBUTE_READONLY;
		 } else {
			 attributes &= ~FILE_ATTRIBUTE_READONLY;
		 }
		 
		 if (hidden) {
			 attributes |= FILE_ATTRIBUTE_HIDDEN;
		 } else {
			 attributes &= ~FILE_ATTRIBUTE_HIDDEN;
		 }
		 
		 return SetFileAttributesA(path.c_str(), attributes) != 0;
		 #else
		 // On Unix-like systems, set read-only by removing write permissions
		 if (readOnly) {
			 // Remove write permissions for all
			 chmod(path.c_str(), 0444);
		 } else {
			 // Add write permissions for owner
			 chmod(path.c_str(), 0644);
		 }
		 
		 // On Unix-like systems, hidden files start with a dot
		 // To make a file hidden, we need to rename it
		 if (hidden) {
			 std::string filename = GetFileName(path);
			 if (!filename.empty() && filename[0] != '.') {
				 std::string dir = GetDirectoryName(path);
				 std::string newPath = JoinPaths(dir, "." + filename);
				 return Rename(path, newPath);
			 }
		 } else {
			 std::string filename = GetFileName(path);
			 if (!filename.empty() && filename[0] == '.') {
				 std::string dir = GetDirectoryName(path);
				 std::string newPath = JoinPaths(dir, filename.substr(1));
				 return Rename(path, newPath);
			 }
		 }
		 
		 return true;
		 #endif
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception setting file attributes: " + std::string(e.what()));
		 return false;
	 }
 }
 
 std::string FileSystem::GetTempFilePath(const std::string& prefix) {
	 try {
		 // Create a unique temporary filename
		 auto now = std::chrono::system_clock::now();
		 auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
			 now.time_since_epoch()).count();
		 
		 // Generate random component
		 std::random_device rd;
		 std::mt19937 gen(rd());
		 std::uniform_int_distribution<int> dist(0, 999999);
		 int random = dist(gen);
		 
		 // Build temporary filename
		 std::string tempDir = fs::temp_directory_path().string();
		 std::string filename = prefix + std::to_string(timestamp) + "_" + std::to_string(random) + ".tmp";
		 
		 return JoinPaths(tempDir, filename);
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception getting temp file path: " + std::string(e.what()));
		 
		 // Fallback implementation
		 return JoinPaths(GetCurrentDirectory(), prefix + "_temp.tmp");
	 }
 }
 
 std::string FileSystem::GetApplicationDirectory() {
	 return m_applicationDir;
 }
 
 std::string FileSystem::GetUserDataDirectory() {
	 return m_userDataDir;
 }
 
 std::string FileSystem::GetROMDirectory() {
	 return m_romDir;
 }
 
 void FileSystem::SetROMDirectory(const std::string& path) {
	 m_romDir = path;
	 CreateDirectory(m_romDir, true);
	 m_logger.Info("FileSystem", "ROM directory set to: " + m_romDir);
 }
 
 std::string FileSystem::GetSaveStateDirectory() {
	 return m_saveStateDir;
 }
 
 void FileSystem::SetSaveStateDirectory(const std::string& path) {
	 m_saveStateDir = path;
	 CreateDirectory(m_saveStateDir, true);
	 m_logger.Info("FileSystem", "Save state directory set to: " + m_saveStateDir);
 }
 
 std::vector<uint8_t> FileSystem::LoadFileToMemory(const std::string& path, bool binary) {
	 std::vector<uint8_t> data;
	 
	 try {
		 if (!FileExists(path)) {
			 m_logger.Error("FileSystem", "File doesn't exist: " + path);
			 return data;
		 }
		 
		 // Open file
		 std::ios_base::openmode mode = std::ios::in;
		 if (binary) {
			 mode |= std::ios::binary;
		 }
		 
		 std::ifstream file(path, mode);
		 if (!file.is_open()) {
			 m_logger.Error("FileSystem", "Failed to open file: " + path);
			 return data;
		 }
		 
		 // Get file size
		 file.seekg(0, std::ios::end);
		 std::streamsize size = file.tellg();
		 file.seekg(0, std::ios::beg);
		 
		 // Read file content
		 data.resize(static_cast<size_t>(size));
		 file.read(reinterpret_cast<char*>(data.data()), size);
		 
		 if (!file) {
			 m_logger.Error("FileSystem", "Failed to read file: " + path);
			 data.clear();
		 }
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception loading file to memory: " + std::string(e.what()));
		 data.clear();
	 }
	 
	 return data;
 }
 
 bool FileSystem::SaveMemoryToFile(const std::string& path, const void* data, uint64_t size, bool binary) {
	 try {
		 // Create directory if it doesn't exist
		 std::string dir = GetDirectoryName(path);
		 if (!dir.empty() && !FileExists(dir)) {
			 if (!CreateDirectory(dir, true)) {
				 m_logger.Error("FileSystem", "Failed to create directory: " + dir);
				 return false;
			 }
		 }
		 
		 // Open file
		 std::ios_base::openmode mode = std::ios::out;
		 if (binary) {
			 mode |= std::ios::binary;
		 }
		 
		 std::ofstream file(path, mode);
		 if (!file.is_open()) {
			 m_logger.Error("FileSystem", "Failed to open file for writing: " + path);
			 return false;
		 }
		 
		 // Write data
		 file.write(static_cast<const char*>(data), size);
		 
		 if (!file) {
			 m_logger.Error("FileSystem", "Failed to write to file: " + path);
			 return false;
		 }
		 
		 return true;
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception saving memory to file: " + std::string(e.what()));
		 return false;
	 }
 }
 
 std::string FileSystem::LoadTextFile(const std::string& path) {
	 try {
		 if (!FileExists(path)) {
			 m_logger.Error("FileSystem", "File doesn't exist: " + path);
			 return "";
		 }
		 
		 std::ifstream file(path);
		 if (!file.is_open()) {
			 m_logger.Error("FileSystem", "Failed to open file: " + path);
			 return "";
		 }
		 
		 std::stringstream buffer;
		 buffer << file.rdbuf();
		 return buffer.str();
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception loading text file: " + std::string(e.what()));
		 return "";
	 }
 }
 
 bool FileSystem::SaveTextFile(const std::string& path, const std::string& text) {
	 return SaveMemoryToFile(path, text.c_str(), text.size(), false);
 }
 
 std::shared_ptr<std::fstream> FileSystem::CreateFileStream(const std::string& path, FileMode mode) {
	 try {
		 // Convert FileMode to std::ios_base::openmode
		 std::ios_base::openmode iosMode = ConvertFileMode(mode);
		 
		 // Create and open the file stream
		 auto stream = std::make_shared<std::fstream>();
		 stream->open(path, iosMode);
		 
		 if (!stream->is_open()) {
			 m_logger.Error("FileSystem", "Failed to create file stream: " + path);
			 return nullptr;
		 }
		 
		 return stream;
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception creating file stream: " + std::string(e.what()));
		 return nullptr;
	 }
 }
 
 bool FileSystem::GetFileTime(const std::string& path, uint64_t& accessTime, uint64_t& modifyTime, uint64_t& createTime) {
	 try {
		 if (!FileExists(path)) {
			 m_logger.Error("FileSystem", "File doesn't exist: " + path);
			 return false;
		 }
		 
		 #ifdef _WIN32
		 // Windows implementation
		 WIN32_FILE_ATTRIBUTE_DATA fileInfo;
		 if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
			 m_logger.Error("FileSystem", "Failed to get file attributes");
			 return false;
		 }
		 
		 // Convert Windows FILETIME to Unix timestamp
		 ULARGE_INTEGER uli;
		 uli.LowPart = fileInfo.ftLastAccessTime.dwLowDateTime;
		 uli.HighPart = fileInfo.ftLastAccessTime.dwHighDateTime;
		 accessTime = (uli.QuadPart / 10000000ULL - 11644473600ULL);
		 
		 uli.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
		 uli.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
		 modifyTime = (uli.QuadPart / 10000000ULL - 11644473600ULL);
		 
		 uli.LowPart = fileInfo.ftCreationTime.dwLowDateTime;
		 uli.HighPart = fileInfo.ftCreationTime.dwHighDateTime;
		 createTime = (uli.QuadPart / 10000000ULL - 11644473600ULL);
		 
		 return true;
		 #else
		 // Unix-like implementation
		 struct stat st;
		 if (stat(path.c_str(), &st) != 0) {
			 m_logger.Error("FileSystem", "Failed to get file stats");
			 return false;
		 }
		 
		 accessTime = st.st_atime;
		 modifyTime = st.st_mtime;
		 
		 // Not all Unix-like systems support creation time
		 #ifdef __APPLE__
		 createTime = st.st_birthtime;
		 #else
		 // Fallback to modification time if creation time not available
		 createTime = st.st_mtime;
		 #endif
		 
		 return true;
		 #endif
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception getting file time: " + std::string(e.what()));
		 return false;
	 }
 }
 
 bool FileSystem::SetFileTime(const std::string& path, uint64_t accessTime, uint64_t modifyTime, uint64_t createTime) {
	 try {
		 if (!FileExists(path)) {
			 m_logger.Error("FileSystem", "File doesn't exist: " + path);
			 return false;
		 }
		 
		 #ifdef _WIN32
		 // Windows implementation
		 HANDLE hFile = CreateFileA(
			 path.c_str(),
			 FILE_WRITE_ATTRIBUTES,
			 FILE_SHARE_READ | FILE_SHARE_WRITE,
			 NULL,
			 OPEN_EXISTING,
			 FILE_ATTRIBUTE_NORMAL,
			 NULL);
		 
		 if (hFile == INVALID_HANDLE_VALUE) {
			 m_logger.Error("FileSystem", "Failed to open file for setting attributes");
			 return false;
		 }
		 
		 FILETIME ftAccess, ftModify, ftCreate;
		 SYSTEMTIME stUTC;
		 
		 // Convert Unix timestamp to Windows FILETIME
		 if (accessTime > 0) {
			 GetSystemTime(&stUTC);
			 SystemTimeToFileTime(&stUTC, &ftAccess);
			 ULARGE_INTEGER uli;
			 uli.QuadPart = (accessTime + 11644473600ULL) * 10000000ULL;
			 ftAccess.dwLowDateTime = uli.LowPart;
			 ftAccess.dwHighDateTime = uli.HighPart;
		 }
		 
		 if (modifyTime > 0) {
			 GetSystemTime(&stUTC);
			 SystemTimeToFileTime(&stUTC, &ftModify);
			 ULARGE_INTEGER uli;
			 uli.QuadPart = (modifyTime + 11644473600ULL) * 10000000ULL;
			 ftModify.dwLowDateTime = uli.LowPart;
			 ftModify.dwHighDateTime = uli.HighPart;
		 }
		 
		 if (createTime > 0) {
			 GetSystemTime(&stUTC);
			 SystemTimeToFileTime(&stUTC, &ftCreate);
			 ULARGE_INTEGER uli;
			 uli.QuadPart = (createTime + 11644473600ULL) * 10000000ULL;
			 ftCreate.dwLowDateTime = uli.LowPart;
			 ftCreate.dwHighDateTime = uli.HighPart;
		 }
		 
		 bool success = SetFileTime(
			 hFile,
			 (createTime > 0) ? &ftCreate : NULL,
			 (accessTime > 0) ? &ftAccess : NULL,
			 (modifyTime > 0) ? &ftModify : NULL
		 ) != 0;
		 
		 CloseHandle(hFile);
		 return success;
		 #else
		 // Unix-like implementation
		 struct utimbuf times;
		 
		 times.actime = (accessTime > 0) ? accessTime : time(NULL);
		 times.modtime = (modifyTime > 0) ? modifyTime : time(NULL);
		 
		 // Note: Most Unix-like systems don't support setting creation time
		 return (utime(path.c_str(), &times) == 0);
		 #endif
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception setting file time: " + std::string(e.what()));
		 return false;
	 }
 }
 
 int FileSystem::GetNextFileHandle() {
	 // Find the first unused handle slot
	 for (size_t i = 0; i < m_fileHandles.size(); i++) {
		 if (!m_fileHandles[i].valid) {
			 return static_cast<int>(i);
		 }
	 }
	 
	 // If all existing slots are used, create a new one
	 if (m_fileHandles.size() < 1024) { // Arbitrary limit
		 m_fileHandles.push_back(FileHandle());
		 return static_cast<int>(m_fileHandles.size() - 1);
	 }
	 
	 // No available handles
	 return -1;
 }
 
 bool FileSystem::IsValidHandle(int handle) const {
	 return (handle >= 0 && 
			 handle < static_cast<int>(m_fileHandles.size()) && 
			 m_fileHandles[handle].valid);
 }
 
 std::ios_base::openmode FileSystem::ConvertFileMode(FileMode mode) const {
	 std::ios_base::openmode iosMode = std::ios_base::openmode(0);
	 
	 switch (mode) {
		 case FileMode::READ:
			 iosMode = std::ios::in;
			 break;
		 case FileMode::WRITE:
			 iosMode = std::ios::out | std::ios::trunc;
			 break;
		 case FileMode::APPEND:
			 iosMode = std::ios::out | std::ios::app;
			 break;
		 case FileMode::READ_WRITE:
			 iosMode = std::ios::in | std::ios::out;
			 break;
		 case FileMode::CREATE:
			 iosMode = std::ios::out | std::ios::trunc;
			 break;
	 }
	 
	 return iosMode;
 }
 
 void FileSystem::InitializePaths() {
	 // Determine application directory (current directory by default)
	 m_applicationDir = GetCurrentDirectory();
	 
	 // Determine user data directory
	 #ifdef _WIN32
	 // On Windows, use AppData
	 char appData[MAX_PATH];
	 if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
		 m_userDataDir = JoinPaths(std::string(appData), "NiXX32");
	 } else {
		 m_userDataDir = JoinPaths(m_applicationDir, "userdata");
	 }
	 #else
	 // On Unix-like systems, use ~/.config or ~/.local/share
	 const char* home = getenv("HOME");
	 if (home) {
		 const char* xdgConfig = getenv("XDG_CONFIG_HOME");
		 if (xdgConfig && xdgConfig[0] != '\0') {
			 m_userDataDir = JoinPaths(std::string(xdgConfig), "NiXX32");
		 } else {
			 m_userDataDir = JoinPaths(std::string(home), ".config/NiXX32");
		 }
	 } else {
		 m_userDataDir = JoinPaths(m_applicationDir, "userdata");
	 }
	 #endif
	 
	 // Initialize ROM and save state directories
	 m_romDir = JoinPaths(m_userDataDir, "roms");
	 m_saveStateDir = JoinPaths(m_userDataDir, "saves");
	 
	 m_logger.Info("FileSystem", "Application directory: " + m_applicationDir);
	 m_logger.Info("FileSystem", "User data directory: " + m_userDataDir);
	 m_logger.Info("FileSystem", "ROM directory: " + m_romDir);
	 m_logger.Info("FileSystem", "Save state directory: " + m_saveStateDir);
 }
 
 bool FileSystem::CreateDirectoryStructure(const std::string& path) {
	 try {
		 fs::path fsPath(path);
		 
		 // Create each directory in the path
		 fs::path current;
		 for (const auto& part : fsPath) {
			 current /= part;
			 
			 if (!current.empty() && !fs::exists(current)) {
				 if (!fs::create_directory(current)) {
					 m_logger.Error("FileSystem", "Failed to create directory: " + current.string());
					 return false;
				 }
			 }
		 }
		 
		 return true;
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception creating directory structure: " + std::string(e.what()));
		 
		 // Fallback implementation for older compilers
		 std::string currentPath;
		 std::vector<std::string> components = SplitPath(path);
		 
		 for (const auto& component : components) {
			 if (component.empty()) {
				 continue;
			 }
			 
			 // Build current path
			 if (currentPath.empty()) {
				 currentPath = component;
				 
				 // Handle absolute paths
				 #ifdef _WIN32
				 if (component.size() >= 2 && component[1] == ':') {
					 continue; // Skip drive letter
				 }
				 #else
				 if (component[0] == '/') {
					 continue; // Skip root
				 }
				 #endif
			 } else {
				 currentPath = JoinPaths(currentPath, component);
			 }
			 
			 // Create directory if it doesn't exist
			 if (!FileExists(currentPath)) {
				 #ifdef _WIN32
				 if (_mkdir(currentPath.c_str()) != 0) {
					 m_logger.Error("FileSystem", "Failed to create directory: " + currentPath);
					 return false;
				 }
				 #else
				 if (mkdir(currentPath.c_str(), 0755) != 0) {
					 m_logger.Error("FileSystem", "Failed to create directory: " + currentPath);
					 return false;
				 }
				 #endif
			 }
		 }
		 
		 return true;
	 }
 }
 
 bool FileSystem::RemoveDirectoryContents(const std::string& path) {
	 try {
		 for (const auto& entry : fs::directory_iterator(path)) {
			 if (fs::is_directory(entry.status())) {
				 // Recursively remove subdirectories
				 if (!RemoveDirectoryContents(entry.path().string()) || 
					 !fs::remove(entry.path())) {
					 return false;
				 }
			 } else {
				 // Remove file
				 if (!fs::remove(entry.path())) {
					 return false;
				 }
			 }
		 }
		 return true;
	 } catch (const std::exception& e) {
		 m_logger.Error("FileSystem", "Exception removing directory contents: " + std::string(e.what()));
		 return false;
	 }
 }
 
 bool FileSystem::MatchesPattern(const std::string& filename, const std::string& pattern) {
	 // Simple wildcard pattern matching
	 // This is a simplified implementation, handles * and ? wildcards
	 
	 // Fast path for common cases
	 if (pattern == "*") {
		 return true;
	 }
	 
	 if (pattern.find_first_of("*?") == std::string::npos) {
		 // No wildcards, exact match
		 return filename == pattern;
	 }
	 
	 return WildcardMatch(filename.c_str(), pattern.c_str());
 }
 
 bool FileSystem::WildcardMatch(const char* str, const char* pattern) {
	 // Implement a simple wildcard matching algorithm
	 if (*pattern == '\0') {
		 return *str == '\0';
	 }
	 
	 if (*pattern == '*') {
		 while (*(pattern + 1) == '*') {
			 pattern++; // Skip consecutive *
		 }
		 
		 if (*(pattern + 1) == '\0') {
			 return true; // Pattern ends with *, match anything
		 }
		 
		 while (*str != '\0') {
			 if (WildcardMatch(str, pattern + 1)) {
				 return true;
			 }
			 str++;
		 }
		 
		 return WildcardMatch(str, pattern + 1);
	 }
	 
	 if (*pattern == '?' || *pattern == *str) {
		 return WildcardMatch(str + 1, pattern + 1);
	 }
	 
	 return false;
 }
 
 } // namespace NiXX32