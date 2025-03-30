// InputSystem.h - NiXX-32 Input Handling
#pragma once

#include <cstdint>
#include <array>
#include <unordered_map>
#include <SDL.h>

// Define input types that would have been used on the original arcade machine
enum class InputType {
    P1_START,
    P1_COIN,
    P1_UP,
    P1_DOWN,
    P1_LEFT,
    P1_RIGHT,
    P1_BUTTON1,  // Attack
    P1_BUTTON2,  // Special move
    P1_BUTTON3,  // Boost
    P2_START,
    P2_COIN,
    P2_UP,
    P2_DOWN,
    P2_LEFT,
    P2_RIGHT,
    P2_BUTTON1,
    P2_BUTTON2,
    P2_BUTTON3,
    SERVICE,
    TEST,
    DIP_SWITCH1,
    DIP_SWITCH2,
    DIP_SWITCH3,
    DIP_SWITCH4,
    DIP_SWITCH5,
    DIP_SWITCH6,
    DIP_SWITCH7,
    DIP_SWITCH8,
    COUNT  // Number of input types
};

class InputSystem {
private:
    // Current state of inputs
    std::array<bool, static_cast<size_t>(InputType::COUNT)> m_inputState;
    
    // Previous state of inputs (for edge detection)
    std::array<bool, static_cast<size_t>(InputType::COUNT)> m_prevInputState;
    
    // Key mappings (configurable)
    std::unordered_map<SDL_Keycode, InputType> m_keyMappings;
    
    // Controller mappings
    struct ControllerMapping {
        int controllerId;
        std::unordered_map<SDL_GameControllerButton, InputType> buttonMappings;
        std::unordered_map<SDL_GameControllerAxis, InputType> posAxisMappings;
        std::unordered_map<SDL_GameControllerAxis, InputType> negAxisMappings;
    };
    
    std::vector<ControllerMapping> m_controllerMappings;
    std::vector<SDL_GameController*> m_controllers;
    
    // DIP switch settings
    uint8_t m_dipSwitches;

public:
    InputSystem();
    ~InputSystem();
    
    // Initialization
    void initialize();
    void setupDefaultKeyMappings();
    void setupDefaultControllerMappings();
    
    // Input handling
    void update();  // Call once per frame to update edge detection
    
    // SDL event handlers
    void handleKeyEvent(const SDL_KeyboardEvent& event);
    void handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event);
    void handleControllerButtonEvent(const SDL_ControllerButtonEvent& event);
    void handleControllerAxisEvent(const SDL_ControllerAxisEvent& event);
    
    // Input state access
    bool isPressed(InputType input) const;
    bool isJustPressed(InputType input) const;  // Rising edge detection
    bool isJustReleased(InputType input) const; // Falling edge detection
    
    // DIP switch handling
    void setDIPSwitch(int index, bool value);
    bool getDIPSwitch(int index) const;
    uint8_t getDIPSwitches() const { return m_dipSwitches; }
    void setDIPSwitches(uint8_t value) { m_dipSwitches = value; }
    
    // Configuration
    void mapKey(SDL_Keycode key, InputType input);
    void clearKeyMappings();
    void mapControllerButton(int controllerId, SDL_GameControllerButton button, InputType input);
    void mapControllerAxis(int controllerId, SDL_GameControllerAxis axis, bool positive, InputType input);
    
    // Memory-mapped register interface (for the emulated system)
    uint8_t readInputPort(uint32_t address);
};

// InputSystem.cpp - Implementation of the Input System
#include "InputSystem.h"
#include <iostream>

InputSystem::InputSystem() : m_dipSwitches(0xFF) {
    // Initialize all inputs to not pressed
    for (size_t i = 0; i < static_cast<size_t>(InputType::COUNT); ++i) {
        m_inputState[i] = false;
        m_prevInputState[i] = false;
    }
}

InputSystem::~InputSystem() {
    // Close any open controllers
    for (auto controller : m_controllers) {
        SDL_GameControllerClose(controller);
    }
    m_controllers.clear();
}

