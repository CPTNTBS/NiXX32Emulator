/**
 * Logger.cpp
 * Implementation of the logging system for NiXX-32 arcade board emulation
 */

 #include "Logger.h"
 #include <iostream>
 #include <iomanip>
 #include <sstream>
 #include <chrono>
 #include <thread>
 #include <ctime>
 #include <algorithm>
 #include <filesystem>
 
 namespace NiXX32 {
 
 LoggerConfig Logger::DefaultConfig() {
	 LoggerConfig config;
	 config.minLevel = LogLevel::INFO;
	 config.outputs = { LogOutput::CONSOLE };
	 config.logFilePath = "nixx32_log.txt";
	 config.appendToFile = true;
	 config.includeTimestamp = true;
	 config.includeLevel = true;
	 config.includeComponent = true;
	 config.includeThreadId = false;
	 config.maxFileSize = 10 * 1024 * 1024; // 10 MB
	 config.maxBufferedMessages = 1000;
	 return config;
 }
 
 Logger::Logger(const LoggerConfig& config)
	 : m_config(config),
	   m_nextCallbackId(1),
	   m_initialized(false) {
	 // Note: We don't initialize the logger here because the constructor
	 // should not throw exceptions or perform I/O. Instead, we require
	 // explicit initialization with Initialize().
 }
 
 Logger::~Logger() {
	 Shutdown();
 }
 
 bool Logger::Initialize() {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 if (m_initialized) {
		 // Already initialized
		 return true;
	 }
	 
	 try {
		 // Initialize file output if enabled
		 if (std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::FILE) != m_config.outputs.end()) {
			 std::ios_base::openmode mode = std::ios::out;
			 if (m_config.appendToFile) {
				 mode |= std::ios::app;
			 }
			 
			 m_logFile.open(m_config.logFilePath, mode);
			 if (!m_logFile.is_open()) {
				 // Failed to open log file, disable file output
				 auto it = std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::FILE);
				 if (it != m_config.outputs.end()) {
					 m_config.outputs.erase(it);
				 }
				 
				 // Log this error to console
				 std::cerr << "Failed to open log file: " << m_config.logFilePath << std::endl;
			 }
		 }
		 
		 m_initialized = true;
		 
		 // Log initialization message
		 Info("Logger", "Logging system initialized");
		 
		 return true;
	 }
	 catch (const std::exception& e) {
		 std::cerr << "Exception during logger initialization: " << e.what() << std::endl;
		 return false;
	 }
	 catch (...) {
		 std::cerr << "Unknown exception during logger initialization" << std::endl;
		 return false;
	 }
 }
 
 void Logger::Shutdown() {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 if (!m_initialized) {
		 return;
	 }
	 
	 // Log shutdown message
	 Info("Logger", "Shutting down logging system");
	 
	 // Flush any remaining messages
	 Flush();
	 
	 // Close log file if open
	 if (m_logFile.is_open()) {
		 m_logFile.close();
	 }
	 
	 m_initialized = false;
 }
 
 void Logger::Log(LogLevel level, const std::string& component, const std::string& message) {
	 if (!ShouldLog(level, component)) {
		 return;
	 }
	 
	 // Create log message
	 LogMessage logMsg;
	 logMsg.level = level;
	 logMsg.component = component;
	 logMsg.message = message;
	 logMsg.timestamp = GetTimestamp();
	 logMsg.threadId = GetThreadId();
	 
	 // Lock for thread safety
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 // Add to history
	 AddToHistory(logMsg);
	 
	 // Output to console if enabled
	 if (std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::CONSOLE) != m_config.outputs.end()) {
		 WriteToConsole(logMsg);
	 }
	 
	 // Output to file if enabled
	 if (std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::FILE) != m_config.outputs.end()) {
		 WriteToFile(logMsg);
	 }
	 
	 // Invoke callbacks if enabled
	 if (std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::CALLBACK) != m_config.outputs.end()) {
		 InvokeCallbacks(logMsg);
	 }
 }
 
 void Logger::Debug(const std::string& component, const std::string& message) {
	 Log(LogLevel::DEBUG, component, message);
 }
 
 void Logger::Info(const std::string& component, const std::string& message) {
	 Log(LogLevel::INFO, component, message);
 }
 
 void Logger::Warning(const std::string& component, const std::string& message) {
	 Log(LogLevel::WARNING, component, message);
 }
 
 void Logger::Error(const std::string& component, const std::string& message) {
	 Log(LogLevel::ERROR, component, message);
 }
 
 void Logger::Fatal(const std::string& component, const std::string& message) {
	 Log(LogLevel::FATAL, component, message);
 }
 
 std::string Logger::FormatMessage(const LogMessage& message) const {
	 std::ostringstream ss;
	 
	 // Include timestamp if configured
	 if (m_config.includeTimestamp) {
		 ss << "[" << FormatTimestamp(message.timestamp) << "] ";
	 }
	 
	 // Include log level if configured
	 if (m_config.includeLevel) {
		 ss << "[" << LogLevelToString(message.level) << "] ";
	 }
	 
	 // Include component name if configured
	 if (m_config.includeComponent && !message.component.empty()) {
		 ss << "[" << message.component << "] ";
	 }
	 
	 // Include thread ID if configured
	 if (m_config.includeThreadId) {
		 ss << "[Thread " << message.threadId << "] ";
	 }
	 
	 // Add the message
	 ss << message.message;
	 
	 return ss.str();
 }
 
 void Logger::SetMinimumLevel(LogLevel level) {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 m_config.minLevel = level;
 }
 
 LogLevel Logger::GetMinimumLevel() const {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 return m_config.minLevel;
 }
 
 void Logger::SetComponentFilter(const std::string& component, LogLevel level) {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 m_componentFilters[component] = level;
 }
 
 void Logger::RemoveComponentFilter(const std::string& component) {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 auto it = m_componentFilters.find(component);
	 if (it != m_componentFilters.end()) {
		 m_componentFilters.erase(it);
	 }
 }
 
 void Logger::SetOutputEnabled(LogOutput output, bool enabled) {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 auto it = std::find(m_config.outputs.begin(), m_config.outputs.end(), output);
	 bool currentlyEnabled = (it != m_config.outputs.end());
	 
	 if (enabled && !currentlyEnabled) {
		 // Enable output
		 m_config.outputs.push_back(output);
		 
		 // If enabling file output, open the file
		 if (output == LogOutput::FILE && !m_logFile.is_open()) {
			 std::ios_base::openmode mode = std::ios::out;
			 if (m_config.appendToFile) {
				 mode |= std::ios::app;
			 }
			 
			 m_logFile.open(m_config.logFilePath, mode);
			 if (!m_logFile.is_open()) {
				 // Failed to open log file
				 Warning("Logger", "Failed to open log file: " + m_config.logFilePath);
				 
				 // Remove FILE from outputs
				 auto newIt = std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::FILE);
				 if (newIt != m_config.outputs.end()) {
					 m_config.outputs.erase(newIt);
				 }
			 }
		 }
	 }
	 else if (!enabled && currentlyEnabled) {
		 // Disable output
		 m_config.outputs.erase(it);
		 
		 // If disabling file output, close the file
		 if (output == LogOutput::FILE && m_logFile.is_open()) {
			 m_logFile.close();
		 }
	 }
 }
 
 bool Logger::IsOutputEnabled(LogOutput output) const {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 return std::find(m_config.outputs.begin(), m_config.outputs.end(), output) != m_config.outputs.end();
 }
 
 bool Logger::SetLogFile(const std::string& filePath, bool append) {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 // Close current log file if open
	 if (m_logFile.is_open()) {
		 m_logFile.close();
	 }
	 
	 // Update config
	 m_config.logFilePath = filePath;
	 m_config.appendToFile = append;
	 
	 // Open new log file
	 std::ios_base::openmode mode = std::ios::out;
	 if (append) {
		 mode |= std::ios::app;
	 }
	 
	 m_logFile.open(filePath, mode);
	 if (!m_logFile.is_open()) {
		 // Failed to open log file
		 std::cerr << "Failed to open log file: " << filePath << std::endl;
		 return false;
	 }
	 
	 // Enable file output if not already enabled
	 if (std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::FILE) == m_config.outputs.end()) {
		 m_config.outputs.push_back(LogOutput::FILE);
	 }
	 
	 return true;
 }
 
 int Logger::RegisterCallback(std::function<void(const LogMessage&)> callback) {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 int callbackId = m_nextCallbackId++;
	 m_callbacks[callbackId] = callback;
	 
	 // Enable callback output if not already enabled
	 if (std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::CALLBACK) == m_config.outputs.end()) {
		 m_config.outputs.push_back(LogOutput::CALLBACK);
	 }
	 
	 return callbackId;
 }
 
 bool Logger::RemoveCallback(int callbackId) {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 auto it = m_callbacks.find(callbackId);
	 if (it == m_callbacks.end()) {
		 return false;
	 }
	 
	 m_callbacks.erase(it);
	 
	 // If no more callbacks, disable callback output
	 if (m_callbacks.empty()) {
		 auto outputIt = std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::CALLBACK);
		 if (outputIt != m_config.outputs.end()) {
			 m_config.outputs.erase(outputIt);
		 }
	 }
	 
	 return true;
 }
 
 void Logger::Flush() {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 // Flush file output
	 if (m_logFile.is_open()) {
		 m_logFile.flush();
	 }
	 
	 // Flush console output
	 std::cout.flush();
	 std::cerr.flush();
 }
 
 void Logger::ClearHistory() {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 m_history.clear();
 }
 
 std::vector<LogMessage> Logger::GetHistory(size_t maxMessages) const {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 if (maxMessages == 0 || maxMessages >= m_history.size()) {
		 return m_history;
	 }
	 
	 // Return the most recent messages
	 return std::vector<LogMessage>(m_history.end() - maxMessages, m_history.end());
 }
 
 bool Logger::SaveHistoryToFile(const std::string& filePath) const {
	 std::lock_guard<std::mutex> lock(m_mutex);
	 
	 std::ofstream file(filePath);
	 if (!file.is_open()) {
		 return false;
	 }
	 
	 for (const auto& message : m_history) {
		 file << FormatMessage(message) << std::endl;
	 }
	 
	 return true;
 }
 
 std::string Logger::LogLevelToString(LogLevel level) {
	 switch (level) {
		 case LogLevel::DEBUG:
			 return "DEBUG";
		 case LogLevel::INFO:
			 return "INFO";
		 case LogLevel::WARNING:
			 return "WARNING";
		 case LogLevel::ERROR:
			 return "ERROR";
		 case LogLevel::FATAL:
			 return "FATAL";
		 default:
			 return "UNKNOWN";
	 }
 }
 
 LogLevel Logger::StringToLogLevel(const std::string& str) {
	 std::string upperStr = str;
	 std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), 
					[](unsigned char c) { return std::toupper(c); });
	 
	 if (upperStr == "DEBUG") return LogLevel::DEBUG;
	 if (upperStr == "INFO") return LogLevel::INFO;
	 if (upperStr == "WARNING" || upperStr == "WARN") return LogLevel::WARNING;
	 if (upperStr == "ERROR" || upperStr == "ERR") return LogLevel::ERROR;
	 if (upperStr == "FATAL" || upperStr == "CRITICAL") return LogLevel::FATAL;
	 
	 // Default to INFO if not recognized
	 return LogLevel::INFO;
 }
 
 uint64_t Logger::GetTimestamp() {
	 // Get current time in milliseconds since epoch
	 auto now = std::chrono::system_clock::now();
	 auto duration = now.time_since_epoch();
	 return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
 }
 
 std::string Logger::FormatTimestamp(uint64_t timestamp) {
	 // Convert milliseconds to seconds for time_t
	 std::time_t time = timestamp / 1000;
	 
	 // Get milliseconds part
	 uint32_t milliseconds = timestamp % 1000;
	 
	 // Format time part
	 std::tm* tm = std::localtime(&time);
	 char buffer[32];
	 std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
	 
	 // Add milliseconds
	 std::stringstream ss;
	 ss << buffer << "." << std::setw(3) << std::setfill('0') << milliseconds;
	 
	 return ss.str();
 }
 
 uint32_t Logger::GetThreadId() {
	 // Get current thread ID and convert to integer
	 std::ostringstream ss;
	 ss << std::this_thread::get_id();
	 uint32_t threadId;
	 ss >> threadId;
	 return threadId;
 }
 
 void Logger::WriteToConsole(const LogMessage& message) {
	 std::string formattedMessage = FormatMessage(message);
	 
	 // Output to the appropriate stream based on log level
	 if (message.level >= LogLevel::WARNING) {
		 std::cerr << formattedMessage << std::endl;
	 } else {
		 std::cout << formattedMessage << std::endl;
	 }
 }
 
 void Logger::WriteToFile(const LogMessage& message) {
	 if (!m_logFile.is_open()) {
		 return;
	 }
	 
	 // Check if file needs rotation
	 RotateLogFileIfNeeded();
	 
	 // Write formatted message to file
	 m_logFile << FormatMessage(message) << std::endl;
 }
 
 void Logger::InvokeCallbacks(const LogMessage& message) {
	 for (const auto& pair : m_callbacks) {
		 try {
			 pair.second(message);
		 }
		 catch (const std::exception& e) {
			 // Log error to console, avoiding potential recursion
			 std::cerr << "Exception in log callback: " << e.what() << std::endl;
		 }
		 catch (...) {
			 std::cerr << "Unknown exception in log callback" << std::endl;
		 }
	 }
 }
 
 void Logger::AddToHistory(const LogMessage& message) {
	 // Add to history, maintaining maximum size
	 m_history.push_back(message);
	 
	 if (m_history.size() > m_config.maxBufferedMessages && m_config.maxBufferedMessages > 0) {
		 m_history.erase(m_history.begin());
	 }
 }
 
 bool Logger::ShouldLog(LogLevel level, const std::string& component) const {
	 // Check component filter first
	 auto it = m_componentFilters.find(component);
	 if (it != m_componentFilters.end()) {
		 // Component has a specific filter level
		 return level >= it->second;
	 }
	 
	 // Fall back to global minimum level
	 return level >= m_config.minLevel;
 }
 
 void Logger::RotateLogFileIfNeeded() {
	 if (!m_logFile.is_open() || m_config.maxFileSize == 0) {
		 return;
	 }
	 
	 // Get current file size
	 m_logFile.flush();
	 std::streampos currentSize = m_logFile.tellp();
	 
	 if (currentSize >= static_cast<std::streampos>(m_config.maxFileSize)) {
		 // Close current file
		 m_logFile.close();
		 
		 // Rename current file to backup
		 std::string backupPath = m_config.logFilePath + ".bak";
		 
		 // Remove existing backup if it exists
		 std::remove(backupPath.c_str());
		 
		 // Rename current log to backup
		 std::rename(m_config.logFilePath.c_str(), backupPath.c_str());
		 
		 // Open new log file
		 m_logFile.open(m_config.logFilePath, std::ios::out);
		 if (!m_logFile.is_open()) {
			 // Failed to reopen log file
			 std::cerr << "Failed to rotate log file: " << m_config.logFilePath << std::endl;
			 
			 // Disable file output
			 auto it = std::find(m_config.outputs.begin(), m_config.outputs.end(), LogOutput::FILE);
			 if (it != m_config.outputs.end()) {
				 m_config.outputs.erase(it);
			 }
		 } else {
			 // Log rotation info
			 m_logFile << "Log file rotated at " << FormatTimestamp(GetTimestamp()) << std::endl;
		 }
	 }
 }
 
 } // namespace NiXX32