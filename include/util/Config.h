/**
 * Config.h
 * Persistent configuration system for NiXX-32 arcade board emulation
 * 
 * This file defines the configuration system that handles loading, saving,
 * and managing emulator settings for the NiXX-32 arcade board emulation.
 * It supports various configuration options for graphics, audio, input,
 * and system behavior.
 */

 #pragma once

 #include <string>
 #include <unordered_map>
 #include <vector>
 #include <memory>
 #include <functional>
 
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 
 /**
  * Configuration value types
  */
 enum class ConfigValueType {
	 BOOLEAN,    // Boolean value (true/false)
	 INTEGER,    // Integer value
	 FLOAT,      // Floating point value
	 STRING,     // String value
	 ARRAY       // Array of values
 };
 
 /**
  * Configuration value container
  */
 class ConfigValue {
 public:
	 /**
	  * Default constructor
	  */
	 ConfigValue();
	 
	 /**
	  * Constructor for boolean value
	  * @param value Boolean value
	  */
	 explicit ConfigValue(bool value);
	 
	 /**
	  * Constructor for integer value
	  * @param value Integer value
	  */
	 explicit ConfigValue(int value);
	 
	 /**
	  * Constructor for float value
	  * @param value Float value
	  */
	 explicit ConfigValue(float value);
	 
	 /**
	  * Constructor for string value
	  * @param value String value
	  */
	 explicit ConfigValue(const std::string& value);
	 
	 /**
	  * Constructor for array value
	  * @param values Vector of values
	  */
	 explicit ConfigValue(const std::vector<ConfigValue>& values);
	 
	 /**
	  * Destructor
	  */
	 ~ConfigValue();
	 
	 /**
	  * Get the value type
	  * @return Value type
	  */
	 ConfigValueType GetType() const;
	 
	 /**
	  * Get boolean value
	  * @return Boolean value, or false if not a boolean
	  */
	 bool AsBool() const;
	 
	 /**
	  * Get integer value
	  * @return Integer value, or 0 if not an integer
	  */
	 int AsInt() const;
	 
	 /**
	  * Get float value
	  * @return Float value, or 0.0 if not a float
	  */
	 float AsFloat() const;
	 
	 /**
	  * Get string value
	  * @return String value, or empty string if not a string
	  */
	 std::string AsString() const;
	 
	 /**
	  * Get array value
	  * @return Array value, or empty array if not an array
	  */
	 std::vector<ConfigValue> AsArray() const;
	 
	 /**
	  * Convert value to string representation
	  * @return String representation
	  */
	 std::string ToString() const;
	 
	 /**
	  * Parse string as config value
	  * @param str String to parse
	  * @param type Value type
	  * @return Parsed config value
	  */
	 static ConfigValue FromString(const std::string& str, ConfigValueType type);
 
 private:
	 // Value type
	 ConfigValueType m_type;
	 
	 // Union of possible values
	 union {
		 bool m_boolValue;
		 int m_intValue;
		 float m_floatValue;
	 };
	 
	 // String value (not in union due to non-POD type)
	 std::string m_stringValue;
	 
	 // Array value (not in union due to non-POD type)
	 std::vector<ConfigValue> m_arrayValue;
 };
 
 /**
  * Configuration option definition
  */
 struct ConfigOption {
	 std::string key;               // Option key
	 std::string name;              // Human-readable name
	 std::string description;       // Option description
	 ConfigValueType type;          // Value type
	 ConfigValue defaultValue;      // Default value
	 std::vector<std::string> enumValues; // Possible values for enum-like options
	 bool requiresRestart;          // Whether option change requires restart
	 std::string category;          // Option category
 };
 
 /**
  * Configuration change event
  */
 struct ConfigChangeEvent {
	 std::string key;               // Changed option key
	 ConfigValue oldValue;          // Previous value
	 ConfigValue newValue;          // New value
 };
 
 /**
  * Main configuration system class
  */
 class Config {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param logger Reference to the system logger
	  * @param configPath Path to configuration file
	  */
	 Config(System& system, Logger& logger, const std::string& configPath = "nixx32_config.ini");
	 
	 /**
	  * Destructor
	  */
	 ~Config();
	 
	 /**
	  * Initialize the configuration system
	  * @return True if initialization was successful
	  */
	 bool Initialize();
	 
	 /**
	  * Load configuration from file
	  * @return True if loading was successful
	  */
	 bool LoadFromFile();
	 
	 /**
	  * Save configuration to file
	  * @return True if saving was successful
	  */
	 bool SaveToFile();
	 
	 /**
	  * Reset configuration to default values
	  */
	 void ResetToDefaults();
	 
	 /**
	  * Get boolean value
	  * @param key Option key
	  * @param defaultValue Default value if option not found
	  * @return Boolean value
	  */
	 bool GetBool(const std::string& key, bool defaultValue = false) const;
	 
	 /**
	  * Set boolean value
	  * @param key Option key
	  * @param value Boolean value
	  * @return True if successful
	  */
	 bool SetBool(const std::string& key, bool value);
	 
	 /**
	  * Get integer value
	  * @param key Option key
	  * @param defaultValue Default value if option not found
	  * @return Integer value
	  */
	 int GetInt(const std::string& key, int defaultValue = 0) const;
	 
	 /**
	  * Set integer value
	  * @param key Option key
	  * @param value Integer value
	  * @return True if successful
	  */
	 bool SetInt(const std::string& key, int value);
	 
	 /**
	  * Get float value
	  * @param key Option key
	  * @param defaultValue Default value if option not found
	  * @return Float value
	  */
	 float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
	 
	 /**
	  * Set float value
	  * @param key Option key
	  * @param value Float value
	  * @return True if successful
	  */
	 bool SetFloat(const std::string& key, float value);
	 
	 /**
	  * Get string value
	  * @param key Option key
	  * @param defaultValue Default value if option not found
	  * @return String value
	  */
	 std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
	 
	 /**
	  * Set string value
	  * @param key Option key
	  * @param value String value
	  * @return True if successful
	  */
	 bool SetString(const std::string& key, const std::string& value);
	 
	 /**
	  * Get array value
	  * @param key Option key
	  * @param defaultValue Default value if option not found
	  * @return Array value
	  */
	 std::vector<ConfigValue> GetArray(const std::string& key, 
									  const std::vector<ConfigValue>& defaultValue = {}) const;
	 
	 /**
	  * Set array value
	  * @param key Option key
	  * @param value Array value
	  * @return True if successful
	  */
	 bool SetArray(const std::string& key, const std::vector<ConfigValue>& value);
	 
	 /**
	  * Get generic value
	  * @param key Option key
	  * @return Config value, or empty value if not found
	  */
	 ConfigValue Get(const std::string& key) const;
	 
	 /**
	  * Set generic value
	  * @param key Option key
	  * @param value Config value
	  * @return True if successful
	  */
	 bool Set(const std::string& key, const ConfigValue& value);
	 
	 /**
	  * Check if option exists
	  * @param key Option key
	  * @return True if option exists
	  */
	 bool HasOption(const std::string& key) const;
	 
	 /**
	  * Remove an option
	  * @param key Option key
	  * @return True if option was removed
	  */
	 bool RemoveOption(const std::string& key);
	 
	 /**
	  * Get option definition
	  * @param key Option key
	  * @return Option definition, or nullptr if not defined
	  */
	 const ConfigOption* GetOptionDefinition(const std::string& key) const;
	 
	 /**
	  * Register option definition
	  * @param option Option definition
	  * @return True if registration was successful
	  */
	 bool RegisterOption(const ConfigOption& option);
	 
	 /**
	  * Register configuration change callback
	  * @param callback Function to call when configuration changes
	  * @return Callback ID
	  */
	 int RegisterChangeCallback(std::function<void(const ConfigChangeEvent&)> callback);
	 
	 /**
	  * Remove configuration change callback
	  * @param callbackId Callback ID
	  * @return True if successful
	  */
	 bool RemoveChangeCallback(int callbackId);
	 
	 /**
	  * Get all registered options
	  * @return Vector of option definitions
	  */
	 std::vector<ConfigOption> GetAllOptions() const;
	 
	 /**
	  * Get options by category
	  * @param category Category name
	  * @return Vector of option definitions in the category
	  */
	 std::vector<ConfigOption> GetOptionsByCategory(const std::string& category) const;
	 
	 /**
	  * Get all available categories
	  * @return Vector of category names
	  */
	 std::vector<std::string> GetCategories() const;
	 
	 /**
	  * Get configuration file path
	  * @return Configuration file path
	  */
	 std::string GetConfigPath() const;
	 
	 /**
	  * Set configuration file path
	  * @param path New configuration file path
	  */
	 void SetConfigPath(const std::string& path);
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Configuration file path
	 std::string m_configPath;
	 
	 // Configuration values
	 std::unordered_map<std::string, ConfigValue> m_values;
	 
	 // Option definitions
	 std::unordered_map<std::string, ConfigOption> m_options;
	 
	 // Change callbacks
	 std::unordered_map<int, std::function<void(const ConfigChangeEvent&)>> m_changeCallbacks;
	 int m_nextCallbackId;
	 
	 // Modification flag
	 bool m_modified;
	 
	 /**
	  * Define default configuration options
	  */
	 void DefineDefaultOptions();
	 
	 /**
	  * Parse configuration file
	  * @param fileContent Configuration file content
	  * @return True if parsing was successful
	  */
	 bool ParseConfigFile(const std::string& fileContent);
	 
	 /**
	  * Generate configuration file content
	  * @return Configuration file content
	  */
	 std::string GenerateConfigFile() const;
	 
	 /**
	  * Notify change callbacks
	  * @param key Changed option key
	  * @param oldValue Previous value
	  * @param newValue New value
	  */
	 void NotifyChangeCallbacks(const std::string& key, 
							   const ConfigValue& oldValue,
							   const ConfigValue& newValue);
	 
	 /**
	  * Get value of correct type
	  * @param key Option key
	  * @param type Value type
	  * @return Config value of specified type, or default value if not found or wrong type
	  */
	 ConfigValue GetTypedValue(const std::string& key, ConfigValueType type) const;
	 
	 /**
	  * Check if value type matches option definition
	  * @param key Option key
	  * @param value Value to check
	  * @return True if types match
	  */
	 bool ValidateValueType(const std::string& key, const ConfigValue& value) const;
 };
 
 } // namespace NiXX32