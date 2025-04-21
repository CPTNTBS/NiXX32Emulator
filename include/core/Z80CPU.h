/**
 * Z80CPU.h
 * Zilog Z80 CPU emulation for NiXX-32 arcade board audio subsystem
 * 
 * This file defines the emulation of the secondary Z80 CPU that handles
 * the audio processing and sound generation in the NiXX-32 arcade system.
 */

 #pragma once

 #include <cstdint>
 #include <vector>
 #include <string>
 #include <functional>
 #include <memory>
 #include <unordered_map>
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class MemoryManager;
 class Logger;
 class AudioSystem;
 
 /**
  * Defines the possible CPU execution states
  */
 enum class Z80State {
	 RUNNING,    // Normal execution
	 HALTED,     // CPU is halted (HALT instruction)
	 INTERRUPT,  // CPU is handling an interrupt
	 RESET       // CPU is in reset state
 };
 
 /**
  * Interrupt modes available in Z80
  */
 enum class InterruptMode {
	 IM0,        // Mode 0 (similar to 8080)
	 IM1,        // Mode 1 (RST 38h)
	 IM2         // Mode 2 (vectored)
 };
 
 /**
  * Types of interrupts that can be triggered
  */
 enum class Z80InterruptType {
	 NONE,       // No interrupt
	 INT,        // Maskable interrupt
	 NMI         // Non-maskable interrupt
 };
 
 /**
  * Z80 register set
  */
 struct Z80Registers {
	 // Main register set
	 union {
		 struct {
			 uint8_t f;  // Flags
			 uint8_t a;  // Accumulator
		 };
		 uint16_t af;
	 };
	 
	 union {
		 struct {
			 uint8_t c;
			 uint8_t b;
		 };
		 uint16_t bc;
	 };
	 
	 union {
		 struct {
			 uint8_t e;
			 uint8_t d;
		 };
		 uint16_t de;
	 };
	 
	 union {
		 struct {
			 uint8_t l;
			 uint8_t h;
		 };
		 uint16_t hl;
	 };
	 
	 // Alternate register set
	 union {
		 struct {
			 uint8_t f_;  // Flags'
			 uint8_t a_;  // Accumulator'
		 };
		 uint16_t af_;
	 };
	 
	 union {
		 struct {
			 uint8_t c_;
			 uint8_t b_;
		 };
		 uint16_t bc_;
	 };
	 
	 union {
		 struct {
			 uint8_t e_;
			 uint8_t d_;
		 };
		 uint16_t de_;
	 };
	 
	 union {
		 struct {
			 uint8_t l_;
			 uint8_t h_;
		 };
		 uint16_t hl_;
	 };
	 
	 // Index registers
	 uint16_t ix;  // Index register X
	 uint16_t iy;  // Index register Y
	 
	 // Special purpose registers
	 uint16_t sp;  // Stack pointer
	 uint16_t pc;  // Program counter
	 
	 // Interrupt page address register
	 uint8_t i;
	 
	 // Memory refresh register
	 uint8_t r;
 };
 
 /**
  * Flag bits within the F register
  */
 enum Z80FlagBits {
	 FLAG_C = 0x01,  // Carry
	 FLAG_N = 0x02,  // Subtract/Add
	 FLAG_P = 0x04,  // Parity/Overflow
	 FLAG_X = 0x08,  // Unused (usually 0)
	 FLAG_H = 0x10,  // Half carry
	 FLAG_Y = 0x20,  // Unused (usually 0)
	 FLAG_Z = 0x40,  // Zero
	 FLAG_S = 0x80   // Sign
 };
 
 /**
  * Class for emulating the Zilog Z80 CPU
  */
 class Z80CPU {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  * @param audioSystem Reference to the audio system
	  */
	 Z80CPU(System& system, MemoryManager& memoryManager, Logger& logger, AudioSystem& audioSystem);
	 
	 /**
	  * Destructor
	  */
	 ~Z80CPU();
	 
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
	  * Trigger an interrupt
	  * @param type Interrupt type
	  * @param data Data for vectored interrupts (IM2)
	  * @return True if interrupt was accepted
	  */
	 bool TriggerInterrupt(Z80InterruptType type, uint8_t data = 0);
	 
	 /**
	  * Get the current CPU state
	  * @return Current CPU state
	  */
	 Z80State GetState() const;
	 
	 /**
	  * Get the current interrupt mode
	  * @return Current interrupt mode
	  */
	 InterruptMode GetInterruptMode() const;
	 
	 /**
	  * Check if interrupts are enabled
	  * @return True if interrupts are enabled
	  */
	 bool AreInterruptsEnabled() const;
	 
	 /**
	  * Get the current PC value
	  * @return Program counter value
	  */
	 uint16_t GetPC() const;
	 
	 /**
	  * Set the PC to a new value
	  * @param newPC New program counter value
	  */
	 void SetPC(uint16_t newPC);
	 
	 /**
	  * Get a copy of all CPU registers
	  * @return Current register state
	  */
	 Z80Registers GetRegisters() const;
	 
	 /**
	  * Set all CPU registers
	  * @param registers New register state
	  */
	 void SetRegisters(const Z80Registers& registers);
	 
	 /**
	  * Check if a flag is set
	  * @param flag Flag to check
	  * @return True if the flag is set
	  */
	 bool IsFlagSet(Z80FlagBits flag) const;
	 
	 /**
	  * Set a flag value
	  * @param flag Flag to set
	  * @param value True to set, false to clear
	  */
	 void SetFlag(Z80FlagBits flag, bool value);
	 
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
	 std::string DisassembleInstruction(uint16_t address, int& instructionSize);
	 
	 /**
	  * Register a callback for CPU port I/O
	  * @param port Port number to hook
	  * @param readCallback Function to call when the port is read
	  * @param writeCallback Function to call when the port is written
	  * @return True if hooks were registered successfully
	  */
	 bool RegisterPortHooks(uint8_t port, 
						   std::function<uint8_t()> readCallback,
						   std::function<void(uint8_t)> writeCallback);
	 
	 /**
	  * Remove hooks for the specified port
	  * @param port Port to remove hooks from
	  * @return True if hooks were removed successfully
	  */
	 bool RemovePortHooks(uint8_t port);
	 
	 /**
	  * Register an instruction execution hook
	  * @param address Address to hook
	  * @param callback Function to call when the address is executed
	  * @return True if hook was registered successfully
	  */
	 bool RegisterExecutionHook(uint16_t address, std::function<void()> callback);
	 
	 /**
	  * Remove an execution hook
	  * @param address Address to remove hook from
	  * @return True if hook was removed successfully
	  */
	 bool RemoveExecutionHook(uint16_t address);
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Reference to audio system
	 AudioSystem& m_audioSystem;
	 
	 // CPU registers
	 Z80Registers m_registers;
	 
	 // Current CPU state
	 Z80State m_state;
	 
	 // Current interrupt mode
	 InterruptMode m_interruptMode;
	 
	 // Interrupt flip-flops
	 bool m_iff1;  // Interrupt enable flip-flop 1
	 bool m_iff2;  // Interrupt enable flip-flop 2
	 
	 // Pending interrupt
	 Z80InterruptType m_pendingInterrupt;
	 uint8_t m_interruptData;
	 
	 // Clock speed in MHz
	 uint32_t m_clockSpeed;
	 
	 // Total cycle count since reset
	 uint64_t m_cycleCount;
	 
	 // Pending cycles for current instruction
	 int m_pendingCycles;
	 
	 // Port I/O hook maps
	 std::unordered_map<uint8_t, std::function<uint8_t()>> m_portReadHooks;
	 std::unordered_map<uint8_t, std::function<void(uint8_t)>> m_portWriteHooks;
	 
	 // Instruction execution hook map
	 std::unordered_map<uint16_t, std::function<void()>> m_executionHooks;
	 
	 // Memory access helpers
	 uint8_t ReadByte(uint16_t address);
	 void WriteByte(uint16_t address, uint8_t value);
	 uint16_t ReadWord(uint16_t address);
	 void WriteWord(uint16_t address, uint16_t value);
	 
	 // Port I/O operations
	 uint8_t InPort(uint8_t port);
	 void OutPort(uint8_t port, uint8_t value);
	 
	 // Stack operations
	 void Push(uint16_t value);
	 uint16_t Pop();
	 
	 // Instruction fetch and decode
	 uint8_t FetchByte();
	 uint16_t FetchWord();
	 
	 // Instruction execution helpers
	 int ExecuteMainInstruction(uint8_t opcode);
	 int ExecuteCBPrefixedInstruction(uint8_t opcode);
	 int ExecuteDDPrefixedInstruction(uint8_t opcode);
	 int ExecuteEDPrefixedInstruction(uint8_t opcode);
	 int ExecuteFDPrefixedInstruction(uint8_t opcode);
	 int ExecuteDDCBPrefixedInstruction(uint8_t displacement, uint8_t opcode);
	 int ExecuteFDCBPrefixedInstruction(uint8_t displacement, uint8_t opcode);
	 
	 // Flag calculation helpers
	 uint8_t CalculateParityFlag(uint8_t value);
	 uint8_t CalculateSignZeroFlags(uint8_t value);
	 void UpdateFlagsAfterLogicalOp(uint8_t result);
	 void UpdateFlagsAfterArithmeticOp(uint8_t result, uint8_t operand1, uint8_t operand2, bool isAdd);
	 
	 // Process pending interrupts
	 bool CheckPendingInterrupts();
	 
	 // Handle NMI
	 void HandleNMI();
	 
	 // Handle maskable interrupt
	 void HandleInterrupt();
 };
 
 } // namespace NiXX32