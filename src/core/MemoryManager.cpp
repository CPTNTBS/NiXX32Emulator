#include "MemoryManager.h"
#include "GraphicsSystem.h"
#include "AudioSystem.h"
#include "InputSystem.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>

MemoryManager::MemoryManager(bool isEnhancedModel)
    : m_graphics(nullptr)
    , m_audio(nullptr)
    , m_input(nullptr)
    , m_isEnhancedModel(isEnhancedModel)
{
    // Determine memory sizes based on model type
    const size_t mainRamSize = m_isEnhancedModel ? 2 * 1024 * 1024 : 1 * 1024 * 1024; // 2MB or 1MB
    const size_t audioRamSize = m_isEnhancedModel ? 32 * 1024 : 16 * 1024; // 32KB or 16KB
    
    // Pre-allocate memory regions
    m_mainRAM.resize(mainRamSize, 0);
    m_audioRAM.resize(audioRamSize, 0);
}

MemoryManager::~MemoryManager()
{
    // Memory vectors will clean up automatically
}

void MemoryManager::initialize()
{
    // Setup memory handlers for various regions
    setupMemoryHandlers();
    
    // Initialize memory with known values
    std::fill(m_mainRAM.begin(), m_mainRAM.end(), 0);
    std::fill(m_audioRAM.begin(), m_audioRAM.end(), 0);
}

void MemoryManager::setComponents(GraphicsSystem* graphics, AudioSystem* audio, InputSystem* input)
{
    m_graphics = graphics;
    m_audio = audio;
    m_input = input;
}

bool MemoryManager::loadROM(const std::string& romPath)
{
    std::ifstream romFile(romPath, std::ios::binary);
    if (!romFile.is_open()) {
        std::cerr << "Failed to open ROM file: " << romPath << std::endl;
        return false;
    }
    
    // Get file size
    romFile.seekg(0, std::ios::end);
    const size_t fileSize = romFile.tellg();
    romFile.seekg(0, std::ios::beg);
    
    // Validate ROM size against hardware model
    const size_t maxRomSize = m_isEnhancedModel ? 4 * 1024 * 1024 : 2 * 1024 * 1024; // 4MB or 2MB
    if (fileSize > maxRomSize) {
        std::cerr << "ROM file exceeds maximum size for selected hardware model" << std::endl;
        return false;
    }
    
    // Read ROM into memory
    m_mainROM.resize(fileSize);
    romFile.read(reinterpret_cast<char*>(m_mainROM.data()), fileSize);
    
    // Check if we need to load audio ROM (usually part of the main ROM, extracted here)
    // For simplicity, we assume it's at the end of the main ROM
    const size_t audioRomSize = std::min<size_t>(32 * 1024, fileSize / 10); // Approximately 10% of main ROM
    m_audioROM.resize(audioRomSize);
    
    // Copy audio ROM data from main ROM (in a real system this would be separate)
    if (fileSize > audioRomSize) {
        std::memcpy(m_audioROM.data(), m_mainROM.data() + (fileSize - audioRomSize), audioRomSize);
    }
    
    return true;
}

void MemoryManager::reset()
{
    // Clear RAM but preserve ROM
    std::fill(m_mainRAM.begin(), m_mainRAM.end(), 0);
    std::fill(m_audioRAM.begin(), m_audioRAM.end(), 0);
    
    // If there are any memory-mapped registers that need resetting,
    // we would handle those here or notify the appropriate systems
    if (m_graphics) {
        // Inform graphics system of reset
        // m_graphics->reset();
    }
    
    if (m_audio) {
        // Reset audio system
        // m_audio->reset();
    }
    
    if (m_input) {
        // Reset input system
        // m_input->reset();
    }
}

// Main CPU (68000) Memory Access
uint8_t MemoryManager::readByte(uint32_t address)
{
    // Check each memory region
    for (const auto& region : m_mainMemoryRegions) {
        if (address >= region.start && address <= region.end) {
            if (region.readHandler) {
                return region.readHandler(address);
            }
            break;
        }
    }
    
    // Direct memory access for common regions
    if (address >= ROM_START && address < ROM_START + m_mainROM.size()) {
        return m_mainROM[address - ROM_START];
    }
    else if (address >= MAIN_RAM_START && address < MAIN_RAM_START + m_mainRAM.size()) {
        return m_mainRAM[address - MAIN_RAM_START];
    }
    
    // Handle unmapped memory
    std::cerr << "Warning: Read from unmapped memory address: 0x" << std::hex << address << std::endl;
    return 0xFF;
}

