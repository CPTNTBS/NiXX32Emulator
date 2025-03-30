// M68000CPU.h - Motorola 68000 CPU Emulation
#pragma once

#include <cstdint>
#include <array>
#include <functional>

class MemoryManager;

class M68000CPU {
public:
    // Register set
    struct Registers {
        // Data registers (D0-D7)
        std::array<uint32_t, 8> d;
        
        // Address registers (A0-A7/SP)
        std::array<uint32_t, 8> a;
        
        // Program counter
        uint32_t pc;
        
        // Status register (CCR + supervisor bits)
        uint16_t sr;
    };

private:
    Registers m_registers;
    MemoryManager* m_memory;
    
    // Clock frequency (16MHz for base model, 20MHz for enhanced)
    double m_clockFrequency;
    
    // Cycle counting
    uint64_t m_cycles;
    
    // Interrupt handling
    bool m_interruptPending;
    int m_interruptLevel;
    
    // Instruction lookup table
    using InstructionHandler = std::function<void(uint16_t)>;
    std::array<InstructionHandler, 65536> m_instructionTable;

public:
    M68000CPU(MemoryManager* memory, double clockFrequency);
    ~M68000CPU();
    
    // CPU lifecycle
    void reset();
    int execute(int cycles); // Returns actual cycles executed
    
    // Memory access (these will delegate to the memory manager)
    uint8_t readByte(uint32_t address);
    uint16_t readWord(uint32_t address);
    uint32_t readLong(uint32_t address);
    void writeByte(uint32_t address, uint8_t value);
    void writeWord(uint32_t address, uint16_t value);
    void writeLong(uint32_t address, uint32_t value);
    
    // Interrupt handling
    void setInterrupt(int level);
    void clearInterrupt();
    
    // Register access
    const Registers& getRegisters() const { return m_registers; }
    void setRegisters(const Registers& regs) { m_registers = regs; }
    
    // Status register convenience functions
    bool isSupervisorMode() const;
    uint8_t getInterruptMask() const;
    bool getXFlag() const;
    bool getNegativeFlag() const;
    bool getZeroFlag() const;
    bool getOverflowFlag() const;
    bool getCarryFlag() const;
    
private:
    // Internal implementation details
    void initializeInstructionTable();
    uint16_t fetchInstruction();
    
    // Addressing mode handlers
    uint32_t calculateEffectiveAddress(uint16_t instruction, int mode, int reg);
    
    // Instruction handlers (just a small sample)
    void handleMOVE(uint16_t instruction);
    void handleADD(uint16_t instruction);
    void handleSUB(uint16_t instruction);
    void handleAND(uint16_t instruction);
    void handleOR(uint16_t instruction);
    void handleEOR(uint16_t instruction);
    void handleCMP(uint16_t instruction);
    void handleJMP(uint16_t instruction);
    void handleJSR(uint16_t instruction);
    void handleRTS(uint16_t instruction);
    void handleLEA(uint16_t instruction);
    void handleBcc(uint16_t instruction);
    void handleDBcc(uint16_t instruction);
    void handleILLEGAL(uint16_t instruction);
};