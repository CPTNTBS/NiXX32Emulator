// M68000CPU.cpp - Motorola 68000 CPU Emulation Implementation
#include "M68000CPU.h"
#include "MemoryManager.h"
#include <stdexcept>

// Status register bit positions
constexpr uint16_t SR_CARRY = 0x0001;
constexpr uint16_t SR_OVERFLOW = 0x0002;
constexpr uint16_t SR_ZERO = 0x0004;
constexpr uint16_t SR_NEGATIVE = 0x0008;
constexpr uint16_t SR_EXTEND = 0x0010;
constexpr uint16_t SR_INTERRUPT_MASK = 0x0700;
constexpr uint16_t SR_SUPERVISOR = 0x2000;
constexpr uint16_t SR_TRACE = 0x8000;

// Addressing mode definitions
enum AddressingMode {
    MODE_DATA_REGISTER_DIRECT = 0,
    MODE_ADDRESS_REGISTER_DIRECT = 1,
    MODE_ADDRESS_REGISTER_INDIRECT = 2,
    MODE_ADDRESS_REGISTER_INDIRECT_POSTINC = 3,
    MODE_ADDRESS_REGISTER_INDIRECT_PREDEC = 4,
    MODE_ADDRESS_REGISTER_INDIRECT_DISPLACEMENT = 5,
    MODE_ADDRESS_REGISTER_INDIRECT_INDEX = 6,
    MODE_SPECIAL = 7
};

// Special addressing mode registers
enum SpecialModeRegister {
    REG_ABSOLUTE_SHORT = 0,
    REG_ABSOLUTE_LONG = 1,
    REG_PC_DISPLACEMENT = 2,
    REG_PC_INDEX = 3,
    REG_IMMEDIATE = 4
};

M68000CPU::M68000CPU(MemoryManager* memory, double clockFrequency) 
    : m_memory(memory),
      m_clockFrequency(clockFrequency),
      m_cycles(0),
      m_interruptPending(false),
      m_interruptLevel(0) {
    
    initializeInstructionTable();
    reset();
}

M68000CPU::~M68000CPU() {
    // Clean up any resources if needed
}

void M68000CPU::reset() {
    // Clear registers
    for (auto& reg : m_registers.d) {
        reg = 0;
    }
    
    for (auto& reg : m_registers.a) {
        reg = 0;
    }
    
    // Initialize stack pointer (A7) from reset vector
    m_registers.a[7] = readLong(0x000000);
    
    // Initialize program counter from reset vector
    m_registers.pc = readLong(0x000004);
    
    // Initialize status register to supervisor mode
    m_registers.sr = SR_SUPERVISOR;
    
    // Reset cycle counter
    m_cycles = 0;
    
    // Clear interrupts
    m_interruptPending = false;
    m_interruptLevel = 0;
}