uint16_t MemoryManager::readWord(uint32_t address)
{
    // 68000 requires word addresses to be even
    if (address & 1) {
        std::cerr << "Error: Unaligned word read at 0x" << std::hex << address << std::endl;
        // In a real 68000, this would trigger an exception
        // For emulation, we'll just force alignment
        address &= ~1;
    }
    
    // Read two consecutive bytes and combine (68000 is big-endian)
    uint16_t highByte = static_cast<uint16_t>(readByte(address)) << 8;
    uint16_t lowByte = readByte(address + 1);
    return highByte | lowByte;
}

uint32_t MemoryManager::readLong(uint32_t address)
{
    // 68000 requires long word addresses to be even
    if (address & 1) {
        std::cerr << "Error: Unaligned long word read at 0x" << std::hex << address << std::endl;
        // Force alignment for emulation
        address &= ~1;
    }
    
    // Read two consecutive words and combine
    uint32_t highWord = static_cast<uint32_t>(readWord(address)) << 16;
    uint32_t lowWord = readWord(address + 2);
    return highWord | lowWord;
}

void MemoryManager::writeByte(uint32_t address, uint8_t value)
{
    // Check each memory region
    for (const auto& region : m_mainMemoryRegions) {
        if (address >= region.start && address <= region.end) {
            if (region.writeHandler) {
                region.writeHandler(address, value);
                return;
            }
            break;
        }
    }
    
    // Direct memory access for writable regions
    if (address >= MAIN_RAM_START && address < MAIN_RAM_START + m_mainRAM.size()) {
        m_mainRAM[address - MAIN_RAM_START] = value;
        return;
    }
    else if (address >= ROM_START && address < ROM_START + m_mainROM.size()) {
        // ROM is read-only - ignore or log attempt to write
        std::cerr << "Warning: Attempted write to ROM at 0x" << std::hex << address << std::endl;
        return;
    }
    
    // Handle unmapped memory
    std::cerr << "Warning: Write to unmapped memory address: 0x" << std::hex << address << std::endl;
}

void MemoryManager::writeWord(uint32_t address, uint16_t value)
{
    // 68000 requires word addresses to be even
    if (address & 1) {
        std::cerr << "Error: Unaligned word write at 0x" << std::hex << address << std::endl;
        // Force alignment for emulation
        address &= ~1;
    }
    
    // Split the word into high and low bytes (big-endian)
    writeByte(address, (value >> 8) & 0xFF);
    writeByte(address + 1, value & 0xFF);
}

void MemoryManager::writeLong(uint32_t address, uint32_t value)
{
    // 68000 requires long word addresses to be even
    if (address & 1) {
        std::cerr << "Error: Unaligned long word write at 0x" << std::hex << address << std::endl;
        // Force alignment for emulation
        address &= ~1;
    }
    
    // Split the long word into high and low words
    writeWord(address, (value >> 16) & 0xFFFF);
    writeWord(address + 2, value & 0xFFFF);
}

// Audio CPU (Z80) Memory Access
uint8_t MemoryManager::audioReadByte(uint16_t address)
{
    // Check audio memory regions
    for (const auto& region : m_audioMemoryRegions) {
        if (address >= region.start && address <= region.end) {
            if (region.readHandler) {
                return region.readHandler(address);
            }
            break;
        }
    }
    
    // Direct memory access
    if (address >= AUDIO_ROM_START && address < AUDIO_ROM_START + m_audioROM.size()) {
        return m_audioROM[address - AUDIO_ROM_START];
    }
    else if (address >= AUDIO_WORK_RAM_START && address < AUDIO_WORK_RAM_START + m_audioRAM.size()) {
        return m_audioRAM[address - AUDIO_WORK_RAM_START];
    }
    
    // Handle unmapped audio memory
    std::cerr << "Warning: Read from unmapped audio memory address: 0x" << std::hex << address << std::endl;
    return 0xFF;
}