void InputSystem::initialize() {
    // Set up default key and controller mappings
    setupDefaultKeyMappings();
    setupDefaultControllerMappings();
    
    // Initialize SDL Game Controller subsystem
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error initializing game controller: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Open any connected controllers
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController* controller = SDL_GameControllerOpen(i);
            if (controller) {
                m_controllers.push_back(controller);
                std::cout << "Opened controller: " << SDL_GameControllerName(controller) << std::endl;
            }
        }
    }
}

void InputSystem::setupDefaultKeyMappings() {
    // Player 1 controls
    m_keyMappings[SDLK_1] = InputType::P1_COIN;
    m_keyMappings[SDLK_RETURN] = InputType::P1_START;
    m_keyMappings[SDLK_w] = InputType::P1_UP;
    m_keyMappings[SDLK_s] = InputType::P1_DOWN;
    m_keyMappings[SDLK_a] = InputType::P1_LEFT;
    m_keyMappings[SDLK_d] = InputType::P1_RIGHT;
    m_keyMappings[SDLK_j] = InputType::P1_BUTTON1;
    m_keyMappings[SDLK_k] = InputType::P1_BUTTON2;
    m_keyMappings[SDLK_l] = InputType::P1_BUTTON3;
    
    // Player 2 controls
    m_keyMappings[SDLK_2] = InputType::P2_COIN;
    m_keyMappings[SDLK_RSHIFT] = InputType::P2_START;
    m_keyMappings[SDLK_UP] = InputType::P2_UP;
    m_keyMappings[SDLK_DOWN] = InputType::P2_DOWN;
    m_keyMappings[SDLK_LEFT] = InputType::P2_LEFT;
    m_keyMappings[SDLK_RIGHT] = InputType::P2_RIGHT;
    m_keyMappings[SDLK_KP_1] = InputType::P2_BUTTON1;
    m_keyMappings[SDLK_KP_2] = InputType::P2_BUTTON2;
    m_keyMappings[SDLK_KP_3] = InputType::P2_BUTTON3;
    
    // Service/test controls
    m_keyMappings[SDLK_9] = InputType::SERVICE;
    m_keyMappings[SDLK_F2] = InputType::TEST;
    
    // DIP switches
    m_keyMappings[SDLK_F1] = InputType::DIP_SWITCH1;
}

void InputSystem::setupDefaultControllerMappings() {
    // Player 1 controller mapping (first controller)
    ControllerMapping p1Mapping;
    p1Mapping.controllerId = 0;
    
    // Button mappings
    p1Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_START] = InputType::P1_START;
    p1Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_BACK] = InputType::P1_COIN;
    p1Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_A] = InputType::P1_BUTTON1;
    p1Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_B] = InputType::P1_BUTTON2;
    p1Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_X] = InputType::P1_BUTTON3;
    
    // D-pad mappings
    p1Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_DPAD_UP] = InputType::P1_UP;
    p1Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = InputType::P1_DOWN;
    p1Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = InputType::P1_LEFT;
    p1Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = InputType::P1_RIGHT;
    
    // Axis mappings
    p1Mapping.posAxisMappings[SDL_CONTROLLER_AXIS_LEFTX] = InputType::P1_RIGHT;
    p1Mapping.negAxisMappings[SDL_CONTROLLER_AXIS_LEFTX] = InputType::P1_LEFT;
    p1Mapping.posAxisMappings[SDL_CONTROLLER_AXIS_LEFTY] = InputType::P1_DOWN;
    p1Mapping.negAxisMappings[SDL_CONTROLLER_AXIS_LEFTY] = InputType::P1_UP;
    
    m_controllerMappings.push_back(p1Mapping);
    
    // Player 2 controller mapping (second controller)
    ControllerMapping p2Mapping;
    p2Mapping.controllerId = 1;
    
    // Button mappings
    p2Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_START] = InputType::P2_START;
    p2Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_BACK] = InputType::P2_COIN;
    p2Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_A] = InputType::P2_BUTTON1;
    p2Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_B] = InputType::P2_BUTTON2;
    p2Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_X] = InputType::P2_BUTTON3;
    
    // D-pad mappings
    p2Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_DPAD_UP] = InputType::P2_UP;
    p2Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = InputType::P2_DOWN;
    p2Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = InputType::P2_LEFT;
    p2Mapping.buttonMappings[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = InputType::P2_RIGHT;
    
    // Axis mappings
    p2Mapping.posAxisMappings[SDL_CONTROLLER_AXIS_LEFTX] = InputType::P2_RIGHT;
    p2Mapping.negAxisMappings[SDL_CONTROLLER_AXIS_LEFTX] = InputType::P2_LEFT;
    p2Mapping.posAxisMappings[SDL_CONTROLLER_AXIS_LEFTY] = InputType::P2_DOWN;
    p2Mapping.negAxisMappings[SDL_CONTROLLER_AXIS_LEFTY] = InputType::P2_UP;
    
    m_controllerMappings.push_back(p2Mapping);
}