int M68000CPU::execute(int cycles) {
    int startCycles = m_cycles;
    int targetCycles = startCycles + cycles;
    
    while (m_cycles < targetCycles) {
        // Check for pending interrupts
        if (m_interruptPending && (m_interruptLevel > getInterruptMask())) {
            // Handle interrupt
            // Save PC and SR to stack
            m_registers.a[7] -= 4;
            writeLong(m_registers.a[7], m_registers.pc);
            m_registers.a[7] -= 2;
            writeWord(m_registers.a[7], m_registers.sr);
            
            // Enter supervisor mode
            m_registers.sr |= SR_SUPERVISOR;
            
            // Set interrupt mask to current level
            m_registers.sr = (m_registers.sr & ~SR_INTERRUPT_MASK) | 
                   (bytes == 2 && (reg_value & 0x8000)) || 
                   (bytes == 4 && (reg_value & 0x80000000));
    bool ea_neg = (bytes == 1 && (ea_value & 0x80)) || 
                  (bytes == 2 && (ea_value & 0x8000)) || 
                  (bytes == 4 && (ea_value & 0x80000000));
    bool result_neg = (bytes == 1 && (result & 0x80)) || 
                      (bytes == 2 && (result & 0x8000)) || 
                      (bytes == 4 && (result & 0x80000000));
                      
    bool carry = (bytes == 1 && (ea_value > reg_value)) || 
                 (bytes == 2 && ((ea_value & 0xFFFF) > (reg_value & 0xFFFF))) || 
                 (bytes == 4 && ea_value > reg_value);
                 
    bool overflow = (reg_neg && !ea_neg && !result_neg) || 
                    (!reg_neg && ea_neg && result_neg);
    
    m_registers.sr &= ~(SR_NEGATIVE | SR_ZERO | SR_OVERFLOW | SR_CARRY);
    if (result_neg) m_registers.sr |= SR_NEGATIVE;
    if (bytes == 1 && (result & 0xFF) == 0) m_registers.sr |= SR_ZERO;
    else if (bytes == 2 && (result & 0xFFFF) == 0) m_registers.sr |= SR_ZERO;
    else if (bytes == 4 && result == 0) m_registers.sr |= SR_ZERO;
    if (overflow) m_registers.sr |= SR_OVERFLOW;
    if (carry) m_registers.sr |= SR_CARRY;
    
    // CMP doesn't store the result
    m_cycles += 4;  // Basic comparison overhead
}

void M68000CPU::handleJMP(uint16_t instruction) {
    // Extract effective address mode and register
    int ea_mode = (instruction >> 3) & 0x7;
    int ea_reg = instruction & 0x7;
    
    // Calculate jump target address
    uint32_t target_addr = calculateEffectiveAddress(instruction, ea_mode, ea_reg);
    
    // Set program counter to target address
    m_registers.pc = target_addr;
    
    // Add cycles for jump
    m_cycles += 8;
}

void M68000CPU::handleJSR(uint16_t instruction) {
    // Extract effective address mode and register
    int ea_mode = (instruction >> 3) & 0x7;
    int ea_reg = instruction & 0x7;
    
    // Calculate jump target address
    uint32_t target_addr = calculateEffectiveAddress(instruction, ea_mode, ea_reg);
    
    // Push return address onto stack
    m_registers.a[7] -= 4;  // Decrement stack pointer
    writeLong(m_registers.a[7], m_registers.pc);
    
    // Set program counter to target address
    m_registers.pc = target_addr;
    
    // Add cycles for jump and stack operations
    m_cycles += 16;
}

void M68000CPU::handleRTS(uint16_t instruction) {
    // Pop return address from stack
    uint32_t return_addr = readLong(m_registers.a[7]);
    m_registers.a[7] += 4;  // Increment stack pointer
    
    // Set program counter to return address
    m_registers.pc = return_addr;
    
    // Add cycles for return and stack operations
    m_cycles += 16;
}

void M68000CPU::handleLEA(uint16_t instruction) {
    // Extract address register and effective address mode/register
    int reg = (instruction >> 9) & 0x7;
    int ea_mode = (instruction >> 3) & 0x7;
    int ea_reg = instruction & 0x7;
    
    // Calculate effective address
    uint32_t ea_addr = calculateEffectiveAddress(instruction, ea_mode, ea_reg);
    
    // Load effective address into address register
    m_registers.a[reg] = ea_addr;
    
    // Add cycles for LEA operation
    m_cycles += 4;
}

void M68000CPU::handleBcc(uint16_t instruction) {
    // Extract condition code and displacement
    int condition = (instruction >> 8) & 0xF;
    int8_t displacement = instruction & 0xFF;
    
    // Check if condition is true
    bool condition_true = false;
    
    switch (condition) {
        case 0x0: // BRA - Branch Always
            condition_true = true;
            break;
        case 0x1: // BSR - Branch to Subroutine
            condition_true = true;
            // Push return address onto stack
            m_registers.a[7] -= 4;
            writeLong(m_registers.a[7], m_registers.pc);
            m_cycles += 8; // Extra cycles for stack operation
            break;
        case 0x2: // BHI - Branch if Higher
            condition_true = !(getCarryFlag() || getZeroFlag());
            break;
        case 0x3: // BLS - Branch if Lower or Same
            condition_true = getCarryFlag() || getZeroFlag();
            break;
        case 0x4: // BCC/BHS - Branch if Carry Clear / Higher or Same
            condition_true = !getCarryFlag();
            break;
        case 0x5: // BCS/BLO - Branch if Carry Set / Lower
            condition_true = getCarryFlag();
            break;
        case 0x6: // BNE - Branch if Not Equal
            condition_true = !getZeroFlag();
            break;
        case 0x7: // BEQ - Branch if Equal
            condition_true = getZeroFlag();
            break;
        case 0x8: // BVC - Branch if Overflow Clear
            condition_true = !getOverflowFlag();
            break;
        case 0x9: // BVS - Branch if Overflow Set
            condition_true = getOverflowFlag();
            break;
        case 0xA: // BPL - Branch if Plus
            condition_true = !getNegativeFlag();
            break;
        case 0xB: // BMI - Branch if Minus
            condition_true = getNegativeFlag();
            break;
        case 0xC: // BGE - Branch if Greater or Equal
            condition_true = (getNegativeFlag() && getOverflowFlag()) || 
                            (!getNegativeFlag() && !getOverflowFlag());
            break;
        case 0xD: // BLT - Branch if Less Than
            condition_true = (getNegativeFlag() && !getOverflowFlag()) || 
                            (!getNegativeFlag() && getOverflowFlag());
            break;
        case 0xE: // BGT - Branch if Greater Than
            condition_true = !getZeroFlag() && 
                            ((getNegativeFlag() && getOverflowFlag()) || 
                             (!getNegativeFlag() && !getOverflowFlag()));
            break;
        case 0xF: // BLE - Branch if Less or Equal
            condition_true = getZeroFlag() || 
                            (getNegativeFlag() && !getOverflowFlag()) || 
                            (!getNegativeFlag() && getOverflowFlag());
            break;
    }
    
    // If condition is true, branch to displacement
    if (condition_true) {
        // Check if displacement is 0, which means read word displacement
        if (displacement == 0) {
            int16_t word_displacement = readWord(m_registers.pc);
            m_registers.pc += 2;
            m_registers.pc += word_displacement;
            m_cycles += 4; // Extra cycles for word displacement
        } 
        // Check if displacement is -1, which means read long displacement
        else if (displacement == -1) {
            int32_t long_displacement = readLong(m_registers.pc);
            m_registers.pc += 4;
            m_registers.pc += long_displacement;
            m_cycles += 8; // Extra cycles for long displacement
        } 
        else {
            // Sign-extend 8-bit displacement
            m_registers.pc += (int32_t)(int8_t)displacement;
        }
        m_cycles += 10; // Base branch taken cycles
    } else {
        // Condition false, just advance PC if necessary for larger displacements
        if (displacement == 0) {
            m_registers.pc += 2;
            m_cycles += 4; // Extra cycles for word displacement fetch
        } 
        else if (displacement == -1) {
            m_registers.pc += 4;
            m_cycles += 8; // Extra cycles for long displacement fetch
        }
        m_cycles += 8; // Base branch not taken cycles
    }
}

void M68000CPU::handleDBcc(uint16_t instruction) {
    // Extract condition code and register
    int condition = (instruction >> 8) & 0xF;
    int reg = instruction & 0x7;
    
    // Read displacement
    int16_t displacement = readWord(m_registers.pc);
    m_registers.pc += 2;
    m_cycles += 4; // Extra cycles for displacement fetch
    
    // Check if condition is false
    bool condition_false = false;
    
    switch (condition) {
        case 0x0: // DBT - Test True
            condition_false = false;
            break;
        case 0x1: // DBF/DBRA - Test False / Decrement and Branch Always
            condition_false = true;
            break;
        case 0x2: // DBHI - Decrement and Branch if Higher
            condition_false = !(getCarryFlag() || getZeroFlag());
            break;
        case 0x3: // DBLS - Decrement and Branch if Lower or Same
            condition_false = getCarryFlag() || getZeroFlag();
            break;
        case 0x4: // DBCC - Decrement and Branch if Carry Clear
            condition_false = !getCarryFlag();
            break;
        case 0x5: // DBCS - Decrement and Branch if Carry Set
            condition_false = getCarryFlag();
            break;
        case 0x6: // DBNE - Decrement and Branch if Not Equal
            condition_false = !getZeroFlag();
            break;
        case 0x7: // DBEQ - Decrement and Branch if Equal
            condition_false = getZeroFlag();
            break;
        case 0x8: // DBVC - Decrement and Branch if Overflow Clear
            condition_false = !getOverflowFlag();
            break;
        case 0x9: // DBVS - Decrement and Branch if Overflow Set
            condition_false = getOverflowFlag();
            break;
        case 0xA: // DBPL - Decrement and Branch if Plus
            condition_false = !getNegativeFlag();
            break;
        case 0xB: // DBMI - Decrement and Branch if Minus
            condition_false = getNegativeFlag();
            break;
        case 0xC: // DBGE - Decrement and Branch if Greater or Equal
            condition_false = (getNegativeFlag() && getOverflowFlag()) || 
                              (!getNegativeFlag() && !getOverflowFlag());
            break;
        case 0xD: // DBLT - Decrement and Branch if Less Than
            condition_false = (getNegativeFlag() && !getOverflowFlag()) || 
                              (!getNegativeFlag() && getOverflowFlag());
            break;
        case 0xE: // DBGT - Decrement and Branch if Greater Than
            condition_false = !getZeroFlag() && 
                              ((getNegativeFlag() && getOverflowFlag()) || 
                               (!getNegativeFlag() && !getOverflowFlag()));
            break;
        case 0xF: // DBLE - Decrement and Branch if Less or Equal
            condition_false = getZeroFlag() || 
                              (getNegativeFlag() && !getOverflowFlag()) || 
                              (!getNegativeFlag() && getOverflowFlag());
            break;
    }
    
    // Invert the condition for DBcc logic (branch if false)
    condition_false = !condition_false;
    
    // If condition is false, decrement counter and branch if not -1
    if (condition_false) {
        // Get counter from data register (low word)
        uint16_t counter = m_registers.d[reg] & 0xFFFF;
        
        // Decrement counter
        counter--;
        
        // Update data register
        m_registers.d[reg] = (m_registers.d[reg] & 0xFFFF0000) | counter;
        
        // Branch if counter != -1 (0xFFFF)
        if (counter != 0xFFFF) {
            m_registers.pc += displacement;
            m_cycles += 10; // Base branch taken cycles
        } else {
            m_cycles += 14; // Base loop terminated cycles
        }
    } else {
        m_cycles += 12; // Base condition true (no branch) cycles
    }
}

void M68000CPU::handleILLEGAL(uint16_t instruction) {
    // Handle illegal instruction - typically generates an exception
    // Save PC and SR to stack
    m_registers.a[7] -= 4;
    writeLong(m_registers.a[7], m_registers.pc - 2); // Point to the illegal instruction
    m_registers.a[7] -= 2;
    writeWord(m_registers.a[7], m_registers.sr);
    
    // Enter supervisor mode
    m_registers.sr |= SR_SUPERVISOR;
    
    // Load illegal instruction vector (vector 4)
    m_registers.pc = readLong(0x10);
    
    // Account for exception processing cycles
    m_cycles += 34;
}         (m_interruptLevel << 8);
            
            // Load interrupt vector
            uint32_t vectorAddress = 0x60 + (m_interruptLevel * 4);
            m_registers.pc = readLong(vectorAddress);
            
            // Clear interrupt
            m_interruptPending = false;
            
            // Account for interrupt processing cycles
            m_cycles += 44;  // Typical interrupt overhead
        }
        
        // Fetch and execute next instruction
        uint16_t instruction = fetchInstruction();
        m_instructionTable[instruction](instruction);
        
        // Break if we've exceeded our cycle budget
        if (m_cycles >= targetCycles) {
            break;
        }
    }
    
    return m_cycles - startCycles;
}