void MemoryManager::audioWriteByte(uint16_t address, uint8_t value)
{
    // Check audio memory regions
    for (const auto& region : m_audioMemoryRegions) {
        if (address >= region.start && address <= region.end) {
            if (region.writeHandler) {
                region.writeHandler(address, value);
                return;
            }
            break;
        }
    }
    
    // Direct memory access for writable regions
    if (address >= AUDIO_WORK_RAM_START && address < AUDIO_WORK_RAM_START + m_audioRAM.size()) {
        m_audioRAM[address - AUDIO_WORK_RAM_START] = value;
        return;
    }
    else if (address >= AUDIO_ROM_START && address < AUDIO_ROM_START + m_audioROM.size()) {
        // ROM is read-only
        std::cerr << "Warning: Attempted write to audio ROM at 0x" << std::hex << address << std::endl;
        return;
    }
    
    // Handle unmapped audio memory
    std::cerr << "Warning: Write to unmapped audio memory address: 0x" << std::hex << address << std::endl;
}

void MemoryManager::setupMemoryHandlers()
{
    // Clear existing handlers
    m_mainMemoryRegions.clear();
    m_audioMemoryRegions.clear();
    
    // Main CPU Memory Handlers
    
    // Graphics VRAM and Registers
    m_mainMemoryRegions.push_back({
        GRAPHICS_VRAM_START,
        GRAPHICS_VRAM_END,
        [this](uint32_t addr) -> uint8_t {
            // Forward read to graphics system
            if (m_graphics) {
                return m_graphics->readVRAM(addr - GRAPHICS_VRAM_START);
            }
            return 0xFF;
        },
        [this](uint32_t addr, uint8_t val) {
            // Forward write to graphics system
            if (m_graphics) {
                m_graphics->writeVRAM(addr - GRAPHICS_VRAM_START, val);
            }
        }
    });
    
    m_mainMemoryRegions.push_back({
        GRAPHICS_REG_START,
        GRAPHICS_REG_END,
        [this](uint32_t addr) -> uint8_t {
            // Forward read to graphics system
            if (m_graphics) {
                return m_graphics->readRegister(addr - GRAPHICS_REG_START);
            }
            return 0xFF;
        },
        [this](uint32_t addr, uint8_t val) {
            // Forward write to graphics system
            if (m_graphics) {
                m_graphics->writeRegister(addr - GRAPHICS_REG_START, val);
            }
        }
    });
    
    // Audio Shared RAM and Registers
    m_mainMemoryRegions.push_back({
        AUDIO_RAM_START,
        AUDIO_RAM_END,
        [this](uint32_t addr) -> uint8_t {
            // Forward read to audio system
            if (m_audio) {
                return m_audio->readSharedRAM(addr - AUDIO_RAM_START);
            }
            return 0xFF;
        },
        [this](uint32_t addr, uint8_t val) {
            // Forward write to audio system
            if (m_audio) {
                m_audio->writeSharedRAM(addr - AUDIO_RAM_START, val);
            }
        }
    });
    
    m_mainMemoryRegions.push_back({
        AUDIO_REG_START,
        AUDIO_REG_END,
        [this](uint32_t addr) -> uint8_t {
            // Forward read to audio system
            if (m_audio) {
                return m_audio->readRegister(addr - AUDIO_REG_START);
            }
            return 0xFF;
        },
        [this](uint32_t addr, uint8_t val) {
            // Forward write to audio system
            if (m_audio) {
                m_audio->writeRegister(addr - AUDIO_REG_START, val);
            }
        }
    });
    
    // Input System Registers
    m_mainMemoryRegions.push_back({
        INPUT_REG_START,
        INPUT_REG_END,
        [this](uint32_t addr) -> uint8_t {
            // Forward read to input system
            if (m_input) {
                return m_input->readRegister(addr - INPUT_REG_START);
            }
            return 0xFF;
        },
        [this](uint32_t addr, uint8_t val) {
            // Forward write to input system
            if (m_input) {
                m_input->writeRegister(addr - INPUT_REG_START, val);
            }
        }
    });
    
    // System Registers (handled by memory manager directly)
    m_mainMemoryRegions.push_back({
        SYSTEM_REG_START,
        SYSTEM_REG_END,
        [this](uint32_t addr) -> uint8_t {
            return readSystemRegister(addr);
        },
        [this](uint32_t addr, uint8_t val) {
            writeSystemRegister(addr, val);
        }
    });
    
    // Audio CPU Memory Handlers
    
    // YM2151 FM Synthesis chip
    m_audioMemoryRegions.push_back({
        YM2151_REG_START,
        YM2151_REG_END,
        [this](uint32_t addr) -> uint8_t {
            // Forward read to audio system
            if (m_audio) {
                return m_audio->readYM2151(addr - YM2151_REG_START);
            }
            return 0xFF;
        },
        [this](uint32_t addr, uint8_t val) {
            // Forward write to audio system
            if (m_audio) {
                m_audio->writeYM2151(addr - YM2151_REG_START, val);
            }
        }
    });
    
    // PCM sample playback
    m_audioMemoryRegions.push_back({
        PCM_REG_START,
        PCM_REG_END,
        [this](uint32_t addr) -> uint8_t {
            // Forward read to audio system
            if (m_audio) {
                return m_audio->readPCM(addr - PCM_REG_START);
            }
            return 0xFF;
        },
        [this](uint32_t addr, uint8_t val) {
            // Forward write to audio system
            if (m_audio) {
                m_audio->writePCM(addr - PCM_REG_START, val);
            }
        }
    });
    
    // Q-Sound 3D audio chip
    m_audioMemoryRegions.push_back({
        QSOUND_REG_START,
        QSOUND_REG_END,
        [this](uint32_t addr) -> uint8_t {
            // Forward read to audio system
            if (m_audio) {
                return m_audio->readQSound(addr - QSOUND_REG_START);
            }
            return 0xFF;
        },
        [this](uint32_t addr, uint8_t val) {
            // Forward write to audio system
            if (m_audio) {
                m_audio->writeQSound(addr - QSOUND_REG_START, val);
            }
        }
    });
}

