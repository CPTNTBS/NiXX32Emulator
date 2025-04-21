/**
 * Logger.h
 * Logging system for NiXX-32 arcade board emulation
 * 
 * This file defines the logging system that handles debug messages, warnings,
 * and errors for the NiXX-32 arcade system emulator. It supports different
 * logging levels, component filtering, and output to console or file.
 */

 #pragma once

 #include <string>
 #include <vector>
 #include <fstream>
 #include <functional>
 #include <unordered_map>
 #include <mutex>
 
 namespace NiXX32 {
 
 /**
  * Log message severity levels
  */
 enum class LogLevel {
	 DEBUG,      // Detailed debugging information
	 INFO,       // General information messages
	 WARNING,    // Warning conditions
	 ERROR,      // Error conditions
	 FATAL       // Critical errors causing termination
 };
 
 /**
  * Log output destination
  */
 enum class LogOutput {
	 CONSOLE,    // Standard output/error
	 FILE,       // Log file
	 CALLBACK,   // Custom callback function
	 ALL         // All available outputs
 };
 
 /**
  * Log message structure
  */
 struct LogMessage {
	 LogLevel level;         // Message severity level
	 std::string component;  // System component that generated the message
	 std::string message;    // The log message text
	 uint64_t timestamp;     // Timestamp when message was generated
	 uint32_t threadId;      // ID of thread that generated the message
 };
 
 /**
  * Logger configuration
  */
 struct LoggerConfig {
	 LogLevel minLevel;                 // Minimum level to log
	 std::vector<LogOutput> outputs;    // Enabled output destinations
	 std::string logFilePath;           // Path to log file (if file output enabled)
	 bool appendToFile;                 // Whether to append to or overwrite log file
	 bool includeTimestamp;             // Whether to include timestamps in log messages
	 bool includeLevel;                 // Whether to include log level in messages
	 bool includeComponent;             // Whether to include component name in messages
	 bool includeThreadId;              // Whether to include thread ID in messages
	 size_t maxFileSize;                // Maximum log file size (0 for unlimited)
	 size_t maxBufferedMessages;        // Maximum number of messages to buffer
 };
 
 /**
  * Main class for logging system
  */
 class Logger {
 public:
	 /**
	  * Constructor
	  * @param config Initial logger configuration
	  */
	 explicit Logger(const LoggerConfig& config = DefaultConfig());
	 
	 /**
	  * Destructor
	  */
	 ~Logger();
	 
	 /**
	  * Get default logger configuration
	  * @return Default configuration
	  */
	 static LoggerConfig DefaultConfig();
	 
	 /**
	  * Initialize the logger
	  * @return True if initialization was successful
	  */
	 bool Initialize();
	 
	 /**
	  * Shutdown the logger and flush any pending messages
	  */
	 void Shutdown();
	 
	 /**
	  * Log a message
	  * @param level Message severity level
	  * @param component System component generating the message
	  * @param message The message text
	  */
	 void Log(LogLevel level, const std::string& component, const std::string& message);
	 
	 /**
	  * Log a debug message
	  * @param component System component generating the message
	  * @param message The message text
	  */
	 void Debug(const std::string& component, const std::string& message);
	 
	 /**
	  * Log an info message
	  * @param component System component generating the message
	  * @param message The message text
	  */
	 void Info(const std::string& component, const std::string& message);
	 
	 /**
	  * Log a warning message
	  * @param component System component generating the message
	  * @param message The message text
	  */
	 void Warning(const std::string& component, const std::string& message);
	 
	 /**
	  * Log an error message
	  * @param component System component generating the message
	  * @param message The message text
	  */
	 void Error(const std::string& component, const std::string& message);
	 
	 /**
	  * Log a fatal error message
	  * @param component System component generating the message
	  * @param message The message text
	  */
	 void Fatal(const std::string& component, const std::string& message);
	 
	 /**
	  * Format a log message
	  * @param message The message to format
	  * @return Formatted message string
	  */
	 std::string FormatMessage(const LogMessage& message) const;
	 
	 /**
	  * Set the minimum log level
	  * @param level Minimum level to log
	  */
	 void SetMinimumLevel(LogLevel level);
	 
	 /**
	  * Get the current minimum log level
	  * @return Current minimum level
	  */
	 LogLevel GetMinimumLevel() const;
	 
	 /**
	  * Set component filter
	  * @param component Component name to filter (empty string for all)
	  * @param level Minimum level for this component
	  */
	 void SetComponentFilter(const std::string& component, LogLevel level);
	 
	 /**
	  * Remove component filter
	  * @param component Component name to remove filter for
	  */
	 void RemoveComponentFilter(const std::string& component);
	 
	 /**
	  * Enable or disable a log output destination
	  * @param output Output destination
	  * @param enabled True to enable, false to disable
	  */
	 void SetOutputEnabled(LogOutput output, bool enabled);
	 
	 /**
	  * Check if a log output destination is enabled
	  * @param output Output destination to check
	  * @return True if enabled
	  */
	 bool IsOutputEnabled(LogOutput output) const;
	 
	 /**
	  * Set log file path
	  * @param filePath Path to log file
	  * @param append True to append to existing file, false to overwrite
	  * @return True if successful
	  */
	 bool SetLogFile(const std::string& filePath, bool append = true);
	 
	 /**
	  * Register a log callback function
	  * @param callback Function to call for log messages
	  * @return Callback ID for removal
	  */
	 int RegisterCallback(std::function<void(const LogMessage&)> callback);
	 
	 /**
	  * Remove a log callback
	  * @param callbackId Callback ID to remove
	  * @return True if successful
	  */
	 bool RemoveCallback(int callbackId);
	 
	 /**
	  * Flush any buffered log messages
	  */
	 void Flush();
	 
	 /**
	  * Clear the log history
	  */
	 void ClearHistory();
	 
	 /**
	  * Get the log history
	  * @param maxMessages Maximum number of messages to retrieve (0 for all)
	  * @return Vector of log messages
	  */
	 std::vector<LogMessage> GetHistory(size_t maxMessages = 0) const;
	 
	 /**
	  * Save log history to file
	  * @param filePath Path to save file
	  * @return True if successful
	  */
	 bool SaveHistoryToFile(const std::string& filePath) const;
	 
	 /**
	  * Convert log level to string
	  * @param level Log level
	  * @return String representation
	  */
	 static std::string LogLevelToString(LogLevel level);
	 
	 /**
	  * Parse string as log level
	  * @param str String to parse
	  * @return Log level, or LogLevel::INFO if not recognized
	  */
	 static LogLevel StringToLogLevel(const std::string& str);
	 
	 /**
	  * Get current timestamp
	  * @return Current timestamp in milliseconds
	  */
	 static uint64_t GetTimestamp();
	 
	 /**
	  * Create a formatted timestamp string
	  * @param timestamp Timestamp to format
	  * @return Formatted timestamp string
	  */
	 static std::string FormatTimestamp(uint64_t timestamp);
	 
	 /**
	  * Get current thread ID
	  * @return Current thread ID
	  */
	 static uint32_t GetThreadId();
 
 private:
	 // Logger configuration
	 LoggerConfig m_config;
	 
	 // Log message history
	 std::vector<LogMessage> m_history;
	 
	 // Component filters (component name -> minimum level)
	 std::unordered_map<std::string, LogLevel> m_componentFilters;
	 
	 // Log file stream
	 std::ofstream m_logFile;
	 
	 // Callback functions
	 std::unordered_map<int, std::function<void(const LogMessage&)>> m_callbacks;
	 int m_nextCallbackId;
	 
	 // Thread synchronization
	 mutable std::mutex m_mutex;
	 
	 // Initialization state
	 bool m_initialized;
	 
	 /**
	  * Write message to console
	  * @param message Log message to write
	  */
	 void WriteToConsole(const LogMessage& message);
	 
	 /**
	  * Write message to log file
	  * @param message Log message to write
	  */
	 void WriteToFile(const LogMessage& message);
	 
	 /**
	  * Invoke registered callbacks
	  * @param message Log message to send
	  */
	 void InvokeCallbacks(const LogMessage& message);
	 
	 /**
	  * Add message to history
	  * @param message Log message to add
	  */
	 void AddToHistory(const LogMessage& message);
	 
	 /**
	  * Check if message should be logged
	  * @param level Message level
	  * @param component Message component
	  * @return True if message should be logged
	  */
	 bool ShouldLog(LogLevel level, const std::string& component) const;
	 
	 /**
	  * Rotate log file if it exceeds maximum size
	  */
	 void RotateLogFileIfNeeded();
 };
 
 } // namespace NiXX32