uint16_t M68000CPU::fetchInstruction() {
    uint16_t instruction = readWord(m_registers.pc);
    m_registers.pc += 2;
    m_cycles += 4;  // Instruction fetch typically takes 4 cycles
    return instruction;
}

uint8_t M68000CPU::readByte(uint32_t address) {
    return m_memory->readByte(address);
}

uint16_t M68000CPU::readWord(uint32_t address) {
    return m_memory->readWord(address);
}

uint32_t M68000CPU::readLong(uint32_t address) {
    return m_memory->readLong(address);
}

void M68000CPU::writeByte(uint32_t address, uint8_t value) {
    m_memory->writeByte(address, value);
}

void M68000CPU::writeWord(uint32_t address, uint16_t value) {
    m_memory->writeWord(address, value);
}

void M68000CPU::writeLong(uint32_t address, uint32_t value) {
    m_memory->writeLong(address, value);
}

void M68000CPU::setInterrupt(int level) {
    m_interruptPending = true;
    m_interruptLevel = level;
}

void M68000CPU::clearInterrupt() {
    m_interruptPending = false;
}

bool M68000CPU::isSupervisorMode() const {
    return (m_registers.sr & SR_SUPERVISOR) != 0;
}

uint8_t M68000CPU::getInterruptMask() const {
    return (m_registers.sr & SR_INTERRUPT_MASK) >> 8;
}

