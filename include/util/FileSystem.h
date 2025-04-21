/**
 * FileSystem.h
 * File system access utilities for NiXX-32 arcade board emulation
 * 
 * This file defines the file system access utilities that handle file operations
 * for the NiXX-32 arcade system emulator, including path management, file I/O,
 * and directory operations across different operating systems.
 */

 #pragma once

 #include <cstdint>
 #include <string>
 #include <vector>
 #include <fstream>
 #include <memory>
 #include <functional>
 
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 
 /**
  * File open modes
  */
 enum class FileMode {
	 READ,        // Open for reading
	 WRITE,       // Open for writing
	 APPEND,      // Open for appending
	 READ_WRITE,  // Open for both reading and writing
	 CREATE       // Create new file (overwrite if exists)
 };
 
 /**
  * File seek origins
  */
 enum class SeekOrigin {
	 BEGIN,       // Seek from beginning of file
	 CURRENT,     // Seek from current position
	 END          // Seek from end of file
 };
 
 /**
  * File type enumeration
  */
 enum class FileType {
	 REGULAR,     // Regular file
	 DIRECTORY,   // Directory
	 SYMBOLIC_LINK, // Symbolic link
	 SPECIAL,     // Special file (device, pipe, etc.)
	 UNKNOWN      // Unknown file type
 };
 
 /**
  * File information structure
  */
 struct FileInfo {
	 std::string name;        // File name
	 std::string path;        // Full path
	 FileType type;           // File type
	 uint64_t size;           // File size in bytes
	 uint64_t accessTime;     // Last access time
	 uint64_t modifyTime;     // Last modification time
	 uint64_t createTime;     // Creation time
	 bool isReadOnly;         // Whether file is read-only
	 bool isHidden;           // Whether file is hidden
 };
 
 /**
  * Class for file system operations
  */
 class FileSystem {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param logger Reference to the system logger
	  */
	 FileSystem(System& system, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~FileSystem();
	 
	 /**
	  * Initialize the file system
	  * @return True if initialization was successful
	  */
	 bool Initialize();
	 
	 /**
	  * Open a file
	  * @param path File path
	  * @param mode File open mode
	  * @return File handle (>= 0) if successful, -1 if failed
	  */
	 int OpenFile(const std::string& path, FileMode mode);
	 
	 /**
	  * Close a file
	  * @param handle File handle
	  * @return True if successful
	  */
	 bool CloseFile(int handle);
	 
	 /**
	  * Read data from file
	  * @param handle File handle
	  * @param buffer Buffer to read into
	  * @param size Number of bytes to read
	  * @return Number of bytes read, or -1 if failed
	  */
	 int64_t ReadFile(int handle, void* buffer, uint64_t size);
	 
	 /**
	  * Write data to file
	  * @param handle File handle
	  * @param buffer Buffer to write from
	  * @param size Number of bytes to write
	  * @return Number of bytes written, or -1 if failed
	  */
	 int64_t WriteFile(int handle, const void* buffer, uint64_t size);
	 
	 /**
	  * Seek to position in file
	  * @param handle File handle
	  * @param offset Offset in bytes
	  * @param origin Seek origin
	  * @return New position, or -1 if failed
	  */
	 int64_t SeekFile(int handle, int64_t offset, SeekOrigin origin);
	 
	 /**
	  * Get current position in file
	  * @param handle File handle
	  * @return Current position, or -1 if failed
	  */
	 int64_t GetFilePosition(int handle);
	 
	 /**
	  * Get file size
	  * @param handle File handle
	  * @return File size in bytes, or -1 if failed
	  */
	 int64_t GetFileSize(int handle);
	 
	 /**
	  * Flush file buffers to disk
	  * @param handle File handle
	  * @return True if successful
	  */
	 bool FlushFile(int handle);
	 
	 /**
	  * Check if file exists
	  * @param path File path
	  * @return True if file exists
	  */
	 bool FileExists(const std::string& path);
	 
	 /**
	  * Create directory
	  * @param path Directory path
	  * @param recursive Create parent directories if needed
	  * @return True if successful
	  */
	 bool CreateDirectory(const std::string& path, bool recursive = false);
	 
	 /**
	  * Remove file
	  * @param path File path
	  * @return True if successful
	  */
	 bool RemoveFile(const std::string& path);
	 
	 /**
	  * Remove directory
	  * @param path Directory path
	  * @param recursive Remove directory contents recursively
	  * @return True if successful
	  */
	 bool RemoveDirectory(const std::string& path, bool recursive = false);
	 
	 /**
	  * Rename file or directory
	  * @param oldPath Old path
	  * @param newPath New path
	  * @return True if successful
	  */
	 bool Rename(const std::string& oldPath, const std::string& newPath);
	 
	 /**
	  * Copy file
	  * @param sourcePath Source file path
	  * @param destPath Destination file path
	  * @param overwrite Overwrite destination if it exists
	  * @return True if successful
	  */
	 bool CopyFile(const std::string& sourcePath, const std::string& destPath, bool overwrite = false);
	 
	 /**
	  * Get file information
	  * @param path File path
	  * @return File information
	  */
	 FileInfo GetFileInfo(const std::string& path);
	 
	 /**
	  * Get list of files in directory
	  * @param path Directory path
	  * @param pattern File pattern to match (e.g., "*.txt")
	  * @return Vector of file information
	  */
	 std::vector<FileInfo> ListDirectory(const std::string& path, const std::string& pattern = "*");
	 
	 /**
	  * Get current working directory
	  * @return Current working directory path
	  */
	 std::string GetCurrentDirectory();
	 
	 /**
	  * Set current working directory
	  * @param path New current directory
	  * @return True if successful
	  */
	 bool SetCurrentDirectory(const std::string& path);
	 
	 /**
	  * Get absolute path
	  * @param path Relative or absolute path
	  * @return Absolute path
	  */
	 std::string GetAbsolutePath(const std::string& path);
	 
	 /**
	  * Normalize path (resolve ".." and "." components)
	  * @param path Path to normalize
	  * @return Normalized path
	  */
	 std::string NormalizePath(const std::string& path);
	 
	 /**
	  * Join path components
	  * @param path1 First path component
	  * @param path2 Second path component
	  * @return Joined path
	  */
	 std::string JoinPaths(const std::string& path1, const std::string& path2);
	 
	 /**
	  * Split path into components
	  * @param path Path to split
	  * @return Vector of path components
	  */
	 std::vector<std::string> SplitPath(const std::string& path);
	 
	 /**
	  * Get directory name (path without last component)
	  * @param path Path to get directory from
	  * @return Directory path
	  */
	 std::string GetDirectoryName(const std::string& path);
	 
	 /**
	  * Get file name (last component of path)
	  * @param path Path to get file name from
	  * @return File name
	  */
	 std::string GetFileName(const std::string& path);
	 
	 /**
	  * Get file extension
	  * @param path Path to get extension from
	  * @return File extension (with dot)
	  */
	 std::string GetFileExtension(const std::string& path);
	 
	 /**
	  * Change file attributes
	  * @param path File path
	  * @param readOnly Set read-only attribute
	  * @param hidden Set hidden attribute
	  * @return True if successful
	  */
	 bool SetFileAttributes(const std::string& path, bool readOnly, bool hidden);
	 
	 /**
	  * Get temporary file path
	  * @param prefix Prefix for temporary file name
	  * @return Temporary file path
	  */
	 std::string GetTempFilePath(const std::string& prefix = "nixx32_");
	 
	 /**
	  * Get application directory
	  * @return Application directory path
	  */
	 std::string GetApplicationDirectory();
	 
	 /**
	  * Get user data directory
	  * @return User data directory path
	  */
	 std::string GetUserDataDirectory();
	 
	 /**
	  * Get ROM directory
	  * @return ROM directory path
	  */
	 std::string GetROMDirectory();
	 
	 /**
	  * Set ROM directory
	  * @param path ROM directory path
	  */
	 void SetROMDirectory(const std::string& path);
	 
	 /**
	  * Get save state directory
	  * @return Save state directory path
	  */
	 std::string GetSaveStateDirectory();
	 
	 /**
	  * Set save state directory
	  * @param path Save state directory path
	  */
	 void SetSaveStateDirectory(const std::string& path);
	 
	 /**
	  * Load file to memory
	  * @param path File path
	  * @param binary True for binary mode, false for text mode
	  * @return File contents as vector of bytes
	  */
	 std::vector<uint8_t> LoadFileToMemory(const std::string& path, bool binary = true);
	 
	 /**
	  * Save memory to file
	  * @param path File path
	  * @param data Data to save
	  * @param size Size of data in bytes
	  * @param binary True for binary mode, false for text mode
	  * @return True if successful
	  */
	 bool SaveMemoryToFile(const std::string& path, const void* data, uint64_t size, bool binary = true);
	 
	 /**
	  * Load text file to string
	  * @param path File path
	  * @return File contents as string
	  */
	 std::string LoadTextFile(const std::string& path);
	 
	 /**
	  * Save string to text file
	  * @param path File path
	  * @param text Text to save
	  * @return True if successful
	  */
	 bool SaveTextFile(const std::string& path, const std::string& text);
	 
	 /**
	  * Create file stream
	  * @param path File path
	  * @param mode File open mode
	  * @return Shared pointer to file stream
	  */
	 std::shared_ptr<std::fstream> CreateFileStream(const std::string& path, FileMode mode);
	 
	 /**
	  * Get file time as timestamp
	  * @param path File path
	  * @param accessTime Output parameter for access time
	  * @param modifyTime Output parameter for modification time
	  * @param createTime Output parameter for creation time
	  * @return True if successful
	  */
	 bool GetFileTime(const std::string& path, uint64_t& accessTime, uint64_t& modifyTime, uint64_t& createTime);
	 
	 /**
	  * Set file time
	  * @param path File path
	  * @param accessTime Access time (0 to keep current)
	  * @param modifyTime Modification time (0 to keep current)
	  * @param createTime Creation time (0 to keep current)
	  * @return True if successful
	  */
	 bool SetFileTime(const std::string& path, uint64_t accessTime, uint64_t modifyTime, uint64_t createTime);
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // File handles
	 struct FileHandle {
		 std::shared_ptr<std::fstream> stream;
		 std::string path;
		 FileMode mode;
		 bool valid;
	 };
	 std::vector<FileHandle> m_fileHandles;
	 
	 // Directory paths
	 std::string m_applicationDir;
	 std::string m_userDataDir;
	 std::string m_romDir;
	 std::string m_saveStateDir;
	 
	 /**
	  * Get next available file handle
	  * @return Handle index, or -1 if none available
	  */
	 int GetNextFileHandle();
	 
	 /**
	  * Validate file handle
	  * @param handle File handle to validate
	  * @return True if handle is valid
	  */
	 bool IsValidHandle(int handle) const;
	 
	 /**
	  * Convert FileMode to std::ios_base::openmode
	  * @param mode File mode
	  * @return Equivalent std::ios_base::openmode
	  */
	 std::ios_base::openmode ConvertFileMode(FileMode mode) const;
	 
	 /**
	  * Initialize directory paths
	  */
	 void InitializePaths();
	 
	 /**
	  * Create directory structure
	  * @param path Directory path
	  * @return True if successful
	  */
	 bool CreateDirectoryStructure(const std::string& path);
	 
	 /**
	  * Remove directory contents
	  * @param path Directory path
	  * @return True if successful
	  */
	 bool RemoveDirectoryContents(const std::string& path);
 };
 
 } // namespace NiXX32