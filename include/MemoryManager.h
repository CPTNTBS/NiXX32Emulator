// MemoryManager.h - NiXX-32 Memory Management System
#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <functional>
#include <unordered_map>

class GraphicsSystem;
class AudioSystem;
class InputSystem;

class MemoryManager {
public:
    // Memory map constants
    // These addresses are fictional but organized in a way that would make sense
    // for an arcade system of this era
    
    // Main CPU (68000) address space
    static constexpr uint32_t ROM_START          = 0x000000;
    static constexpr uint32_t ROM_END            = 0x3FFFFF; // 4MB max on NiXX-32+
    
    static constexpr uint32_t MAIN_RAM_START     = 0x400000;
    static constexpr uint32_t MAIN_RAM_END       = 0x5FFFFF; // 2MB on NiXX-32+
    
    static constexpr uint32_t GRAPHICS_VRAM_START = 0x600000;
    static constexpr uint32_t GRAPHICS_VRAM_END  = 0x6FFFFF;
    
    static constexpr uint32_t GRAPHICS_REG_START = 0x800000;
    static constexpr uint32_t GRAPHICS_REG_END   = 0x80FFFF;
    
    static constexpr uint32_t AUDIO_RAM_START    = 0x810000;
    static constexpr uint32_t AUDIO_RAM_END      = 0x81FFFF;
    
    static constexpr uint32_t AUDIO_REG_START    = 0x820000;
    static constexpr uint32_t AUDIO_REG_END      = 0x82FFFF;
    
    static constexpr uint32_t INPUT_REG_START    = 0x830000;
    static constexpr uint32_t INPUT_REG_END      = 0x83FFFF;
    
    static constexpr uint32_t SYSTEM_REG_START   = 0x840000;
    static constexpr uint32_t SYSTEM_REG_END     = 0x84FFFF;
    
    // Z80 Audio CPU address space (separate from main CPU)
    static constexpr uint32_t AUDIO_ROM_START    = 0x0000;
    static constexpr uint32_t AUDIO_ROM_END      = 0x7FFF;
    
    static constexpr uint32_t AUDIO_WORK_RAM_START = 0x8000;
    static constexpr uint32_t AUDIO_WORK_RAM_END = 0xBFFF;
    
    static constexpr uint32_t YM2151_REG_START   = 0xC000;
    static constexpr uint32_t YM2151_REG_END     = 0xC0FF;
    
    static constexpr uint32_t PCM_REG_START      = 0xD000;
    static constexpr uint32_t PCM_REG_END        = 0xDFFF;
    
    static constexpr uint32_t QSOUND_REG_START   = 0xE000;
    static constexpr uint32_t QSOUND_REG_END     = 0xEFFF;

private:
    // References to other system components
    GraphicsSystem* m_graphics;
    AudioSystem* m_audio;
    InputSystem* m_input;
    bool m_isEnhancedModel;
    
    // Memory regions
    std::vector<uint8_t> m_mainROM;
    std::vector<uint8_t> m_mainRAM;
    std::vector<uint8_t> m_audioROM;
    std::vector<uint8_t> m_audioRAM;
    
    // Memory-mapped I/O handlers
    using ReadHandler = std::function<uint8_t(uint32_t)>;
    using WriteHandler = std::function<void(uint32_t, uint8_t)>;
    
    struct MemoryRegion {
        uint32_t start;
        uint32_t end;
        ReadHandler readHandler;
        WriteHandler writeHandler;
    };
    
    std::vector<MemoryRegion> m_mainMemoryRegions;
    std::vector<MemoryRegion> m_audioMemoryRegions;

public:
    MemoryManager(bool isEnhancedModel);
    ~MemoryManager();
    
    // Setup and initialization
    void initialize();
    void setComponents(GraphicsSystem* graphics, AudioSystem* audio, InputSystem* input);
    bool loadROM(const std::string& romPath);
    void reset();
    
    // Main CPU (68000) Memory Access
    uint8_t readByte(uint32_t address);
    uint16_t readWord(uint32_t address);
    uint32_t readLong(uint32_t address);
    void writeByte(uint32_t address, uint8_t value);
    void writeWord(uint32_t address, uint16_t value);
    void writeLong(uint32_t address, uint32_t value);
    
    // Audio CPU (Z80) Memory Access
    uint8_t audioReadByte(uint16_t address);
    void audioWriteByte(uint16_t address, uint8_t value);
    
    // Direct memory access (for debugging/tools)
    const std::vector<uint8_t>& getMainROM() const { return m_mainROM; }
    const std::vector<uint8_t>& getMainRAM() const { return m_mainRAM; }
    const std::vector<uint8_t>& getAudioROM() const { return m_audioROM; }
    const std::vector<uint8_t>& getAudioRAM() const { return m_audioRAM; }

private:
    // Internal functions
    void setupMemoryHandlers();
    bool isAddressInRange(uint32_t address, uint32_t start, uint32_t end);
    
    // System register handlers
    uint8_t readSystemRegister(uint32_t address);
    void writeSystemRegister(uint32_t address, uint8_t value);
};