bool M68000CPU::getXFlag() const {
    return (m_registers.sr & SR_EXTEND) != 0;
}

bool M68000CPU::getNegativeFlag() const {
    return (m_registers.sr & SR_NEGATIVE) != 0;
}

bool M68000CPU::getZeroFlag() const {
    return (m_registers.sr & SR_ZERO) != 0;
}

bool M68000CPU::getOverflowFlag() const {
    return (m_registers.sr & SR_OVERFLOW) != 0;
}

bool M68000CPU::getCarryFlag() const {
    return (m_registers.sr & SR_CARRY) != 0;
}

void M68000CPU::initializeInstructionTable() {
    // Initialize all entries to ILLEGAL instruction handler
    for (auto& handler : m_instructionTable) {
        handler = [this](uint16_t instruction) { handleILLEGAL(instruction); };
    }
    
    // Set up instruction handlers based on bit patterns
    
    // MOVE instructions (0b00xxxxxxxxxxxx)
    for (uint16_t size = 1; size <= 3; size++) {
        if (size == 3) continue; // Skip invalid size

        for (uint16_t dst_mode = 0; dst_mode <= 7; dst_mode++) {
            for (uint16_t dst_reg = 0; dst_reg <= 7; dst_reg++) {
                for (uint16_t src_mode = 0; src_mode <= 7; src_mode++) {
                    for (uint16_t src_reg = 0; src_reg <= 7; src_reg++) {
                        uint16_t opcode = (size << 12) | (dst_reg << 9) | (dst_mode << 6) | (src_mode << 3) | src_reg;
                        m_instructionTable[opcode] = [this](uint16_t instruction) { handleMOVE(instruction); };
                    }
                }
            }
        }
    }
    
    // ADD instructions (0b1101xxx0xxxxxxxx)
    for (uint16_t reg = 0; reg <= 7; reg++) {
        for (uint16_t op_mode = 0; op_mode <= 7; op_mode++) {
            for (uint16_t ea_mode = 0; ea_mode <= 7; ea_mode++) {
                for (uint16_t ea_reg = 0; ea_reg <= 7; ea_reg++) {
                    uint16_t opcode = 0xD000 | (reg << 9) | (op_mode << 6) | (ea_mode << 3) | ea_reg;
                    m_instructionTable[opcode] = [this](uint16_t instruction) { handleADD(instruction); };
                }
            }
        }
    }
    
    // SUB instructions (0b1001xxx0xxxxxxxx)
    for (uint16_t reg = 0; reg <= 7; reg++) {
        for (uint16_t op_mode = 0; op_mode <= 7; op_mode++) {
            for (uint16_t ea_mode = 0; ea_mode <= 7; ea_mode++) {
                for (uint16_t ea_reg = 0; ea_reg <= 7; ea_reg++) {
                    uint16_t opcode = 0x9000 | (reg << 9) | (op_mode << 6) | (ea_mode << 3) | ea_reg;
                    m_instructionTable[opcode] = [this](uint16_t instruction) { handleSUB(instruction); };
                }
            }
        }
    }
    
    // AND instructions (0b1100xxx0xxxxxxxx)
    for (uint16_t reg = 0; reg <= 7; reg++) {
        for (uint16_t op_mode = 0; op_mode <= 7; op_mode++) {
            for (uint16_t ea_mode = 0; ea_mode <= 7; ea_mode++) {
                for (uint16_t ea_reg = 0; ea_reg <= 7; ea_reg++) {
                    uint16_t opcode = 0xC000 | (reg << 9) | (op_mode << 6) | (ea_mode << 3) | ea_reg;
                    m_instructionTable[opcode] = [this](uint16_t instruction) { handleAND(instruction); };
                }
            }
        }
    }
    
    // OR instructions (0b1000xxx0xxxxxxxx)
    for (uint16_t reg = 0; reg <= 7; reg++) {
        for (uint16_t op_mode = 0; op_mode <= 7; op_mode++) {
            for (uint16_t ea_mode = 0; ea_mode <= 7; ea_mode++) {
                for (uint16_t ea_reg = 0; ea_reg <= 7; ea_reg++) {
                    uint16_t opcode = 0x8000 | (reg << 9) | (op_mode << 6) | (ea_mode << 3) | ea_reg;
                    m_instructionTable[opcode] = [this](uint16_t instruction) { handleOR(instruction); };
                }
            }
        }
    }
    
    // EOR instructions (0b1011xxx1xxxxxxxx)
    for (uint16_t reg = 0; reg <= 7; reg++) {
        for (uint16_t op_mode = 0; op_mode <= 7; op_mode++) {
            for (uint16_t ea_mode = 0; ea_mode <= 7; ea_mode++) {
                for (uint16_t ea_reg = 0; ea_reg <= 7; ea_reg++) {
                    uint16_t opcode = 0xB100 | (reg << 9) | (op_mode << 6) | (ea_mode << 3) | ea_reg;
                    m_instructionTable[opcode] = [this](uint16_t instruction) { handleEOR(instruction); };
                }
            }
        }
    }
    
    // CMP instructions (0b1011xxx0xxxxxxxx)
    for (uint16_t reg = 0; reg <= 7; reg++) {
        for (uint16_t op_mode = 0; op_mode <= 7; op_mode++) {
            for (uint16_t ea_mode = 0; ea_mode <= 7; ea_mode++) {
                for (uint16_t ea_reg = 0; ea_reg <= 7; ea_reg++) {
                    uint16_t opcode = 0xB000 | (reg << 9) | (op_mode << 6) | (ea_mode << 3) | ea_reg;
                    m_instructionTable[opcode] = [this](uint16_t instruction) { handleCMP(instruction); };
                }
            }
        }
    }
    
    // JMP instruction (0b0100111011xxxxxx)
    for (uint16_t ea_mode = 2; ea_mode <= 7; ea_mode++) {
        for (uint16_t ea_reg = 0; ea_reg <= 7; ea_reg++) {
            uint16_t opcode = 0x4EC0 | (ea_mode << 3) | ea_reg;
            m_instructionTable[opcode] = [this](uint16_t instruction) { handleJMP(instruction); };
        }
    }
    
    // JSR instruction (0b0100111010xxxxxx)
    for (uint16_t ea_mode = 2; ea_mode <= 7; ea_mode++) {
        for (uint16_t ea_reg = 0; ea_reg <= 7; ea_reg++) {
            uint16_t opcode = 0x4E80 | (ea_mode << 3) | ea_reg;
            m_instructionTable[opcode] = [this](uint16_t instruction) { handleJSR(instruction); };
        }
    }
    
    // RTS instruction (0b0100111001110101)
    m_instructionTable[0x4E75] = [this](uint16_t instruction) { handleRTS(instruction); };
    
    // LEA instruction (0b0100xxx111xxxxxx)
    for (uint16_t reg = 0; reg <= 7; reg++) {
        for (uint16_t ea_mode = 2; ea_mode <= 7; ea_mode++) {
            for (uint16_t ea_reg = 0; ea_reg <= 7; ea_reg++) {
                uint16_t opcode = 0x41C0 | (reg << 9) | (ea_mode << 3) | ea_reg;
                m_instructionTable[opcode] = [this](uint16_t instruction) { handleLEA(instruction); };
            }
        }
    }
    
    // Bcc instructions (0b0110xxxx????????)
    for (uint16_t condition = 0; condition <= 15; condition++) {
        for (uint16_t displacement = 0; displacement <= 255; displacement++) {
            uint16_t opcode = 0x6000 | (condition << 8) | displacement;
            m_instructionTable[opcode] = [this](uint16_t instruction) { handleBcc(instruction); };
        }
    }
    
    // DBcc instructions (0b0101xxxx11001xxx)
    for (uint16_t condition = 0; condition <= 15; condition++) {
        for (uint16_t reg = 0; reg <= 7; reg++) {
            uint16_t opcode = 0x50C8 | (condition << 8) | reg;
            m_instructionTable[opcode] = [this](uint16_t instruction) { handleDBcc(instruction); };
        }
    }
}

