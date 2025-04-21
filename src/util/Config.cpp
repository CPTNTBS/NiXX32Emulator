/**
 * Config.cpp
 * Implementation of the persistent configuration system for NiXX-32 arcade board emulation
 */

 #include "Config.h"
 #include "NiXX32System.h"
 #include <fstream>
 #include <sstream>
 #include <algorithm>
 #include <cctype>
 #include <cstdlib>
 #include <stdexcept>
 #include <iomanip>
 
 namespace NiXX32 {
 
 //
 // ConfigValue Implementation
 //
 
 ConfigValue::ConfigValue()
	 : m_type(ConfigValueType::BOOLEAN),
	   m_boolValue(false) {
 }
 
 ConfigValue::ConfigValue(bool value)
	 : m_type(ConfigValueType::BOOLEAN),
	   m_boolValue(value) {
 }
 
 ConfigValue::ConfigValue(int value)
	 : m_type(ConfigValueType::INTEGER),
	   m_intValue(value) {
 }
 
 ConfigValue::ConfigValue(float value)
	 : m_type(ConfigValueType::FLOAT),
	   m_floatValue(value) {
 }
 
 ConfigValue::ConfigValue(const std::string& value)
	 : m_type(ConfigValueType::STRING),
	   m_stringValue(value) {
 }
 
 ConfigValue::ConfigValue(const std::vector<ConfigValue>& values)
	 : m_type(ConfigValueType::ARRAY),
	   m_arrayValue(values) {
 }
 
 ConfigValue::~ConfigValue() {
	 // Nothing to do here, std::string and std::vector handle their own cleanup
 }
 
 ConfigValueType ConfigValue::GetType() const {
	 return m_type;
 }
 
 bool ConfigValue::AsBool() const {
	 if (m_type == ConfigValueType::BOOLEAN) {
		 return m_boolValue;
	 } else if (m_type == ConfigValueType::INTEGER) {
		 return m_intValue != 0;
	 } else if (m_type == ConfigValueType::FLOAT) {
		 return m_floatValue != 0.0f;
	 } else if (m_type == ConfigValueType::STRING) {
		 return !m_stringValue.empty() && 
				(m_stringValue == "true" || m_stringValue == "1" || m_stringValue == "yes" || m_stringValue == "on");
	 }
	 return false;
 }
 
 int ConfigValue::AsInt() const {
	 if (m_type == ConfigValueType::INTEGER) {
		 return m_intValue;
	 } else if (m_type == ConfigValueType::BOOLEAN) {
		 return m_boolValue ? 1 : 0;
	 } else if (m_type == ConfigValueType::FLOAT) {
		 return static_cast<int>(m_floatValue);
	 } else if (m_type == ConfigValueType::STRING) {
		 try {
			 return std::stoi(m_stringValue);
		 } catch (...) {
			 return 0;
		 }
	 }
	 return 0;
 }
 
 float ConfigValue::AsFloat() const {
	 if (m_type == ConfigValueType::FLOAT) {
		 return m_floatValue;
	 } else if (m_type == ConfigValueType::INTEGER) {
		 return static_cast<float>(m_intValue);
	 } else if (m_type == ConfigValueType::BOOLEAN) {
		 return m_boolValue ? 1.0f : 0.0f;
	 } else if (m_type == ConfigValueType::STRING) {
		 try {
			 return std::stof(m_stringValue);
		 } catch (...) {
			 return 0.0f;
		 }
	 }
	 return 0.0f;
 }
 
 std::string ConfigValue::AsString() const {
	 if (m_type == ConfigValueType::STRING) {
		 return m_stringValue;
	 } else if (m_type == ConfigValueType::BOOLEAN) {
		 return m_boolValue ? "true" : "false";
	 } else if (m_type == ConfigValueType::INTEGER) {
		 return std::to_string(m_intValue);
	 } else if (m_type == ConfigValueType::FLOAT) {
		 std::stringstream ss;
		 ss << std::fixed << std::setprecision(6) << m_floatValue;
		 // Remove trailing zeros and decimal point if necessary
		 std::string str = ss.str();
		 str.erase(str.find_last_not_of('0') + 1, std::string::npos);
		 if (str.back() == '.') {
			 str.pop_back();
		 }
		 return str;
	 } else if (m_type == ConfigValueType::ARRAY) {
		 std::stringstream ss;
		 ss << "[";
		 for (size_t i = 0; i < m_arrayValue.size(); ++i) {
			 if (i > 0) ss << ", ";
			 
			 // For string values in arrays, add quotes
			 if (m_arrayValue[i].GetType() == ConfigValueType::STRING) {
				 ss << "\"" << m_arrayValue[i].AsString() << "\"";
			 } else {
				 ss << m_arrayValue[i].ToString();
			 }
		 }
		 ss << "]";
		 return ss.str();
	 }
	 return "";
 }
 
 std::vector<ConfigValue> ConfigValue::AsArray() const {
	 if (m_type == ConfigValueType::ARRAY) {
		 return m_arrayValue;
	 }
	 // Return a single-element array with this value
	 return { *this };
 }
 
 std::string ConfigValue::ToString() const {
	 return AsString();
 }
 
 ConfigValue ConfigValue::FromString(const std::string& str, ConfigValueType type) {
	 switch (type) {
		 case ConfigValueType::BOOLEAN:
			 if (str == "true" || str == "1" || str == "yes" || str == "on") {
				 return ConfigValue(true);
			 } else {
				 return ConfigValue(false);
			 }
		 
		 case ConfigValueType::INTEGER:
			 try {
				 return ConfigValue(std::stoi(str));
			 } catch (...) {
				 return ConfigValue(0);
			 }
		 
		 case ConfigValueType::FLOAT:
			 try {
				 return ConfigValue(std::stof(str));
			 } catch (...) {
				 return ConfigValue(0.0f);
			 }
		 
		 case ConfigValueType::STRING:
			 return ConfigValue(str);
		 
		 case ConfigValueType::ARRAY: {
			 // Parse array string like "[value1, value2, ...]"
			 std::vector<ConfigValue> values;
			 
			 // Simple parsing - look for opening and closing brackets
			 size_t start = str.find('[');
			 size_t end = str.rfind(']');
			 
			 if (start != std::string::npos && end != std::string::npos && start < end) {
				 // Extract content between brackets
				 std::string content = str.substr(start + 1, end - start - 1);
				 
				 // Split by commas
				 size_t pos = 0;
				 size_t nextPos;
				 
				 while ((nextPos = content.find(',', pos)) != std::string::npos) {
					 std::string element = content.substr(pos, nextPos - pos);
					 
					 // Trim whitespace
					 element.erase(0, element.find_first_not_of(" \t\r\n"));
					 element.erase(element.find_last_not_of(" \t\r\n") + 1);
					 
					 // Check if it's a quoted string
					 if (element.size() >= 2 && element.front() == '"' && element.back() == '"') {
						 // Remove quotes
						 element = element.substr(1, element.size() - 2);
						 values.push_back(ConfigValue(element));
					 } else {
						 // Try to guess the type
						 if (element == "true" || element == "false") {
							 values.push_back(ConfigValue(element == "true"));
						 } else if (element.find('.') != std::string::npos) {
							 // Assume float
							 try {
								 values.push_back(ConfigValue(std::stof(element)));
							 } catch (...) {
								 values.push_back(ConfigValue(element));
							 }
						 } else {
							 // Try int, fall back to string
							 try {
								 values.push_back(ConfigValue(std::stoi(element)));
							 } catch (...) {
								 values.push_back(ConfigValue(element));
							 }
						 }
					 }
					 
					 pos = nextPos + 1;
				 }
				 
				 // Process the last element
				 if (pos < content.size()) {
					 std::string element = content.substr(pos);
					 
					 // Trim whitespace
					 element.erase(0, element.find_first_not_of(" \t\r\n"));
					 element.erase(element.find_last_not_of(" \t\r\n") + 1);
					 
					 // Check if it's a quoted string
					 if (element.size() >= 2 && element.front() == '"' && element.back() == '"') {
						 // Remove quotes
						 element = element.substr(1, element.size() - 2);
						 values.push_back(ConfigValue(element));
					 } else {
						 // Try to guess the type
						 if (element == "true" || element == "false") {
							 values.push_back(ConfigValue(element == "true"));
						 } else if (element.find('.') != std::string::npos) {
							 // Assume float
							 try {
								 values.push_back(ConfigValue(std::stof(element)));
							 } catch (...) {
								 values.push_back(ConfigValue(element));
							 }
						 } else {
							 // Try int, fall back to string
							 try {
								 values.push_back(ConfigValue(std::stoi(element)));
							 } catch (...) {
								 values.push_back(ConfigValue(element));
							 }
						 }
					 }
				 }
			 }
			 
			 return ConfigValue(values);
		 }
	 }
	 
	 // Default case
	 return ConfigValue();
 }
 
 //
 // Config Implementation
 //
 
 Config::Config(System& system, Logger& logger, const std::string& configPath)
	 : m_system(system),
	   m_logger(logger),
	   m_configPath(configPath),
	   m_nextCallbackId(1),
	   m_modified(false) {
	 
	 // Log initialization
	 m_logger.Info("Config", "Initializing configuration system with path: " + configPath);
 }
 
 Config::~Config() {
	 // Save configuration if modified
	 if (m_modified) {
		 m_logger.Info("Config", "Saving configuration changes on shutdown");
		 SaveToFile();
	 }
	 
	 m_logger.Info("Config", "Configuration system shutdown");
 }
 
 bool Config::Initialize() {
	 m_logger.Info("Config", "Initializing configuration system");
	 
	 // Define default options first
	 DefineDefaultOptions();
	 
	 // Then load from file, which will override defaults if file exists
	 if (!LoadFromFile()) {
		 m_logger.Warning("Config", "Failed to load configuration file, using defaults");
	 }
	 
	 m_logger.Info("Config", "Configuration system initialized successfully");
	 return true;
 }
 
 bool Config::LoadFromFile() {
	 m_logger.Info("Config", "Loading configuration from: " + m_configPath);
	 
	 std::ifstream file(m_configPath);
	 if (!file.is_open()) {
		 m_logger.Warning("Config", "Could not open configuration file: " + m_configPath);
		 return false;
	 }
	 
	 // Read entire file content
	 std::stringstream buffer;
	 buffer << file.rdbuf();
	 std::string content = buffer.str();
	 
	 // Parse configuration
	 bool result = ParseConfigFile(content);
	 
	 // Reset modified flag since we just loaded
	 m_modified = false;
	 
	 return result;
 }
 
 bool Config::SaveToFile() {
	 m_logger.Info("Config", "Saving configuration to: " + m_configPath);
	 
	 std::string content = GenerateConfigFile();
	 
	 std::ofstream file(m_configPath);
	 if (!file.is_open()) {
		 m_logger.Error("Config", "Could not open configuration file for writing: " + m_configPath);
		 return false;
	 }
	 
	 file << content;
	 
	 // Reset modified flag since we just saved
	 m_modified = false;
	 
	 m_logger.Info("Config", "Configuration saved successfully");
	 return true;
 }
 
 void Config::ResetToDefaults() {
	 m_logger.Info("Config", "Resetting configuration to defaults");
	 
	 // Clear all values
	 m_values.clear();
	 
	 // Set default values for all registered options
	 for (const auto& pair : m_options) {
		 const ConfigOption& option = pair.second;
		 m_values[option.key] = option.defaultValue;
	 }
	 
	 m_modified = true;
	 m_logger.Info("Config", "Configuration reset to defaults");
 }
 
 bool Config::GetBool(const std::string& key, bool defaultValue) const {
	 ConfigValue value = GetTypedValue(key, ConfigValueType::BOOLEAN);
	 if (value.GetType() != ConfigValueType::BOOLEAN) {
		 return defaultValue;
	 }
	 return value.AsBool();
 }
 
 bool Config::SetBool(const std::string& key, bool value) {
	 ConfigValue oldValue = Get(key);
	 ConfigValue newValue(value);
	 
	 // Validate type if option is defined
	 if (!ValidateValueType(key, newValue)) {
		 return false;
	 }
	 
	 m_values[key] = newValue;
	 m_modified = true;
	 
	 // Notify callbacks
	 NotifyChangeCallbacks(key, oldValue, newValue);
	 
	 return true;
 }
 
 int Config::GetInt(const std::string& key, int defaultValue) const {
	 ConfigValue value = GetTypedValue(key, ConfigValueType::INTEGER);
	 if (value.GetType() != ConfigValueType::INTEGER) {
		 return defaultValue;
	 }
	 return value.AsInt();
 }
 
 bool Config::SetInt(const std::string& key, int value) {
	 ConfigValue oldValue = Get(key);
	 ConfigValue newValue(value);
	 
	 // Validate type if option is defined
	 if (!ValidateValueType(key, newValue)) {
		 return false;
	 }
	 
	 m_values[key] = newValue;
	 m_modified = true;
	 
	 // Notify callbacks
	 NotifyChangeCallbacks(key, oldValue, newValue);
	 
	 return true;
 }
 
 float Config::GetFloat(const std::string& key, float defaultValue) const {
	 ConfigValue value = GetTypedValue(key, ConfigValueType::FLOAT);
	 if (value.GetType() != ConfigValueType::FLOAT) {
		 return defaultValue;
	 }
	 return value.AsFloat();
 }
 
 bool Config::SetFloat(const std::string& key, float value) {
	 ConfigValue oldValue = Get(key);
	 ConfigValue newValue(value);
	 
	 // Validate type if option is defined
	 if (!ValidateValueType(key, newValue)) {
		 return false;
	 }
	 
	 m_values[key] = newValue;
	 m_modified = true;
	 
	 // Notify callbacks
	 NotifyChangeCallbacks(key, oldValue, newValue);
	 
	 return true;
 }
 
 std::string Config::GetString(const std::string& key, const std::string& defaultValue) const {
	 ConfigValue value = GetTypedValue(key, ConfigValueType::STRING);
	 if (value.GetType() != ConfigValueType::STRING) {
		 return defaultValue;
	 }
	 return value.AsString();
 }
 
 bool Config::SetString(const std::string& key, const std::string& value) {
	 ConfigValue oldValue = Get(key);
	 ConfigValue newValue(value);
	 
	 // Validate type if option is defined
	 if (!ValidateValueType(key, newValue)) {
		 return false;
	 }
	 
	 m_values[key] = newValue;
	 m_modified = true;
	 
	 // Notify callbacks
	 NotifyChangeCallbacks(key, oldValue, newValue);
	 
	 return true;
 }
 
 std::vector<ConfigValue> Config::GetArray(const std::string& key, 
										   const std::vector<ConfigValue>& defaultValue) const {
	 ConfigValue value = GetTypedValue(key, ConfigValueType::ARRAY);
	 if (value.GetType() != ConfigValueType::ARRAY) {
		 return defaultValue;
	 }
	 return value.AsArray();
 }
 
 bool Config::SetArray(const std::string& key, const std::vector<ConfigValue>& value) {
	 ConfigValue oldValue = Get(key);
	 ConfigValue newValue(value);
	 
	 // Validate type if option is defined
	 if (!ValidateValueType(key, newValue)) {
		 return false;
	 }
	 
	 m_values[key] = newValue;
	 m_modified = true;
	 
	 // Notify callbacks
	 NotifyChangeCallbacks(key, oldValue, newValue);
	 
	 return true;
 }
 
 ConfigValue Config::Get(const std::string& key) const {
	 auto it = m_values.find(key);
	 if (it != m_values.end()) {
		 return it->second;
	 }
	 
	 // If the option is defined, return its default value
	 auto optIt = m_options.find(key);
	 if (optIt != m_options.end()) {
		 return optIt->second.defaultValue;
	 }
	 
	 // Return an empty value
	 return ConfigValue();
 }
 
 bool Config::Set(const std::string& key, const ConfigValue& value) {
	 ConfigValue oldValue = Get(key);
	 
	 // Validate type if option is defined
	 if (!ValidateValueType(key, value)) {
		 return false;
	 }
	 
	 m_values[key] = value;
	 m_modified = true;
	 
	 // Notify callbacks
	 NotifyChangeCallbacks(key, oldValue, value);
	 
	 return true;
 }
 
 bool Config::HasOption(const std::string& key) const {
	 return m_options.find(key) != m_options.end();
 }
 
 bool Config::RemoveOption(const std::string& key) {
	 auto it = m_options.find(key);
	 if (it == m_options.end()) {
		 return false;
	 }
	 
	 m_options.erase(it);
	 
	 // Also remove any value for this option
	 auto valueIt = m_values.find(key);
	 if (valueIt != m_values.end()) {
		 m_values.erase(valueIt);
	 }
	 
	 m_modified = true;
	 return true;
 }
 
 const ConfigOption* Config::GetOptionDefinition(const std::string& key) const {
	 auto it = m_options.find(key);
	 if (it != m_options.end()) {
		 return &it->second;
	 }
	 return nullptr;
 }
 
 bool Config::RegisterOption(const ConfigOption& option) {
	 // Check if option already exists
	 auto it = m_options.find(option.key);
	 if (it != m_options.end()) {
		 m_logger.Warning("Config", "Overwriting existing option: " + option.key);
	 }
	 
	 // Add the option
	 m_options[option.key] = option;
	 
	 // If no value exists for this option, set the default
	 if (m_values.find(option.key) == m_values.end()) {
		 m_values[option.key] = option.defaultValue;
	 }
	 
	 m_logger.Info("Config", "Registered configuration option: " + option.key);
	 return true;
 }
 
 int Config::RegisterChangeCallback(std::function<void(const ConfigChangeEvent&)> callback) {
	 int callbackId = m_nextCallbackId++;
	 m_changeCallbacks[callbackId] = callback;
	 return callbackId;
 }
 
 bool Config::RemoveChangeCallback(int callbackId) {
	 auto it = m_changeCallbacks.find(callbackId);
	 if (it == m_changeCallbacks.end()) {
		 return false;
	 }
	 
	 m_changeCallbacks.erase(it);
	 return true;
 }
 
 std::vector<ConfigOption> Config::GetAllOptions() const {
	 std::vector<ConfigOption> options;
	 options.reserve(m_options.size());
	 
	 for (const auto& pair : m_options) {
		 options.push_back(pair.second);
	 }
	 
	 return options;
 }
 
 std::vector<ConfigOption> Config::GetOptionsByCategory(const std::string& category) const {
	 std::vector<ConfigOption> options;
	 
	 for (const auto& pair : m_options) {
		 if (pair.second.category == category) {
			 options.push_back(pair.second);
		 }
	 }
	 
	 return options;
 }
 
 std::vector<std::string> Config::GetCategories() const {
	 std::vector<std::string> categories;
	 std::unordered_map<std::string, bool> categoryMap;
	 
	 // Collect unique categories
	 for (const auto& pair : m_options) {
		 if (!pair.second.category.empty() && categoryMap.find(pair.second.category) == categoryMap.end()) {
			 categoryMap[pair.second.category] = true;
			 categories.push_back(pair.second.category);
		 }
	 }
	 
	 // Sort categories
	 std::sort(categories.begin(), categories.end());
	 
	 return categories;
 }
 
 std::string Config::GetConfigPath() const {
	 return m_configPath;
 }
 
 void Config::SetConfigPath(const std::string& path) {
	 m_configPath = path;
	 m_logger.Info("Config", "Configuration path changed to: " + path);
 }
 
 void Config::DefineDefaultOptions() {
	 m_logger.Info("Config", "Defining default configuration options");
	 
	 // System options
	 RegisterOption({
		 "system.hardware_variant",
		 "Hardware Variant",
		 "Hardware variant to emulate (original or plus)",
		 ConfigValueType::STRING,
		 ConfigValue("original"),
		 {"original", "plus"},
		 true,
		 "System"
	 });
	 
	 RegisterOption({
		 "system.region",
		 "Region",
		 "Emulated hardware region",
		 ConfigValueType::STRING,
		 ConfigValue("us"),
		 {"us", "jp", "eu"},
		 true,
		 "System"
	 });
	 
	 // Display options
	 RegisterOption({
		 "display.width",
		 "Display Width",
		 "Window width in pixels",
		 ConfigValueType::INTEGER,
		 ConfigValue(800),
		 {},
		 false,
		 "Display"
	 });
	 
	 RegisterOption({
		 "display.height",
		 "Display Height",
		 "Window height in pixels",
		 ConfigValueType::INTEGER,
		 ConfigValue(600),
		 {},
		 false,
		 "Display"
	 });
	 
	 RegisterOption({
		 "display.fullscreen",
		 "Fullscreen Mode",
		 "Start in fullscreen mode",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(false),
		 {},
		 false,
		 "Display"
	 });
	 
	 RegisterOption({
		 "display.vsync",
		 "Vertical Sync",
		 "Enable vertical synchronization",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(true),
		 {},
		 false,
		 "Display"
	 });
	 
	 RegisterOption({
		 "display.scaling",
		 "Scaling Mode",
		 "Scaling algorithm to use",
		 ConfigValueType::STRING,
		 ConfigValue("nearest"),
		 {"nearest", "linear", "best"},
		 false,
		 "Display"
	 });
	 
	 RegisterOption({
		 "display.aspect_ratio",
		 "Aspect Ratio",
		 "Aspect ratio handling mode",
		 ConfigValueType::STRING,
		 ConfigValue("maintain"),
		 {"stretch", "maintain", "pixel_perfect"},
		 false,
		 "Display"
	 });
	 
	 RegisterOption({
		 "display.scanlines",
		 "Scanline Effect",
		 "Enable CRT scanline effect",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(false),
		 {},
		 false,
		 "Display"
	 });
	 
	 RegisterOption({
		 "display.scanline_intensity",
		 "Scanline Intensity",
		 "Intensity of scanline effect (0-100)",
		 ConfigValueType::INTEGER,
		 ConfigValue(50),
		 {},
		 false,
		 "Display"
	 });
	 
	 RegisterOption({
		 "display.crt_effect",
		 "CRT Effect",
		 "Enable CRT screen curvature effect",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(false),
		 {},
		 false,
		 "Display"
	 });
	 
	 RegisterOption({
		 "display.show_fps",
		 "Show FPS",
		 "Display frames per second counter",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(false),
		 {},
		 false,
		 "Display"
	 });
	 
	 // Audio options
	 RegisterOption({
		 "audio.enabled",
		 "Audio Enabled",
		 "Enable audio output",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(true),
		 {},
		 false,
		 "Audio"
	 });
	 
	 RegisterOption({
		 "audio.volume",
		 "Master Volume",
		 "Master volume level (0-100)",
		 ConfigValueType::INTEGER,
		 ConfigValue(80),
		 {},
		 false,
		 "Audio"
	 });
	 
	 RegisterOption({
		 "audio.sample_rate",
		 "Sample Rate",
		 "Audio sample rate in Hz",
		 ConfigValueType::INTEGER,
		 ConfigValue(44100),
		 {},
		 true,
		 "Audio"
	 });
	 
	 RegisterOption({
		 "audio.buffer_size",
		 "Buffer Size",
		 "Audio buffer size in samples",
		 ConfigValueType::INTEGER,
		 ConfigValue(1024),
		 {},
		 true,
		 "Audio"
	 });
	 
	 RegisterOption({
		 "audio.resampling_quality",
		 "Resampling Quality",
		 "Audio resampling quality",
		 ConfigValueType::STRING,
		 ConfigValue("medium"),
		 {"low", "medium", "high"},
		 false,
		 "Audio"
	 });
	 
	 // Input options
	 RegisterOption({
		 "input.keyboard_layout",
		 "Keyboard Layout",
		 "Keyboard mapping layout",
		 ConfigValueType::STRING,
		 ConfigValue("us"),
		 {"us", "jp", "eu"},
		 false,
		 "Input"
	 });
	 
	 // Debug options
	 RegisterOption({
		 "debug.log_level",
		 "Log Level",
		 "Minimum log level to display",
		 ConfigValueType::STRING,
		 ConfigValue("info"),
		 {"debug", "info", "warning", "error", "fatal"},
		 false,
		 "Debug"
	 });
	 
	 RegisterOption({
		 "debug.log_to_file",
		 "Log to File",
		 "Save logs to file",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(false),
		 {},
		 false,
		 "Debug"
	 });
	 
	 RegisterOption({
		 "debug.log_file_path",
		 "Log File Path",
		 "Path to log file",
		 ConfigValueType::STRING,
		 ConfigValue("nixx32_log.txt"),
		 {},
		 false,
		 "Debug"
	 });
	 
	 // ROM options
	 RegisterOption({
		 "rom.path",
		 "ROM Path",
		 "Default path for ROM files",
		 ConfigValueType::STRING,
		 ConfigValue("roms"),
		 {},
		 false,
		 "ROM"
	 });
	 
	 RegisterOption({
		 "rom.validate_checksum",
		 "Validate Checksums",
		 "Validate ROM checksums when loading",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(true),
		 {},
		 false,
		 "ROM"
	 });
	 
	 // Save state options
	 RegisterOption({
		 "save.path",
		 "Save State Path",
		 "Default path for save states",
		 ConfigValueType::STRING,
		 ConfigValue("saves"),
		 {},
		 false,
		 "Save States"
	 });
	 
	 RegisterOption({
		 "save.auto_save",
		 "Auto Save",
		 "Automatically save state on exit",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(false),
		 {},
		 false,
		 "Save States"
	 });
	 
	 RegisterOption({
		 "save.auto_load",
		 "Auto Load",
		 "Automatically load previous state on startup",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(false),
		 {},
		 false,
		 "Save States"
	 });
	 
	 // Performance options
	 RegisterOption({
		 "performance.emulation_speed",
		 "Emulation Speed",
		 "Emulation speed factor",
		 ConfigValueType::STRING,
		 ConfigValue("normal"),
		 {"slowest", "slow", "normal", "fast", "fastest", "unlimited"},
		 false,
		 "Performance"
	 });
	 
	 RegisterOption({
		 "performance.throttle_fps",
		 "Throttle FPS",
		 "Limit frame rate to hardware spec",
		 ConfigValueType::BOOLEAN,
		 ConfigValue(true),
		 {},
		 false,
		 "Performance"
	 });
	 
	 RegisterOption({
		 "performance.frame_skip",
		 "Frame Skip",
		 "Number of frames to skip for performance",
		 ConfigValueType::INTEGER,
		 ConfigValue(0),
		 {},
		 false,
		 "Performance"
	 });
 }
 
 bool Config::ParseConfigFile(const std::string& fileContent) {
	 std::istringstream stream(fileContent);
	 std::string line;
	 std::string currentCategory;
	 
	 while (std::getline(stream, line)) {
		 // Skip empty lines and comments
		 if (line.empty() || line[0] == '#' || line[0] == ';') {
			 continue;
		 }
		 
		 // Trim leading and trailing whitespace
		 line.erase(0, line.find_first_not_of(" \t\r\n"));
		 line.erase(line.find_last_not_of(" \t\r\n") + 1);
		 
		 // Check if this is a category line [Category]
		 if (line[0] == '[' && line[line.length() - 1] == ']') {
			 currentCategory = line.substr(1, line.length() - 2);
			 continue;
		 }
		 
		 // Look for key=value pair
		 size_t equalPos = line.find('=');
		 if (equalPos != std::string::npos) {
			 std::string key = line.substr(0, equalPos);
			 std::string value = line.substr(equalPos + 1);
			 
			 // Trim key and value
			 key.erase(0, key.find_first_not_of(" \t"));
			 key.erase(key.find_last_not_of(" \t") + 1);
			 value.erase(0, value.find_first_not_of(" \t"));
			 value.erase(value.find_last_not_of(" \t") + 1);
			 
			 // If we have a category prefix, use it
			 std::string fullKey = key;
			 if (!currentCategory.empty()) {
				 fullKey = currentCategory + "." + key;
			 }
			 
			 // Check if this option is defined and get its type
			 auto it = m_options.find(fullKey);
			 if (it != m_options.end()) {
				 ConfigValueType type = it->second.type;
				 m_values[fullKey] = ConfigValue::FromString(value, type);
			 } else {
				 // If option is not defined, try to guess the type
				 if (value == "true" || value == "false") {
					 m_values[fullKey] = ConfigValue(value == "true");
				 } else if (value.find('.') != std::string::npos) {
					 // Try as float
					 try {
						 m_values[fullKey] = ConfigValue(std::stof(value));
					 } catch (...) {
						 m_values[fullKey] = ConfigValue(value);
					 }
				 } else if (value.find_first_not_of("0123456789-") == std::string::npos) {
					 // Try as integer
					 try {
						 m_values[fullKey] = ConfigValue(std::stoi(value));
					 } catch (...) {
						 m_values[fullKey] = ConfigValue(value);
					 }
				 } else {
					 // Default to string
					 m_values[fullKey] = ConfigValue(value);
				 }
			 }
		 }
	 }
	 
	 m_logger.Info("Config", "Parsed configuration file successfully");
	 return true;
 }
 
 std::string Config::GenerateConfigFile() const {
	 std::stringstream ss;
	 
	 // Add header
	 ss << "# NiXX-32 Emulator Configuration" << std::endl;
	 ss << "# Generated on " << std::time(nullptr) << std::endl;
	 ss << std::endl;
	 
	 // Group by category
	 std::unordered_map<std::string, std::vector<std::string>> categorizedOptions;
	 
	 // First, collect all options by category
	 for (const auto& pair : m_options) {
		 const std::string& key = pair.first;
		 const ConfigOption& option = pair.second;
		 
		 categorizedOptions[option.category].push_back(key);
	 }
	 
	 // Get all categories and sort them
	 std::vector<std::string> categories;
	 for (const auto& pair : categorizedOptions) {
		 categories.push_back(pair.first);
	 }
	 
	 std::sort(categories.begin(), categories.end());
	 
	 // Process each category
	 for (const std::string& category : categories) {
		 // Add category header
		 ss << "[" << category << "]" << std::endl;
		 
		 // Sort options within category
		 std::vector<std::string>& keys = categorizedOptions[category];
		 std::sort(keys.begin(), keys.end());
		 
		 // Add each option
		 for (const std::string& key : keys) {
			 // Get option
			 const ConfigOption& option = m_options.at(key);
			 
			 // Add comment if available
			 if (!option.description.empty()) {
				 ss << "# " << option.description << std::endl;
			 }
			 
			 // Add possible values if available
			 if (!option.enumValues.empty()) {
				 ss << "# Possible values: ";
				 for (size_t i = 0; i < option.enumValues.size(); ++i) {
					 if (i > 0) ss << ", ";
					 ss << option.enumValues[i];
				 }
				 ss << std::endl;
			 }
			 
			 // Extract key without category prefix
			 std::string shortKey = key;
			 size_t dotPos = key.find('.');
			 if (dotPos != std::string::npos) {
				 shortKey = key.substr(dotPos + 1);
			 }
			 
			 // Get value
			 ConfigValue value = Get(key);
			 
			 // Write key=value
			 ss << shortKey << " = " << value.ToString() << std::endl;
		 }
		 
		 // Add spacing between categories
		 ss << std::endl;
	 }
	 
	 return ss.str();
 }
 
 void Config::NotifyChangeCallbacks(const std::string& key, 
								   const ConfigValue& oldValue,
								   const ConfigValue& newValue) {
	 ConfigChangeEvent event;
	 event.key = key;
	 event.oldValue = oldValue;
	 event.newValue = newValue;
	 
	 for (const auto& pair : m_changeCallbacks) {
		 try {
			 pair.second(event);
		 } catch (const std::exception& e) {
			 m_logger.Error("Config", "Exception in change callback: " + std::string(e.what()));
		 } catch (...) {
			 m_logger.Error("Config", "Unknown exception in change callback");
		 }
	 }
 }
 
 ConfigValue Config::GetTypedValue(const std::string& key, ConfigValueType type) const {
	 auto it = m_values.find(key);
	 if (it != m_values.end()) {
		 // If the value has the correct type, return it
		 if (it->second.GetType() == type) {
			 return it->second;
		 }
		 
		 // Otherwise, try to convert
		 switch (type) {
			 case ConfigValueType::BOOLEAN:
				 return ConfigValue(it->second.AsBool());
			 
			 case ConfigValueType::INTEGER:
				 return ConfigValue(it->second.AsInt());
			 
			 case ConfigValueType::FLOAT:
				 return ConfigValue(it->second.AsFloat());
			 
			 case ConfigValueType::STRING:
				 return ConfigValue(it->second.AsString());
			 
			 case ConfigValueType::ARRAY:
				 return ConfigValue(it->second.AsArray());
		 }
	 }
	 
	 // If the option is defined, return its default value
	 auto optIt = m_options.find(key);
	 if (optIt != m_options.end() && optIt->second.type == type) {
		 return optIt->second.defaultValue;
	 }
	 
	 // Return an empty value of the requested type
	 switch (type) {
		 case ConfigValueType::BOOLEAN:
			 return ConfigValue(false);
		 
		 case ConfigValueType::INTEGER:
			 return ConfigValue(0);
		 
		 case ConfigValueType::FLOAT:
			 return ConfigValue(0.0f);
		 
		 case ConfigValueType::STRING:
			 return ConfigValue("");
		 
		 case ConfigValueType::ARRAY:
			 return ConfigValue(std::vector<ConfigValue>());
	 }
	 
	 // Default case
	 return ConfigValue();
 }
 
 bool Config::ValidateValueType(const std::string& key, const ConfigValue& value) const {
	 auto it = m_options.find(key);
	 if (it == m_options.end()) {
		 // Option is not defined, so no validation needed
		 return true;
	 }
	 
	 const ConfigOption& option = it->second;
	 
	 // Validate type
	 if (option.type != value.GetType()) {
		 m_logger.Warning("Config", "Type mismatch for option " + key + 
						 ": expected " + std::to_string(static_cast<int>(option.type)) + 
						 ", got " + std::to_string(static_cast<int>(value.GetType())));
		 return false;
	 }
	 
	 // For enum-like options, validate the value is in the allowed list
	 if (option.type == ConfigValueType::STRING && !option.enumValues.empty()) {
		 std::string strValue = value.AsString();
		 bool valid = false;
		 
		 for (const std::string& enumValue : option.enumValues) {
			 if (strValue == enumValue) {
				 valid = true;
				 break;
			 }
		 }
		 
		 if (!valid) {
			 m_logger.Warning("Config", "Invalid value for option " + key + 
							 ": " + strValue + " is not in the allowed values list");
			 return false;
		 }
	 }
	 
	 return true;
 }
 
 } // namespace NiXX32