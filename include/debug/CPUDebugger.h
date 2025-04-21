/**
 * CPUDebugger.h
 * CPU-specific debugging for NiXX-32 arcade board emulation
 * 
 * This file defines the CPU-specific debugging functionality that provides
 * breakpoints, register inspection, instruction tracing, and step execution
 * for the 68000 main CPU and Z80 audio CPU in the NiXX-32 arcade system.
 */

 #pragma once

 #include <cstdint>
 #include <string>
 #include <vector>
 #include <functional>
 #include <unordered_map>
 #include <unordered_set>
 #include <memory>
 
 #include "MemoryManager.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class Debugger;
 class M68000CPU;
 class Z80CPU;
 class M68000Registers;
 class Z80Registers;
 
 /**
  * CPU trace entry structure
  */
 struct TraceEntry {
	 uint32_t address;           // Instruction address
	 std::string disassembly;    // Disassembled instruction
	 uint64_t cycleCount;        // CPU cycle count when executed
	 union {
		 M68000Registers* m68kRegs;  // 68000 registers state (if applicable)
		 Z80Registers* z80Regs;      // Z80 registers state (if applicable)
	 };
	 uint32_t timestamp;         // System timestamp
 };
 
 /**
  * CPU instruction hook types
  */
 enum class InstructionHookType {
	 PRE_EXECUTION,    // Hook called before instruction execution
	 POST_EXECUTION,   // Hook called after instruction execution
	 EXCEPTION,        // Hook called on CPU exception
	 INTERRUPT         // Hook called on CPU interrupt
 };
 
 /**
  * CPU register types for 68000
  */
 enum class M68000RegisterType {
	 DATA,             // Data registers (D0-D7)
	 ADDRESS,          // Address registers (A0-A7)
	 PC,               // Program counter
	 SR,               // Status register
	 USP,              // User stack pointer
	 SSP,              // Supervisor stack pointer
	 OTHER             // Other special registers
 };
 
 /**
  * CPU register types for Z80
  */
 enum class Z80RegisterType {
	 MAIN,             // Main register set (A, F, B, C, D, E, H, L)
	 ALTERNATE,        // Alternate register set (A', F', B', C', D', E', H', L')
	 INDEX,            // Index registers (IX, IY)
	 SPECIAL,          // Special registers (SP, PC, I, R)
	 OTHER             // Other special registers
 };
 
 /**
  * CPU execution step modes
  */
 enum class StepMode {
	 INSTRUCTION,      // Step one instruction
	 OVER,             // Step over subroutine calls
	 OUT,              // Step out of current subroutine
	 UNTIL_RETURN      // Execute until return instruction
 };
 
 /**
  * Instruction information structure
  */
 struct InstructionInfo {
	 uint32_t address;            // Instruction address
	 std::string disassembly;     // Disassembled instruction
	 uint32_t size;               // Instruction size in bytes
	 std::vector<uint8_t> bytes;  // Raw instruction bytes
	 bool isCall;                 // Is this a subroutine call instruction
	 bool isReturn;               // Is this a return instruction
	 bool isBranch;               // Is this a branch instruction
	 bool isJump;                 // Is this an unconditional jump
 };
 
 /**
  * Class for CPU-specific debugging functionality
  */
 class CPUDebugger {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param debugger Reference to the main debugger
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  * @param cpuType Type of CPU this debugger is for
	  */
	 CPUDebugger(System& system, Debugger& debugger, 
				 MemoryManager& memoryManager, Logger& logger,
				 CPUType cpuType);
	 
	 /**
	  * Destructor
	  */
	 ~CPUDebugger();
	 
	 /**
	  * Initialize the CPU debugger
	  * @return True if initialization was successful
	  */
	 bool Initialize();
	 
	 /**
	  * Reset the CPU debugger state
	  */
	 void Reset();
	 
	 /**
	  * Update CPU debugger state
	  */
	 void Update();
	 
	 /**
	  * Get the CPU type this debugger is for
	  * @return CPU type
	  */
	 CPUType GetCPUType() const;
	 
	 /**
	  * Set execution breakpoint at specified address
	  * @param address Address to set breakpoint at
	  * @param temporary True for one-time breakpoint
	  * @return Breakpoint ID if successful, -1 otherwise
	  */
	 int SetBreakpoint(uint32_t address, bool temporary = false);
	 
	 /**
	  * Remove execution breakpoint
	  * @param address Address to remove breakpoint from
	  * @return True if breakpoint was removed
	  */
	 bool RemoveBreakpoint(uint32_t address);
	 
	 /**
	  * Check if an address has a breakpoint
	  * @param address Address to check
	  * @return True if address has a breakpoint
	  */
	 bool HasBreakpoint(uint32_t address) const;
	 
	 /**
	  * Set memory read breakpoint
	  * @param address Address to monitor for reads
	  * @return Breakpoint ID if successful, -1 otherwise
	  */
	 int SetReadBreakpoint(uint32_t address);
	 
	 /**
	  * Set memory write breakpoint
	  * @param address Address to monitor for writes
	  * @return Breakpoint ID if successful, -1 otherwise
	  */
	 int SetWriteBreakpoint(uint32_t address);
	 
	 /**
	  * Set memory access breakpoint (read or write)
	  * @param address Address to monitor for access
	  * @return Breakpoint ID if successful, -1 otherwise
	  */
	 int SetAccessBreakpoint(uint32_t address);
	 
	 /**
	  * Get the current program counter value
	  * @return Program counter value
	  */
	 uint32_t GetPC() const;
	 
	 /**
	  * Set the program counter to a new value
	  * @param pc New program counter value
	  * @return True if successful
	  */
	 bool SetPC(uint32_t pc);
	 
	 /**
	  * Get disassembly at current PC
	  * @param instructionCount Number of instructions to disassemble
	  * @return Vector of disassembled instructions
	  */
	 std::vector<InstructionInfo> GetDisassembly(uint32_t instructionCount = 10);
	 
	 /**
	  * Get disassembly at specified address
	  * @param address Address to start disassembly from
	  * @param instructionCount Number of instructions to disassemble
	  * @return Vector of disassembled instructions
	  */
	 std::vector<InstructionInfo> GetDisassemblyAt(uint32_t address, uint32_t instructionCount = 10);
	 
	 /**
	  * Get register value by name
	  * @param registerName Register name
	  * @return Register value, or 0 if not found
	  */
	 uint32_t GetRegisterValue(const std::string& registerName) const;
	 
	 /**
	  * Set register value by name
	  * @param registerName Register name
	  * @param value New register value
	  * @return True if successful
	  */
	 bool SetRegisterValue(const std::string& registerName, uint32_t value);
	 
	 /**
	  * Get all register values
	  * @return Map of register names to values
	  */
	 std::unordered_map<std::string, uint32_t> GetAllRegisters() const;
	 
	 /**
	  * Step execution by one instruction
	  * @return True if successful
	  */
	 bool Step();
	 
	 /**
	  * Step execution with specified mode
	  * @param mode Step mode
	  * @return True if successful
	  */
	 bool StepWithMode(StepMode mode);
	 
	 /**
	  * Run until specified address is reached
	  * @param address Address to run to
	  * @return True if successful
	  */
	 bool RunTo(uint32_t address);
	 
	 /**
	  * Run until return instruction
	  * @return True if successful
	  */
	 bool RunUntilReturn();
	 
	 /**
	  * Register an instruction hook
	  * @param type Hook type
	  * @param callback Function to call when hook is triggered
	  * @return Hook ID
	  */
	 int RegisterInstructionHook(InstructionHookType type,
								std::function<void(uint32_t, void*)> callback);
	 
	 /**
	  * Remove an instruction hook
	  * @param hookId Hook ID
	  * @return True if successful
	  */
	 bool RemoveInstructionHook(int hookId);
	 
	 /**
	  * Set tracing enabled
	  * @param enabled True to enable tracing, false to disable
	  */
	 void SetTraceEnabled(bool enabled);
	 
	 /**
	  * Check if tracing is enabled
	  * @return True if tracing is enabled
	  */
	 bool IsTraceEnabled() const;
	 
	 /**
	  * Get the execution trace
	  * @param count Number of trace entries to get (0 for all)
	  * @return Vector of trace entries
	  */
	 std::vector<TraceEntry> GetTrace(uint32_t count = 0) const;
	 
	 /**
	  * Clear the execution trace
	  */
	 void ClearTrace();
	 
	 /**
	  * Set the maximum trace size
	  * @param maxEntries Maximum number of trace entries to keep
	  */
	 void SetMaxTraceSize(uint32_t maxEntries);
	 
	 /**
	  * Save trace to file
	  * @param filename File name to save to
	  * @return True if successful
	  */
	 bool SaveTraceToFile(const std::string& filename) const;
	 
	 /**
	  * Get direct access to 68000 CPU (if applicable)
	  * @return Pointer to 68000 CPU, or nullptr if not applicable
	  */
	 M68000CPU* GetM68000CPU() const;
	 
	 /**
	  * Get direct access to Z80 CPU (if applicable)
	  * @return Pointer to Z80 CPU, or nullptr if not applicable
	  */
	 Z80CPU* GetZ80CPU() const;
	 
	 /**
	  * Get a stack trace (call stack)
	  * @param maxDepth Maximum stack depth to traverse
	  * @return Vector of call stack entries (address, function name if known)
	  */
	 std::vector<std::pair<uint32_t, std::string>> GetStackTrace(uint32_t maxDepth = 16) const;
	 
	 /**
	  * Get instruction info at address
	  * @param address Address to get info for
	  * @return Instruction information
	  */
	 InstructionInfo GetInstructionInfo(uint32_t address) const;
	 
	 /**
	  * Get memory value at address
	  * @param address Address to read from
	  * @param size Size to read (1, 2, or 4 bytes)
	  * @return Memory value
	  */
	 uint32_t GetMemoryValue(uint32_t address, uint8_t size = 1) const;
	 
	 /**
	  * Set memory value at address
	  * @param address Address to write to
	  * @param value Value to write
	  * @param size Size to write (1, 2, or 4 bytes)
	  * @return True if successful
	  */
	 bool SetMemoryValue(uint32_t address, uint32_t value, uint8_t size = 1);
	 
	 /**
	  * Get execution cycle count
	  * @return Current cycle count
	  */
	 uint64_t GetCycleCount() const;
	 
	 /**
	  * Get current CPU state string
	  * @return State description string
	  */
	 std::string GetStateString() const;
	 
	 /**
	  * Get CPU specific register set
	  * @param regs Output parameter for register data
	  * @return True if successful
	  */
	 bool GetRegisterSet(void* regs) const;
	 
	 /**
	  * Set CPU specific register set
	  * @param regs Register data to set
	  * @return True if successful
	  */
	 bool SetRegisterSet(const void* regs);
	 
	 /**
	  * Add a memory region symbol
	  * @param address Address of symbol
	  * @param name Symbol name
	  * @param size Size of symbol (0 for automatic)
	  * @return True if successful
	  */
	 bool AddSymbol(uint32_t address, const std::string& name, uint32_t size = 0);
	 
	 /**
	  * Remove a symbol
	  * @param address Address of symbol
	  * @return True if successful
	  */
	 bool RemoveSymbol(uint32_t address);
	 
	 /**
	  * Get symbol at address
	  * @param address Address to look up
	  * @return Symbol name, or empty string if not found
	  */
	 std::string GetSymbolAt(uint32_t address) const;
	 
	 /**
	  * Get address of named symbol
	  * @param name Symbol name
	  * @return Address, or 0 if not found
	  */
	 uint32_t GetSymbolAddress(const std::string& name) const;
	 
	 /**
	  * Get all defined symbols
	  * @return Map of addresses to symbol info (name, size)
	  */
	 std::unordered_map<uint32_t, std::pair<std::string, uint32_t>> GetAllSymbols() const;
	 
	 /**
	  * Load symbols from file
	  * @param filename File to load from
	  * @return True if successful
	  */
	 bool LoadSymbolsFromFile(const std::string& filename);
	 
	 /**
	  * Save symbols to file
	  * @param filename File to save to
	  * @return True if successful
	  */
	 bool SaveSymbolsToFile(const std::string& filename) const;
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to main debugger
	 Debugger& m_debugger;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // CPU type this debugger is for
	 CPUType m_cpuType;
	 
	 // Pointers to CPU instances
	 M68000CPU* m_m68000Cpu;
	 Z80CPU* m_z80Cpu;
	 
	 // Tracing state
	 bool m_traceEnabled;
	 uint32_t m_maxTraceSize;
	 std::vector<TraceEntry> m_trace;
	 
	 // Instruction hooks
	 struct InstructionHookInfo {
		 int id;
		 InstructionHookType type;
		 std::function<void(uint32_t, void*)> callback;
	 };
	 std::vector<InstructionHookInfo> m_instructionHooks;
	 int m_nextHookId;
	 
	 // Temporary execution breakpoints
	 std::unordered_set<uint32_t> m_tempBreakpoints;
	 
	 // Step execution state
	 struct StepState {
		 bool active;
		 StepMode mode;
		 uint32_t returnAddress;
		 uint32_t stackPointer;
		 uint32_t targetAddress;
	 };
	 StepState m_stepState;
	 
	 // Symbol table
	 std::unordered_map<uint32_t, std::pair<std::string, uint32_t>> m_symbols;
	 std::unordered_map<std::string, uint32_t> m_symbolsByName;
	 
	 /**
	  * Add a trace entry
	  * @param address Instruction address
	  * @param disassembly Disassembled instruction
	  */
	 void AddTraceEntry(uint32_t address, const std::string& disassembly);
	 
	 /**
	  * Handle instruction execution (for hooks and tracing)
	  * @param address Instruction address
	  * @param before True if before execution, false if after
	  */
	 void HandleInstruction(uint32_t address, bool before);
	 
	 /**
	  * Install CPU-specific hooks
	  */
	 void InstallHooks();
	 
	 /**
	  * Check for step completion
	  * @param currentAddress Current instruction address
	  * @return True if step is complete
	  */
	 bool CheckStepCompletion(uint32_t currentAddress);
	 
	 /**
	  * Get M68000 register name and type
	  * @param registerName Register name
	  * @param type Output parameter for register type
	  * @param index Output parameter for register index
	  * @return True if register was found
	  */
	 bool GetM68000RegisterInfo(const std::string& registerName, 
								M68000RegisterType& type, int& index) const;
	 
	 /**
	  * Get Z80 register name and type
	  * @param registerName Register name
	  * @param type Output parameter for register type
	  * @param index Output parameter for register index
	  * @return True if register was found
	  */
	 bool GetZ80RegisterInfo(const std::string& registerName,
							Z80RegisterType& type, int& index) const;
	 
	 /**
	  * Get subroutine return address based on current context
	  * @return Expected return address, or 0 if unknown
	  */
	 uint32_t GetReturnAddress() const;
	 
	 /**
	  * Get current stack pointer value
	  * @return Stack pointer value
	  */
	 uint32_t GetStackPointer() const;
	 
	 /**
	  * Check if current instruction is a subroutine call
	  * @param address Address to check
	  * @return True if instruction is a call
	  */
	 bool IsCallInstruction(uint32_t address) const;
	 
	 /**
	  * Check if current instruction is a return
	  * @param address Address to check
	  * @return True if instruction is a return
	  */
	 bool IsReturnInstruction(uint32_t address) const;
	 
	 /**
	  * Trigger instruction hooks of specified type
	  * @param type Hook type to trigger
	  * @param address Instruction address
	  * @param data Additional data for the hook
	  */
	 void TriggerInstructionHooks(InstructionHookType type, uint32_t address, void* data = nullptr);
	 
	 /**
	  * Format symbol for display
	  * @param address Address to format
	  * @return Formatted string (address or symbol+offset)
	  */
	 std::string FormatSymbolAddress(uint32_t address) const;
	 
	 /**
	  * Find nearest symbol below an address
	  * @param address Address to search from
	  * @param offset Output parameter for offset from symbol
	  * @return Symbol name, or empty string if not found
	  */
	 std::string FindNearestSymbol(uint32_t address, uint32_t& offset) const;
 };
 
 } // namespace NiXX32