uint32_t M68000CPU::calculateEffectiveAddress(uint16_t instruction, int mode, int reg) {
    switch (mode) {
        case MODE_DATA_REGISTER_DIRECT:
            throw std::runtime_error("Data register direct mode doesn't have an effective address");
            
        case MODE_ADDRESS_REGISTER_DIRECT:
            throw std::runtime_error("Address register direct mode doesn't have an effective address");
            
        case MODE_ADDRESS_REGISTER_INDIRECT:
            return m_registers.a[reg];
            
        case MODE_ADDRESS_REGISTER_INDIRECT_POSTINC: {
            uint32_t address = m_registers.a[reg];
            // Size of increment depends on operand size and register (A7 has special handling)
            int size = (instruction >> 12) & 0x3;
            int increment = (size == 0) ? 1 : (size == 1) ? 2 : 4;
            // Special handling for stack pointer (A7) and byte operations
            if (reg == 7 && size == 0) increment = 2;
            m_registers.a[reg] += increment;
            return address;
        }
            
        case MODE_ADDRESS_REGISTER_INDIRECT_PREDEC: {
            // Size of decrement depends on operand size and register (A7 has special handling)
            int size = (instruction >> 12) & 0x3;
            int decrement = (size == 0) ? 1 : (size == 1) ? 2 : 4;
            // Special handling for stack pointer (A7) and byte operations
            if (reg == 7 && size == 0) decrement = 2;
            m_registers.a[reg] -= decrement;
            return m_registers.a[reg];
        }
            
        case MODE_ADDRESS_REGISTER_INDIRECT_DISPLACEMENT: {
            int16_t displacement = readWord(m_registers.pc);
            m_registers.pc += 2;
            m_cycles += 4;  // Extra cycles for displacement fetch
            return m_registers.a[reg] + displacement;
        }
            
        case MODE_ADDRESS_REGISTER_INDIRECT_INDEX: {
            uint16_t extension = readWord(m_registers.pc);
            m_registers.pc += 2;
            m_cycles += 4;  // Extra cycles for extension word fetch
            
            // Extract index register, size, and displacement from extension word
            int idx_reg = (extension >> 12) & 0x7;
            bool idx_is_a = ((extension >> 15) & 0x1) != 0;
            bool idx_long = ((extension >> 11) & 0x1) != 0;
            int8_t displacement = extension & 0xFF;
            
            // Calculate indexed address
            uint32_t index_value = idx_is_a ? m_registers.a[idx_reg] : m_registers.d[idx_reg];
            if (!idx_long) index_value = (int16_t)(index_value & 0xFFFF);  // Sign extend if word
            
            return m_registers.a[reg] + index_value + displacement;
        }
            
        case MODE_SPECIAL:
            switch (reg) {
                case REG_ABSOLUTE_SHORT: {
                    int16_t address = readWord(m_registers.pc);
                    m_registers.pc += 2;
                    m_cycles += 4;  // Extra cycles for address fetch
                    return (uint32_t)(int32_t)address;
                }
                    
                case REG_ABSOLUTE_LONG: {
                    uint32_t address = readLong(m_registers.pc);
                    m_registers.pc += 4;
                    m_cycles += 8;  // Extra cycles for long address fetch
                    return address;
                }
                    
                case REG_PC_DISPLACEMENT: {
                    int16_t displacement = readWord(m_registers.pc);
                    uint32_t base_pc = m_registers.pc;
                    m_registers.pc += 2;
                    m_cycles += 4;  // Extra cycles for displacement fetch
                    return base_pc + displacement;
                }
                    
                case REG_PC_INDEX: {
                    uint16_t extension = readWord(m_registers.pc);
                    uint32_t base_pc = m_registers.pc;
                    m_registers.pc += 2;
                    m_cycles += 4;  // Extra cycles for extension word fetch
                    
                    // Extract index register, size, and displacement from extension word
                    int idx_reg = (extension >> 12) & 0x7;
                    bool idx_is_a = ((extension >> 15) & 0x1) != 0;
                    bool idx_long = ((extension >> 11) & 0x1) != 0;
                    int8_t displacement = extension & 0xFF;
                    
                    // Calculate indexed address
                    uint32_t index_value = idx_is_a ? m_registers.a[idx_reg] : m_registers.d[idx_reg];
                    if (!idx_long) index_value = (int16_t)(index_value & 0xFFFF);  // Sign extend if word
                    
                    return base_pc + index_value + displacement;
                }
                    
                case REG_IMMEDIATE:
                    throw std::runtime_error("Immediate mode doesn't have an effective address");
                    
                default:
                    throw std::runtime_error("Invalid special mode register");
            }
            
        default:
            throw std::runtime_error("Invalid addressing mode");
    }
}