void InputSystem::update() {
    // Copy current state to previous state for edge detection
    m_prevInputState = m_inputState;
}

void InputSystem::handleKeyEvent(const SDL_KeyboardEvent& event) {
    // Look up the key in our mapping
    auto it = m_keyMappings.find(event.keysym.sym);
    if (it != m_keyMappings.end()) {
        // Set input state based on key state
        m_inputState[static_cast<size_t>(it->second)] = (event.type == SDL_KEYDOWN);
    }
}

void InputSystem::handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event) {
    if (event.type == SDL_CONTROLLERDEVICEADDED) {
        // Open the new controller
        SDL_GameController* controller = SDL_GameControllerOpen(event.which);
        if (controller) {
            m_controllers.push_back(controller);
            std::cout << "Controller connected: " << SDL_GameControllerName(controller) << std::endl;
        }
    } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        // Find and close the removed controller
        for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
            if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(*it)) == event.which) {
                std::cout << "Controller disconnected: " << SDL_GameControllerName(*it) << std::endl;
                SDL_GameControllerClose(*it);
                m_controllers.erase(it);
                break;
            }
        }
    }
}

void InputSystem::handleControllerButtonEvent(const SDL_ControllerButtonEvent& event) {
    // Find the controller mapping for this joystick instance
    int controllerId = -1;
    for (size_t i = 0; i < m_controllers.size(); ++i) {
        if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(m_controllers[i])) == event.which) {
            controllerId = static_cast<int>(i);
            break;
        }
    }
    
    if (controllerId >= 0 && controllerId < static_cast<int>(m_controllerMappings.size())) {
        // Look up the button in our mapping
        auto& mapping = m_controllerMappings[controllerId];
        auto it = mapping.buttonMappings.find(static_cast<SDL_GameControllerButton>(event.button));
        if (it != mapping.buttonMappings.end()) {
            // Set input state based on button state
            m_inputState[static_cast<size_t>(it->second)] = (event.state == SDL_PRESSED);
        }
    }
}

void InputSystem::handleControllerAxisEvent(const SDL_ControllerAxisEvent& event) {
    // Find the controller mapping for this joystick instance
    int controllerId = -1;
    for (size_t i = 0; i < m_controllers.size(); ++i) {
        if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(m_controllers[i])) == event.which) {
            controllerId = static_cast<int>(i);
            break;
        }
    }
    
    if (controllerId >= 0 && controllerId < static_cast<int>(m_controllerMappings.size())) {
        // Look up the axis in our mappings
        auto& mapping = m_controllerMappings[controllerId];
        auto axis = static_cast<SDL_GameControllerAxis>(event.axis);
        
        // Check positive direction
        auto posIt = mapping.posAxisMappings.find(axis);
        if (posIt != mapping.posAxisMappings.end()) {
            // Set input state based on axis value (with deadzone)
            m_inputState[static_cast<size_t>(posIt->second)] = (event.value > 16384);
        }
        
        // Check negative direction
        auto negIt = mapping.negAxisMappings.find(axis);
        if (negIt != mapping.negAxisMappings.end()) {
            // Set input state based on axis value (with deadzone)
            m_inputState[static_cast<size_t>(negIt->second)] = (event.value < -16384);
        }
    }
}

bool InputSystem::isPressed(InputType input) const {
    return m_inputState[static_cast<size_t>(input)];
}

bool InputSystem::isJustPressed(InputType input) const {
    size_t idx = static_cast<size_t>(input);
    return m_inputState[idx] && !m_prevInputState[idx];
}

