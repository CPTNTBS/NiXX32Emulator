/**
 * InputSystem.h
 * Input handling system for NiXX-32 arcade board emulation
 * 
 * This file defines the input system that handles arcade controls including
 * joysticks, buttons, coin mechanisms, and specialized racing controls for
 * the NiXX-32 arcade hardware. It supports both the original and enhanced
 * hardware variants with different control configurations for racing games.
 */

 #pragma once

 #include <cstdint>
 #include <vector>
 #include <string>
 #include <functional>
 #include <memory>
 #include <unordered_map>
 
 #include "MemoryManager.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 
 /**
  * Input device types
  */
 enum class InputDeviceType {
	 JOYSTICK,       // Standard 8-way joystick
	 SPINNER,        // Rotary spinner control
	 STEERING_WHEEL, // Racing steering wheel
	 PEDALS,         // Accelerator and brake pedals
	 SHIFTER,        // Manual transmission shifter
	 BUTTONS,        // Standard button controls
	 SERVICE,        // Service/operator controls
	 COIN            // Coin mechanism
 };
 
 /**
  * Joystick directions
  */
 enum JoystickDirection {
	 NONE  = 0x00,
	 UP    = 0x01,
	 DOWN  = 0x02,
	 LEFT  = 0x04,
	 RIGHT = 0x08,
	 // Diagonal directions
	 UP_LEFT    = UP | LEFT,
	 UP_RIGHT   = UP | RIGHT,
	 DOWN_LEFT  = DOWN | LEFT,
	 DOWN_RIGHT = DOWN | RIGHT
 };
 
 /**
  * Button states
  */
 enum ButtonState {
	 RELEASED = 0,
	 PRESSED  = 1
 };
 
 /**
  * Standard arcade buttons
  */
 enum StandardButton {
	 BUTTON_1 = 0,  // Primary action button
	 BUTTON_2,      // Secondary action button
	 BUTTON_3,      // Tertiary action button
	 BUTTON_4,      // Quaternary action button
	 BUTTON_5,      // Additional button
	 BUTTON_6,      // Additional button
	 BUTTON_START,  // Start game
	 BUTTON_SELECT, // Select/menu navigation
	 BUTTON_MAX     // Total number of standard buttons
 };
 
 /**
  * Service controls
  */
 enum ServiceControl {
	 SERVICE_BUTTON,   // Service button
	 TEST_BUTTON,      // Test mode button
	 TILT_SENSOR,      // Tilt/shock sensor
	 DOOR_SWITCH,      // Cabinet door switch
	 SERVICE_MAX       // Total number of service controls
 };
 
 /**
  * Coin mechanism controls
  */
 enum CoinControl {
	 COIN_1,           // Coin slot 1
	 COIN_2,           // Coin slot 2
	 COIN_COUNTER_1,   // Coin counter 1
	 COIN_COUNTER_2,   // Coin counter 2
	 COIN_LOCKOUT_1,   // Coin lockout solenoid 1
	 COIN_LOCKOUT_2,   // Coin lockout solenoid 2
	 COIN_MAX          // Total number of coin controls
 };
 
 /**
  * Racing controls
  */
 enum RacingControl {
	 STEERING,         // Steering wheel position (-128 to 127)
	 ACCELERATOR,      // Accelerator pedal (0-255)
	 BRAKE,            // Brake pedal (0-255)
	 CLUTCH,           // Clutch pedal (0-255, NiXX-32+ only)
	 SHIFTER_GEAR,     // Shifter position (0=Neutral, 1-5=Gears, 6=Reverse)
	 SHIFTER_UP,       // Paddle shifter up
	 SHIFTER_DOWN,     // Paddle shifter down
	 HANDBRAKE,        // Handbrake control (NiXX-32+ only)
	 BOOST_BUTTON,     // Nitro/boost button
	 RACING_MAX        // Total number of racing controls
 };
 
 /**
  * Input device configurations
  */
 struct InputConfig {
	 bool racingControlsEnabled;   // Whether racing-specific controls are enabled
	 bool analogControlsEnabled;   // Whether analog controls are enabled
	 uint8_t numPlayers;           // Number of players supported
	 bool forceFeedbackSupported;  // Whether force feedback is supported (NiXX-32+ only)
	 bool lightgunSupported;       // Whether lightgun is supported
 };
 
 /**
  * Input device state structure
  */
 struct InputState {
	 // Joystick state
	 JoystickDirection joystickDirection;
	 
	 // Button states
	 bool buttons[BUTTON_MAX];
	 
	 // Racing controls
	 int8_t steeringPosition;   // Steering wheel position (-128 to 127)
	 uint8_t acceleratorValue;  // Accelerator pedal position (0-255)
	 uint8_t brakeValue;        // Brake pedal position (0-255)
	 uint8_t clutchValue;       // Clutch pedal position (0-255)
	 uint8_t shifterGear;       // Gear position (0=Neutral, 1-5=Gears, 6=Reverse)
	 bool shifterUp;            // Paddle shifter up
	 bool shifterDown;          // Paddle shifter down
	 bool handbrake;            // Handbrake state
	 bool boostButton;          // Nitro/boost button state
	 
	 // Service controls
	 bool serviceControls[SERVICE_MAX];
	 
	 // Coin controls
	 bool coinControls[COIN_MAX];
	 
	 // Specialized inputs (NiXX-32+ only)
	 struct {
		 uint16_t lightgunX;    // Lightgun X position
		 uint16_t lightgunY;    // Lightgun Y position
		 bool lightgunTrigger;  // Lightgun trigger state
	 } specialized;
 };
 
 /**
  * Force feedback effects (NiXX-32+ only)
  */
 enum class ForceFeedbackEffect {
	 NONE,                // No effect
	 CONSTANT,            // Constant force
	 SPRING,              // Spring effect
	 FRICTION,            // Friction effect
	 VIBRATION,           // Vibration effect
	 RUMBLE,              // Rumble effect
	 COLLISION,           // Collision effect
	 ROAD_TEXTURE,        // Road texture effect
	 CUSTOM               // Custom effect
 };
 
 /**
  * Main class that manages the NiXX-32 input system
  */
 class InputSystem {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 InputSystem(System& system, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~InputSystem();
	 
	 /**
	  * Initialize the input system
	  * @param variant Hardware variant to configure for
	  * @return True if initialization was successful
	  */
	 bool Initialize(HardwareVariant variant);
	 
	 /**
	  * Reset the input system to initial state
	  */
	 void Reset();
	 
	 /**
	  * Update input state for the current frame
	  * @param deltaTime Time elapsed since last update in milliseconds
	  */
	 void Update(float deltaTime);
	 
	 /**
	  * Configure the input system
	  * @param config Input configuration
	  */
	 void Configure(const InputConfig& config);
	 
	 /**
	  * Get the input configuration
	  * @return Input configuration
	  */
	 const InputConfig& GetConfig() const;
	 
	 /**
	  * Get the current input state for a player
	  * @param playerIndex Player index (0-based)
	  * @return Input state for the specified player
	  */
	 const InputState& GetInputState(uint8_t playerIndex = 0) const;
	 
	 /**
	  * Set joystick direction for a player
	  * @param direction Joystick direction
	  * @param playerIndex Player index (0-based)
	  */
	 void SetJoystickDirection(JoystickDirection direction, uint8_t playerIndex = 0);
	 
	 /**
	  * Set button state for a player
	  * @param button Button ID
	  * @param state Button state (true for pressed, false for released)
	  * @param playerIndex Player index (0-based)
	  */
	 void SetButtonState(uint8_t button, bool state, uint8_t playerIndex = 0);
	 
	 /**
	  * Set steering wheel position for a player
	  * @param position Steering position (-128 to 127, 0 is center)
	  * @param playerIndex Player index (0-based)
	  */
	 void SetSteeringPosition(int8_t position, uint8_t playerIndex = 0);
	 
	 /**
	  * Set accelerator pedal value for a player
	  * @param value Accelerator value (0-255)
	  * @param playerIndex Player index (0-based)
	  */
	 void SetAcceleratorValue(uint8_t value, uint8_t playerIndex = 0);
	 
	 /**
	  * Set brake pedal value for a player
	  * @param value Brake value (0-255)
	  * @param playerIndex Player index (0-based)
	  */
	 void SetBrakeValue(uint8_t value, uint8_t playerIndex = 0);
	 
	 /**
	  * Set clutch pedal value for a player (NiXX-32+ only)
	  * @param value Clutch value (0-255)
	  * @param playerIndex Player index (0-based)
	  */
	 void SetClutchValue(uint8_t value, uint8_t playerIndex = 0);
	 
	 /**
	  * Set shifter gear for a player
	  * @param gear Gear position (0=Neutral, 1-5=Gears, 6=Reverse)
	  * @param playerIndex Player index (0-based)
	  */
	 void SetShifterGear(uint8_t gear, uint8_t playerIndex = 0);
	 
	 /**
	  * Set paddle shifter state for a player
	  * @param up Up paddle state
	  * @param down Down paddle state
	  * @param playerIndex Player index (0-based)
	  */
	 void SetPaddleShifterState(bool up, bool down, uint8_t playerIndex = 0);
	 
	 /**
	  * Set handbrake state for a player (NiXX-32+ only)
	  * @param state Handbrake state
	  * @param playerIndex Player index (0-based)
	  */
	 void SetHandbrakeState(bool state, uint8_t playerIndex = 0);
	 
	 /**
	  * Set boost button state for a player
	  * @param state Boost button state
	  * @param playerIndex Player index (0-based)
	  */
	 void SetBoostButtonState(bool state, uint8_t playerIndex = 0);
	 
	 /**
	  * Set service control state
	  * @param control Service control ID
	  * @param state Control state
	  */
	 void SetServiceControlState(uint8_t control, bool state);
	 
	 /**
	  * Set coin control state
	  * @param control Coin control ID
	  * @param state Control state
	  */
	 void SetCoinControlState(uint8_t control, bool state);
	 
	 /**
	  * Insert a coin into a coin slot
	  * @param slotIndex Coin slot index (0-based)
	  */
	 void InsertCoin(uint8_t slotIndex = 0);
	 
	 /**
	  * Set lightgun position (NiXX-32+ only)
	  * @param x X position
	  * @param y Y position
	  * @param trigger Trigger state
	  * @param playerIndex Player index (0-based)
	  */
	 void SetLightgunPosition(uint16_t x, uint16_t y, bool trigger, uint8_t playerIndex = 0);
	 
	 /**
	  * Apply force feedback effect (NiXX-32+ only)
	  * @param effect Effect type
	  * @param strength Effect strength (0-255)
	  * @param duration Effect duration in milliseconds (0 = continuous)
	  * @param playerIndex Player index (0-based)
	  * @return True if effect was applied successfully
	  */
	 bool ApplyForceFeedback(ForceFeedbackEffect effect, uint8_t strength, 
						   uint16_t duration = 0, uint8_t playerIndex = 0);
	 
	 /**
	  * Stop force feedback (NiXX-32+ only)
	  * @param playerIndex Player index (0-based)
	  */
	 void StopForceFeedback(uint8_t playerIndex = 0);
	 
	 /**
	  * Register an input callback
	  * @param callback Function to call when input state changes
	  * @return Callback ID for later removal
	  */
	 int RegisterInputCallback(std::function<void(uint8_t, const InputState&)> callback);
	 
	 /**
	  * Remove an input callback
	  * @param callbackId Callback ID returned from RegisterInputCallback
	  * @return True if callback was removed successfully
	  */
	 bool RemoveInputCallback(int callbackId);
	 
	 /**
	  * Handle input register write
	  * @param address Register address
	  * @param value Value to write
	  */
	 void HandleRegisterWrite(uint32_t address, uint8_t value);
	 
	 /**
	  * Read from input register
	  * @param address Register address
	  * @return Register value
	  */
	 uint8_t HandleRegisterRead(uint32_t address);
	 
	 /**
	  * Check if a specific input device is supported
	  * @param deviceType Device type to check
	  * @return True if the device is supported
	  */
	 bool IsDeviceSupported(InputDeviceType deviceType) const;
	 
	 /**
	  * Get the number of supported players
	  * @return Number of players
	  */
	 uint8_t GetPlayerCount() const;

	 // Power management Method (TODO: Document)
	 uint8_t GetActiveChannelCount() const;	 

 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Input configuration
	 InputConfig m_config;
	 
	 // Input states for each player
	 std::vector<InputState> m_playerStates;
	 
	 // Hardware variant
	 HardwareVariant m_variant;
	 
	 // Force feedback state (NiXX-32+ only)
	 struct ForceFeedbackState {
		 bool active;
		 ForceFeedbackEffect currentEffect;
		 uint8_t strength;
		 uint16_t duration;
		 uint16_t remainingTime;
	 };
	 std::vector<ForceFeedbackState> m_forceFeedback;
	 
	 // Coin counters
	 uint16_t m_coinCounters[2];
	 
	 // Input callbacks
	 int m_nextCallbackId;
	 std::unordered_map<int, std::function<void(uint8_t, const InputState&)>> m_callbacks;
	 
	 // Hardware registers
	 struct {
		 uint8_t control;           // Input control register
		 uint8_t status;            // Input status register
		 uint8_t playerSelect;      // Player select register
		 uint8_t joystickData;      // Joystick data register
		 uint8_t buttonData;        // Button data register
		 uint8_t racingControlData; // Racing control data register
		 uint8_t serviceData;       // Service control data register
		 uint8_t coinData;          // Coin control data register
		 uint8_t lightgunData[2];   // Lightgun data registers (X/Y)
		 uint8_t ffControl;         // Force feedback control register
		 uint8_t ffStrength;        // Force feedback strength register
		 uint8_t ffDuration[2];     // Force feedback duration registers (low/high)
	 } m_registers;
	 
	 /**
	  * Configure input system for original NiXX-32 hardware
	  */
	 void ConfigureOriginalHardware();
	 
	 /**
	  * Configure input system for enhanced NiXX-32+ hardware
	  */
	 void ConfigurePlusHardware();
	 
	 /**
	  * Update force feedback effects
	  * @param deltaTime Time elapsed since last update in milliseconds
	  */
	 void UpdateForceFeedback(float deltaTime);
	 
	 /**
	  * Notify input callbacks of state change
	  * @param playerIndex Player index whose state changed
	  */
	 void NotifyCallbacks(uint8_t playerIndex);
	 
	 /**
	  * Convert input state to register values
	  * @param playerIndex Player index
	  */
	 void UpdateRegisterValues(uint8_t playerIndex);
	 
	 /**
	  * Convert register values to input state
	  * @param playerIndex Player index
	  */
	 void UpdateStateFromRegisters(uint8_t playerIndex);
	 
	 /**
	  * Check if a player index is valid
	  * @param playerIndex Player index to check
	  * @return True if player index is valid
	  */
	 bool IsValidPlayerIndex(uint8_t playerIndex) const;
 };
 
 } // namespace NiXX32