// Instruction handler implementations
void M68000CPU::handleMOVE(uint16_t instruction) {
    // Extract the size from the instruction
    int size = (instruction >> 12) & 0x3;
    int bytes = (size == 1) ? 1 : (size == 3) ? 2 : 4;
    
    // Extract source and destination addressing modes and registers
    int src_mode = (instruction >> 3) & 0x7;
    int src_reg = instruction & 0x7;
    int dst_mode = (instruction >> 6) & 0x7;
    int dst_reg = (instruction >> 9) & 0x7;
    
    // Calculate source effective address and read value
    uint32_t value;
    if (src_mode == MODE_DATA_REGISTER_DIRECT) {
        value = m_registers.d[src_reg];
        if (bytes == 1) value &= 0xFF;
        else if (bytes == 2) value &= 0xFFFF;
        m_cycles += 4;
    } 
    else if (src_mode == MODE_ADDRESS_REGISTER_DIRECT) {
        value = m_registers.a[src_reg];
        if (bytes == 1) value &= 0xFF;
        else if (bytes == 2) value &= 0xFFFF;
        m_cycles += 4;
    } 
    else if (src_mode == MODE_SPECIAL && src_reg == REG_IMMEDIATE) {
        if (bytes == 1) {
            value = readWord(m_registers.pc) & 0xFF;
            m_registers.pc += 2;
            m_cycles += 4;
        } 
        else if (bytes == 2) {
            value = readWord(m_registers.pc);
            m_registers.pc += 2;
            m_cycles += 4;
        } 
        else { // bytes == 4
            value = readLong(m_registers.pc);
            m_registers.pc += 4;
            m_cycles += 8;
        }
    } 
    else {
        uint32_t src_addr = calculateEffectiveAddress(instruction, src_mode, src_reg);
        if (bytes == 1) {
            value = readByte(src_addr);
            m_cycles += 4;
        } 
        else if (bytes == 2) {
            value = readWord(src_addr);
            m_cycles += 4;
        } 
        else { // bytes == 4
            value = readLong(src_addr);
            m_cycles += 8;
        }
    }
    
    // Update condition codes
    m_registers.sr &= ~(SR_NEGATIVE | SR_ZERO | SR_OVERFLOW | SR_CARRY);
    if ((bytes == 1 && (value & 0x80)) ||
        (bytes == 2 && (value & 0x8000)) ||
        (bytes == 4 && (value & 0x80000000))) {
        m_registers.sr |= SR_NEGATIVE;
    }
    if (((bytes == 1) && ((value & 0xFF) == 0)) ||
        ((bytes == 2) && ((value & 0xFFFF) == 0)) ||
        ((bytes == 4) && (value == 0))) {
        m_registers.sr |= SR_ZERO;
    }
    
    // Write to destination
    if (dst_mode == MODE_DATA_REGISTER_DIRECT) {
        if (bytes == 1) {
            m_registers.d[dst_reg] = (m_registers.d[dst_reg] & 0xFFFFFF00) | (value & 0xFF);
        } 
        else if (bytes == 2) {
            m_registers.d[dst_reg] = (m_registers.d[dst_reg] & 0xFFFF0000) | (value & 0xFFFF);
        } 
        else { // bytes == 4
            m_registers.d[dst_reg] = value;
        }
        m_cycles += 4;
    } 
    else if (dst_mode == MODE_ADDRESS_REGISTER_DIRECT) {
        if (bytes == 1) {
            // Byte operations not allowed on address registers
            throw std::runtime_error("Illegal byte operation on address register");
        } 
        else if (bytes == 2) {
            // Sign extend word to long for address registers
            m_registers.a[dst_reg] = (int32_t)(int16_t)(value & 0xFFFF);
        } 
        else { // bytes == 4
            m_registers.a[dst_reg] = value;
        }
        m_cycles += 4;
    } 
    else {
        uint32_t dst_addr = calculateEffectiveAddress(instruction, dst_mode, dst_reg);
        if (bytes == 1) {
            writeByte(dst_addr, value & 0xFF);
            m_cycles += 4;
        } 
        else if (bytes == 2) {
            writeWord(dst_addr, value & 0xFFFF);
            m_cycles += 4;
        } 
        else { // bytes == 4
            writeLong(dst_addr, value);
            m_cycles += 8;
        }
    }
}