bool MemoryManager::isAddressInRange(uint32_t address, uint32_t start, uint32_t end)
{
    return (address >= start && address <= end);
}

uint8_t MemoryManager::readSystemRegister(uint32_t address)
{
    const uint32_t offset = address - SYSTEM_REG_START;
    
    // Define system registers - these would control global system state
    // Examples might include:
    // - 0x00: System status/flags
    // - 0x01: Hardware model information
    // - 0x02: Interrupt control
    // - 0x03: DIP switch settings
    // - 0x04: Coin counters
    // etc.
    
    switch (offset) {
        case 0x00: // System status
            return 0x01; // System ready
            
        case 0x01: // Hardware model
            return m_isEnhancedModel ? 0x02 : 0x01; // 0x01 = NiXX-32, 0x02 = NiXX-32+
            
        case 0x02: // Interrupt status
            // Would return pending interrupt status
            return 0x00;
            
        case 0x03: // DIP switches
            // These would typically be set based on arcade cabinet configuration
            return 0xFF; // All switches on
            
        default:
            std::cerr << "Warning: Read from undefined system register: 0x" 
                      << std::hex << offset << std::endl;
            return 0xFF;
    }
}

void MemoryManager::writeSystemRegister(uint32_t address, uint8_t value)
{
    const uint32_t offset = address - SYSTEM_REG_START;
    
    switch (offset) {
        case 0x00: // System control
            // Could handle system reset, interrupt enables, etc.
            if (value & 0x01) {
                std::cout << "System reset requested via register write" << std::endl;
                reset();
            }
            break;
            
        case 0x02: // Interrupt control
            // Control which interrupts are enabled
            std::cout << "Interrupt control set to: 0x" << std::hex << static_cast<int>(value) << std::endl;
            break;
            
        case 0x04: // Coin counter
            // In a real arcade system, this would trigger the coin mechanism
            std::cout << "Coin counter incremented" << std::endl;
            break;
            
        default:
            std::cerr << "Warning: Write to undefined system register: 0x" 
                      << std::hex << offset << " with value 0x" 
                      << static_cast<int>(value) << std::endl;
            break;
    }
}