bool InputSystem::isJustReleased(InputType input) const {
    size_t idx = static_cast<size_t>(input);
    return !m_inputState[idx] && m_prevInputState[idx];
}

void InputSystem::setDIPSwitch(int index, bool value) {
    if (index >= 0 && index < 8) {
        if (value) {
            m_dipSwitches |= (1 << index);
        } else {
            m_dipSwitches &= ~(1 << index);
        }
    }
}

bool InputSystem::getDIPSwitch(int index) const {
    if (index >= 0 && index < 8) {
        return (m_dipSwitches & (1 << index)) != 0;
    }
    return false;
}

void InputSystem::mapKey(SDL_Keycode key, InputType input) {
    m_keyMappings[key] = input;
}

void InputSystem::clearKeyMappings() {
    m_keyMappings.clear();
}

void InputSystem::mapControllerButton(int controllerId, SDL_GameControllerButton button, InputType input) {
    // Find or create controller mapping
    for (auto& mapping : m_controllerMappings) {
        if (mapping.controllerId == controllerId) {
            mapping.buttonMappings[button] = input;
            return;
        }
    }
    
    // Create new mapping if not found
    ControllerMapping newMapping;
    newMapping.controllerId = controllerId;
    newMapping.buttonMappings[button] = input;
    m_controllerMappings.push_back(newMapping);
}

void InputSystem::mapControllerAxis(int controllerId, SDL_GameControllerAxis axis, bool positive, InputType input) {
    // Find or create controller mapping
    for (auto& mapping : m_controllerMappings) {
        if (mapping.controllerId == controllerId) {
            if (positive) {
                mapping.posAxisMappings[axis] = input;
            } else {
                mapping.negAxisMappings[axis] = input;
            }
            return;
        }
    }
    
    // Create new mapping if not found
    ControllerMapping newMapping;
    newMapping.controllerId = controllerId;
    if (positive) {
        newMapping.posAxisMappings[axis] = input;
    } else {
        newMapping.negAxisMappings[axis] = input;
    }
    m_controllerMappings.push_back(newMapping);
}

uint8_t InputSystem::readInputPort(uint32_t address) {
    // Map memory addresses to input ports
    // This would match the original hardware's memory map
    switch (address) {
        case 0x830000:  // Player 1 inputs
            return (isPressed(InputType::P1_RIGHT) ? 0x01 : 0) |
                   (isPressed(InputType::P1_LEFT) ? 0x02 : 0) |
                   (isPressed(InputType::P1_DOWN) ? 0x04 : 0) |
                   (isPressed(InputType::P1_UP) ? 0x08 : 0) |
                   (isPressed(InputType::P1_BUTTON1) ? 0x10 : 0) |
                   (isPressed(InputType::P1_BUTTON2) ? 0x20 : 0) |
                   (isPressed(InputType::P1_BUTTON3) ? 0x40 : 0) |
                   (isPressed(InputType::P1_START) ? 0x80 : 0);
                   
        case 0x830001:  // Player 2 inputs
            return (isPressed(InputType::P2_RIGHT) ? 0x01 : 0) |
                   (isPressed(InputType::P2_LEFT) ? 0x02 : 0) |
                   (isPressed(InputType::P2_DOWN) ? 0x04 : 0) |
                   (isPressed(InputType::P2_UP) ? 0x08 : 0) |
                   (isPressed(InputType::P2_BUTTON1) ? 0x10 : 0) |
                   (isPressed(InputType::P2_BUTTON2) ? 0x20 : 0) |
                   (isPressed(InputType::P2_BUTTON3) ? 0x40 : 0) |
                   (isPressed(InputType::P2_START) ? 0x80 : 0);
                   
        case 0x830002:  // System inputs
            return (isPressed(InputType::P1_COIN) ? 0x01 : 0) |
                   (isPressed(InputType::P2_COIN) ? 0x02 : 0) |
                   (isPressed(InputType::SERVICE) ? 0x04 : 0) |
                   (isPressed(InputType::TEST) ? 0x08 : 0) |
                   0xF0;  // Upper bits typically unused or fixed
                   
        case 0x830003:  // DIP switches
            return m_dipSwitches;
            
        default:
            return 0xFF;  // Default value for unmapped addresses
    }
}