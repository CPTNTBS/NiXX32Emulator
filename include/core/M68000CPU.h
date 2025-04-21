/**
 * M68000CPU.h
 * Motorola 68000 CPU emulation for NiXX-32 arcade board
 * 
 * This file defines the emulation of the primary 68000 CPU that handles
 * the main game logic, physics calculations, and graphics coordination
 * in the NiXX-32 arcade system.
 */

 #pragma once

 #include <cstdint>
 #include <vector>
 #include <string>
 #include <functional>
 #include <memory>
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class MemoryManager;
 class Logger;
 
 /**
  * Defines the possible CPU execution states
  */
 enum class CPUState {
	 RUNNING,    // Normal execution
	 HALTED,     // CPU is stopped (STOP instruction)
	 EXCEPTION,  // CPU is handling an exception
	 RESET       // CPU is in reset state
 };
 
 /**
  * Defines the different privilege levels
  */
 enum class PrivilegeMode {
	 USER,       // User mode
	 SUPERVISOR  // Supervisor mode
 };
 
 /**
  * Exception types that can be triggered
  */
 enum class ExceptionType {
	 RESET,              // Reset exception
	 BUS_ERROR,          // Bus error
	 ADDRESS_ERROR,      // Address error
	 ILLEGAL_INSTRUCTION, // Illegal instruction
	 ZERO_DIVIDE,        // Division by zero
	 CHK_INSTRUCTION,    // CHK instruction
	 TRAPV_INSTRUCTION,  // TRAPV instruction
	 PRIVILEGE_VIOLATION, // Privilege violation
	 TRACE,              // Trace
	 LINE_A_EMULATOR,    // Line-A emulator
	 LINE_F_EMULATOR,    // Line-F emulator
	 INTERRUPT,          // Interrupt
	 TRAP                // TRAP instruction
 };
 
 /**
  * Motorola 68000 register set
  */
 struct M68000Registers {
	 // Data registers (D0-D7)
	 uint32_t d[8];
	 
	 // Address registers (A0-A7/SP)
	 uint32_t a[8];
	 
	 // Program counter
	 uint32_t pc;
	 
	 // Status register
	 uint16_t sr;
	 
	 // User stack pointer (USP)
	 uint32_t usp;
	 
	 // Supervisor stack pointer (SSP)
	 uint32_t ssp;
	 
	 // Instruction prefetch
	 uint16_t prefetch[2];
 };
 
 /**
  * Condition codes within the status register
  */
 enum StatusRegisterBits {
	 SR_C = 0x0001,  // Carry
	 SR_V = 0x0002,  // Overflow
	 SR_Z = 0x0004,  // Zero
	 SR_N = 0x0008,  // Negative
	 SR_X = 0x0010,  // Extend
	 SR_S = 0x2000,  // Supervisor mode
	 SR_T = 0x8000   // Trace mode
 };
 
 /**
  * Interrupt mask levels
  */
 enum InterruptLevel {
	 IPL_NONE = 0,
	 IPL_1 = 1,
	 IPL_2 = 2,
	 IPL_3 = 3,
	 IPL_4 = 4,
	 IPL_5 = 5,
	 IPL_6 = 6,
	 IPL_7 = 7  // Non-maskable
 };
 
 /**
  * Class for emulating the Motorola 68000 CPU
  */
 class M68000CPU {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 M68000CPU(System& system, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~M68000CPU();
	 
	 /**
	  * Initialize the CPU
	  * @param clockSpeed CPU clock speed in MHz
	  * @return True if initialization was successful
	  */
	 bool Initialize(uint32_t clockSpeed);
	 
	 /**
	  * Reset the CPU to initial state
	  */
	 void Reset();
	 
	 /**
	  * Execute a specified number of cycles
	  * @param cycles Number of cycles to execute
	  * @return Actual number of cycles executed
	  */
	 int Execute(int cycles);
	 
	 /**
	  * Execute a single instruction
	  * @return Number of cycles used
	  */
	 int ExecuteInstruction();
	 
	 /**
	  * Trigger a CPU exception
	  * @param type Exception type
	  * @param vector Vector number (if applicable)
	  */
	 void TriggerException(ExceptionType type, int vector = 0);
	 
	 /**
	  * Set an interrupt level
	  * @param level Interrupt level (0-7)
	  */
	 void SetInterruptLevel(InterruptLevel level);
	 
	 /**
	  * Get the current CPU state
	  * @return Current CPU state
	  */
	 CPUState GetState() const;
	 
	 /**
	  * Get the current privilege mode
	  * @return Current privilege mode
	  */
	 PrivilegeMode GetPrivilegeMode() const;
	 
	 /**
	  * Get the current PC value
	  * @return Program counter value
	  */
	 uint32_t GetPC() const;
	 
	 /**
	  * Set the PC to a new value
	  * @param newPC New program counter value
	  */
	 void SetPC(uint32_t newPC);
	 
	 /**
	  * Get a copy of all CPU registers
	  * @return Current register state
	  */
	 M68000Registers GetRegisters() const;
	 
	 /**
	  * Set all CPU registers
	  * @param registers New register state
	  */
	 void SetRegisters(const M68000Registers& registers);
	 
	 /**
	  * Get the value of a data register
	  * @param regNum Register number (0-7)
	  * @return Register value
	  */
	 uint32_t GetDataRegister(int regNum) const;
	 
	 /**
	  * Set the value of a data register
	  * @param regNum Register number (0-7)
	  * @param value New register value
	  */
	 void SetDataRegister(int regNum, uint32_t value);
	 
	 /**
	  * Get the value of an address register
	  * @param regNum Register number (0-7)
	  * @return Register value
	  */
	 uint32_t GetAddressRegister(int regNum) const;
	 
	 /**
	  * Set the value of an address register
	  * @param regNum Register number (0-7)
	  * @param value New register value
	  */
	 void SetAddressRegister(int regNum, uint32_t value);
	 
	 /**
	  * Get the current status register value
	  * @return Status register value
	  */
	 uint16_t GetSR() const;
	 
	 /**
	  * Set the status register value
	  * @param value New status register value
	  */
	 void SetSR(uint16_t value);
	 
	 /**
	  * Check if a status register flag is set
	  * @param flag Flag to check
	  * @return True if the flag is set
	  */
	 bool IsFlagSet(StatusRegisterBits flag) const;
	 
	 /**
	  * Set a status register flag
	  * @param flag Flag to set
	  * @param value True to set, false to clear
	  */
	 void SetFlag(StatusRegisterBits flag, bool value);
	 
	 /**
	  * Get the current cycle count
	  * @return Total cycles executed since reset
	  */
	 uint64_t GetCycleCount() const;
	 
	 /**
	  * Get the CPU clock speed
	  * @return Clock speed in MHz
	  */
	 uint32_t GetClockSpeed() const;
	 
	 /**
	  * Disassemble the instruction at the given address
	  * @param address Memory address
	  * @param instructionSize Output parameter for instruction size
	  * @return Disassembled instruction string
	  */
	 std::string DisassembleInstruction(uint32_t address, int& instructionSize);
	 
	 /**
	  * Register a callback for CPU hooks
	  * @param address Address to hook
	  * @param callback Function to call when the address is executed
	  * @return True if hook was registered successfully
	  */
	 bool RegisterHook(uint32_t address, std::function<void()> callback);
	 
	 /**
	  * Remove a hook at the specified address
	  * @param address Address to remove hook from
	  * @return True if hook was removed successfully
	  */
	 bool RemoveHook(uint32_t address);
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // CPU registers
	 M68000Registers m_registers;
	 
	 // Current CPU state
	 CPUState m_state;
	 
	 // Current interrupt level
	 InterruptLevel m_interruptLevel;
	 
	 // Clock speed in MHz
	 uint32_t m_clockSpeed;
	 
	 // Total cycle count since reset
	 uint64_t m_cycleCount;
	 
	 // Pending cycles for current instruction
	 int m_pendingCycles;
	 
	 // Instruction hook map
	 std::unordered_map<uint32_t, std::function<void()>> m_hooks;
	 
	 // Prefetch queue management
	 void RefillPrefetchQueue();
	 uint16_t FetchNextInstruction();
	 
	 // Instruction execution helpers
	 void ExecuteGroup0(uint16_t opcode);
	 void ExecuteGroup1(uint16_t opcode);
	 void ExecuteGroup2(uint16_t opcode);
	 void ExecuteGroup3(uint16_t opcode);
	 void ExecuteGroup4(uint16_t opcode);
	 void ExecuteGroup5(uint16_t opcode);
	 void ExecuteGroup6(uint16_t opcode);
	 void ExecuteGroup7(uint16_t opcode);
	 void ExecuteGroup8(uint16_t opcode);
	 void ExecuteGroup9(uint16_t opcode);
	 void ExecuteGroupA(uint16_t opcode);
	 void ExecuteGroupB(uint16_t opcode);
	 void ExecuteGroupC(uint16_t opcode);
	 void ExecuteGroupD(uint16_t opcode);
	 void ExecuteGroupE(uint16_t opcode);
	 void ExecuteGroupF(uint16_t opcode);
	 
	 // Stack operations
	 void PushLong(uint32_t value);
	 void PushWord(uint16_t value);
	 uint32_t PopLong();
	 uint16_t PopWord();
	 
	 // Addressing mode handlers
	 uint32_t CalculateEffectiveAddress(uint16_t mode, uint16_t reg);
	 
	 // Helper for switching privilege modes
	 void SwitchPrivilegeMode(PrivilegeMode newMode);
	 
	 // Process pending interrupts
	 bool CheckPendingInterrupts();
	 
	 // Calculate condition code based on instruction results
	 void UpdateConditionCodes(uint32_t src, uint32_t dst, uint32_t result, uint16_t size, bool isAdd);
 };
 
 } // namespace NiXX32