void M68000CPU::handleADD(uint16_t instruction) {
    // Extract operation mode and size
    int op_mode = (instruction >> 6) & 0x7;
    int size = op_mode & 0x3;  // 0=byte, 1=word, 2=long
    bool mem_to_reg = ((op_mode >> 2) & 0x1) == 0;
    int bytes = (size == 0) ? 1 : (size == 1) ? 2 : 4;
    
    // Extract data register and effective address
    int reg = (instruction >> 9) & 0x7;
    int ea_mode = (instruction >> 3) & 0x7;
    int ea_reg = instruction & 0x7;
    
    // Calculate values based on direction
    uint32_t reg_value = m_registers.d[reg];
    if (bytes == 1) reg_value &= 0xFF;
    else if (bytes == 2) reg_value &= 0xFFFF;
    
    uint32_t ea_value;
    if (mem_to_reg) {
        // Memory/register to register
        if (ea_mode == MODE_DATA_REGISTER_DIRECT) {
            ea_value = m_registers.d[ea_reg];
            if (bytes == 1) ea_value &= 0xFF;
            else if (bytes == 2) ea_value &= 0xFFFF;
            m_cycles += 4;
        } 
        else if (ea_mode == MODE_ADDRESS_REGISTER_DIRECT) {
            ea_value = m_registers.a[ea_reg];
            if (bytes == 1) ea_value &= 0xFF;
            else if (bytes == 2) ea_value &= 0xFFFF;
            m_cycles += 4;
        }
        else if (ea_mode == MODE_SPECIAL && ea_reg == REG_IMMEDIATE) {
            if (bytes == 1) {
                ea_value = readWord(m_registers.pc) & 0xFF;
                m_registers.pc += 2;
                m_cycles += 4;
            } 
            else if (bytes == 2) {
                ea_value = readWord(m_registers.pc);
                m_registers.pc += 2;
                m_cycles += 4;
            } 
            else { // bytes == 4
                ea_value = readLong(m_registers.pc);
                m_registers.pc += 4;
                m_cycles += 8;
            }
        } 
        else {
            uint32_t ea_addr = calculateEffectiveAddress(instruction, ea_mode, ea_reg);
            if (bytes == 1) {
                ea_value = readByte(ea_addr);
                m_cycles += 4;
            } 
            else if (bytes == 2) {
                ea_value = readWord(ea_addr);
                m_cycles += 4;
            } 
            else { // bytes == 4
                ea_value = readLong(ea_addr);
                m_cycles += 8;
            }
        }
        
        // Perform addition
        uint32_t result = reg_value + ea_value;
        
        // Update condition codes
        bool reg_neg = (bytes == 1 && (reg_value & 0x80)) || 
                       (bytes == 2 && (reg_value & 0x8000)) || 
                       (bytes == 4 && (reg_value & 0x80000000));
        bool ea_neg = (bytes == 1 && (ea_value & 0x80)) || 
                      (bytes == 2 && (ea_value & 0x8000)) || 
                      (bytes == 4 && (ea_value & 0x80000000));
        bool result_neg = (bytes == 1 && (result & 0x80)) || 
                          (bytes == 2 && (result & 0x8000)) || 
                          (bytes == 4 && (result & 0x80000000));
                          
        bool carry = (bytes == 1 && (result > 0xFF)) || 
                     (bytes == 2 && (result > 0xFFFF)) || 
                     (bytes == 4 && (result > 0xFFFFFFFF));
                     
        bool overflow = (reg_neg && ea_neg && !result_neg) || 
                        (!reg_neg && !ea_neg && result_neg);
                        
        m_registers.sr &= ~(SR_NEGATIVE | SR_ZERO | SR_OVERFLOW | SR_CARRY | SR_EXTEND);
        if (result_neg) m_registers.sr |= SR_NEGATIVE;
        if (bytes == 1 && (result & 0xFF) == 0) m_registers.sr |= SR_ZERO;
        else if (bytes == 2 && (result & 0xFFFF) == 0) m_registers.sr |= SR_ZERO;
        else if (bytes == 4 && result == 0) m_registers.sr |= SR_ZERO;
        if (overflow) m_registers.sr |= SR_OVERFLOW;
        if (carry) m_registers.sr |= (SR_CARRY | SR_EXTEND);
        
        // Update register
        if (bytes == 1) {
            m_registers.d[reg] = (m_registers.d[reg] & 0xFFFFFF00) | (result & 0xFF);
        } else if (bytes == 2) {
            m_registers.d[reg] = (m_registers.d[reg] & 0xFFFF0000) | (result & 0xFFFF);
        } else { // bytes == 4
            m_registers.d[reg] = result;
        }
    } else {
        // Register to memory
        uint32_t ea_addr = calculateEffectiveAddress(instruction, ea_mode, ea_reg);
        uint32_t ea_value;
        
        if (bytes == 1) {
            ea_value = readByte(ea_addr);
            m_cycles += 4;
        } else if (bytes == 2) {
            ea_value = readWord(ea_addr);
            m_cycles += 4;
        } else { // bytes == 4
            ea_value = readLong(ea_addr);
            m_cycles += 8;
        }
        
        // Perform addition
        uint32_t result = ea_value + reg_value;
        
        // Update condition codes
        bool reg_neg = (bytes == 1 && (reg_value & 0x80)) || 
                       (bytes == 2 && (reg_value & 0x8000)) || 
                       (bytes == 4 && (reg_value & 0x80000000));
        bool ea_neg = (bytes == 1 && (ea_value & 0x80)) || 
                      (bytes == 2 && (ea_value & 0x8000)) || 
                      (bytes == 4 && (ea_value & 0x80000000));
        bool result_neg = (bytes == 1 && (result & 0x80)) || 
                          (bytes == 2 && (result & 0x8000)) || 
                          (bytes == 4 && (result & 0x80000000));
                          
        bool carry = (bytes == 1 && (result > 0xFF)) || 
                     (bytes == 2 && (result > 0xFFFF)) || 
                     (bytes == 4 && (result > 0xFFFFFFFF));
                     
        bool overflow = (reg_neg && ea_neg && !result_neg) || 
                        (!reg_neg && !ea_neg && result_neg);
        
        m_registers.sr &= ~(SR_NEGATIVE | SR_ZERO | SR_OVERFLOW | SR_CARRY | SR_EXTEND);
        if (result_neg) m_registers.sr |= SR_NEGATIVE;
        if (bytes == 1 && (result & 0xFF) == 0) m_registers.sr |= SR_ZERO;
        else if (bytes == 2 && (result & 0xFFFF) == 0) m_registers.sr |= SR_ZERO;
        else if (bytes == 4 && result == 0) m_registers.sr |= SR_ZERO;
        if (overflow) m_registers.sr |= SR_OVERFLOW;
        if (carry) m_registers.sr |= (SR_CARRY | SR_EXTEND);
        
        // Write result back to memory
        if (bytes == 1) {
            writeByte(ea_addr, result & 0xFF);
            m_cycles += 4;
        } else if (bytes == 2) {
            writeWord(ea_addr, result & 0xFFFF);
            m_cycles += 4;
        } else { // bytes == 4
            writeLong(ea_addr, result);
            m_cycles += 8;
        }
    }