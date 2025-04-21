/**
 * Debugger.h
 * Debugging system for NiXX-32 arcade board emulation
 * 
 * This file defines the core debugging infrastructure that provides monitoring,
 * breakpoints, memory inspection, and diagnostic capabilities for the NiXX-32
 * arcade system emulation.
 */

 #pragma once

 #include <cstdint>
 #include <string>
 #include <vector>
 #include <functional>
 #include <memory>
 #include <unordered_map>
 #include <unordered_set>
 
 #include "MemoryManager.h"
 #include "Logger.h"
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 class M68000CPU;
 class Z80CPU;
 class CPUDebugger;
 class MemoryViewer;
 
 /**
  * Breakpoint types supported by the debugger
  */
 enum class BreakpointType {
	 EXECUTION,      // Break on instruction execution
	 MEMORY_READ,    // Break on memory read
	 MEMORY_WRITE,   // Break on memory write
	 MEMORY_ACCESS,  // Break on any memory access
	 INTERRUPT,      // Break on interrupt
	 IO_READ,        // Break on I/O read
	 IO_WRITE,       // Break on I/O write
	 CONDITION       // Break on custom condition
 };
 
 /**
  * CPU types that can be targeted for debugging
  */
 enum class CPUType {
	 MAIN_CPU,       // 68000 main CPU
	 AUDIO_CPU       // Z80 audio CPU
 };
 
 /**
  * Debug event types
  */
 enum class DebugEventType {
	 BREAKPOINT_HIT,       // Breakpoint was triggered
	 WATCH_TRIGGERED,      // Watchpoint was triggered
	 CPU_EXCEPTION,        // CPU exception occurred
	 CPU_INTERRUPT,        // CPU interrupt occurred
	 ILLEGAL_MEMORY,       // Illegal memory access
	 IO_ACCESS,            // I/O port access
	 LOG_MESSAGE,          // Logging message
	 USER_BREAK,           // User-initiated break
	 EXECUTION_STEP        // Single step execution
 };
 
 /**
  * Debug event information
  */
 struct DebugEvent {
	 DebugEventType type;       // Event type
	 CPUType cpuType;           // CPU that triggered the event
	 uint32_t address;          // Address related to the event
	 uint32_t value;            // Value related to the event
	 std::string message;       // Event message
	 int breakpointId;          // Breakpoint ID if applicable
 };
 
 /**
  * Breakpoint definition
  */
 struct Breakpoint {
	 int id;                    // Unique breakpoint ID
	 BreakpointType type;       // Breakpoint type
	 CPUType cpuType;           // CPU this breakpoint applies to
	 uint32_t address;          // Address for the breakpoint
	 uint32_t addressEnd;       // End address for range (if applicable)
	 uint32_t mask;             // Address mask (if applicable)
	 uint32_t condition;        // Value to compare against (if applicable)
	 bool enabled;              // Whether breakpoint is enabled
	 std::string conditionExpr; // Condition expression (for conditional breakpoints)
	 std::string description;   // User description of the breakpoint
 };
 
 /**
  * Debugger command result
  */
 struct DebugCommandResult {
	 bool success;              // Whether command was successful
	 std::string output;        // Command output text
	 std::vector<uint32_t> data; // Command data result (if applicable)
 };
 
 /**
  * Main debugger class that coordinates all debugging functionality
  */
 class Debugger {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param memoryManager Reference to the memory manager
	  * @param logger Reference to the system logger
	  */
	 Debugger(System& system, MemoryManager& memoryManager, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~Debugger();
	 
	 /**
	  * Initialize the debugger
	  * @return True if initialization was successful
	  */
	 bool Initialize();
	 
	 /**
	  * Reset the debugger state
	  */
	 void Reset();
	 
	 /**
	  * Update debugger state
	  */
	 void Update();
	 
	 /**
	  * Process a debug event
	  * @param event Debug event to process
	  * @return True if execution should break
	  */
	 bool ProcessEvent(const DebugEvent& event);
	 
	 /**
	  * Add a breakpoint
	  * @param type Breakpoint type
	  * @param cpuType CPU type
	  * @param address Address for the breakpoint
	  * @param condition Condition value (if applicable)
	  * @param description User description
	  * @return Breakpoint ID if successful, -1 otherwise
	  */
	 int AddBreakpoint(BreakpointType type, CPUType cpuType, uint32_t address,
					 uint32_t condition = 0, const std::string& description = "");
	 
	 /**
	  * Add a memory range breakpoint
	  * @param type Breakpoint type
	  * @param cpuType CPU type
	  * @param startAddress Start address for the range
	  * @param endAddress End address for the range
	  * @param description User description
	  * @return Breakpoint ID if successful, -1 otherwise
	  */
	 int AddMemoryRangeBreakpoint(BreakpointType type, CPUType cpuType,
								uint32_t startAddress, uint32_t endAddress,
								const std::string& description = "");
	 
	 /**
	  * Add a conditional breakpoint
	  * @param cpuType CPU type
	  * @param address Address for the breakpoint
	  * @param conditionExpr Condition expression
	  * @param description User description
	  * @return Breakpoint ID if successful, -1 otherwise
	  */
	 int AddConditionalBreakpoint(CPUType cpuType, uint32_t address,
								const std::string& conditionExpr,
								const std::string& description = "");
	 
	 /**
	  * Remove a breakpoint
	  * @param id Breakpoint ID
	  * @return True if breakpoint was removed
	  */
	 bool RemoveBreakpoint(int id);
	 
	 /**
	  * Enable or disable a breakpoint
	  * @param id Breakpoint ID
	  * @param enabled True to enable, false to disable
	  * @return True if successful
	  */
	 bool SetBreakpointEnabled(int id, bool enabled);
	 
	 /**
	  * Get all defined breakpoints
	  * @return Vector of breakpoints
	  */
	 std::vector<Breakpoint> GetBreakpoints() const;
	 
	 /**
	  * Add a memory watch
	  * @param address Address to watch
	  * @param size Size of memory to watch (1, 2, or 4 bytes)
	  * @param description User description
	  * @return Watch ID if successful, -1 otherwise
	  */
	 int AddMemoryWatch(uint32_t address, uint8_t size, 
					  const std::string& description = "");
	 
	 /**
	  * Remove a memory watch
	  * @param id Watch ID
	  * @return True if watch was removed
	  */
	 bool RemoveMemoryWatch(int id);
	 
	 /**
	  * Get memory values for all watches
	  * @return Map of watch IDs to current values
	  */
	 std::unordered_map<int, uint32_t> GetWatchValues() const;
	 
	 /**
	  * Execute a debug command
	  * @param command Debug command string
	  * @return Command result
	  */
	 DebugCommandResult ExecuteCommand(const std::string& command);
	 
	 /**
	  * Step execution by one instruction
	  * @param cpuType CPU to step
	  * @return True if successful
	  */
	 bool StepInstruction(CPUType cpuType);
	 
	 /**
	  * Run to specified address
	  * @param cpuType CPU to run
	  * @param address Address to run to
	  * @return True if successful
	  */
	 bool RunToAddress(CPUType cpuType, uint32_t address);
	 
	 /**
	  * Pause emulation
	  * @return True if successful
	  */
	 bool PauseEmulation();
	 
	 /**
	  * Resume emulation
	  * @return True if successful
	  */
	 bool ResumeEmulation();
	 
	 /**
	  * Get CPU debugger for a specific CPU
	  * @param cpuType CPU type
	  * @return Pointer to CPU debugger
	  */
	 CPUDebugger* GetCPUDebugger(CPUType cpuType);
	 
	 /**
	  * Get memory viewer
	  * @return Pointer to memory viewer
	  */
	 MemoryViewer* GetMemoryViewer();
	 
	 /**
	  * Register debug event callback
	  * @param callback Function to call when debug event occurs
	  * @return Callback ID
	  */
	 int RegisterEventCallback(std::function<void(const DebugEvent&)> callback);
	 
	 /**
	  * Remove debug event callback
	  * @param callbackId Callback ID
	  * @return True if successful
	  */
	 bool RemoveEventCallback(int callbackId);
	 
	 /**
	  * Disassemble memory region
	  * @param cpuType CPU type for disassembly
	  * @param startAddress Start address
	  * @param instructionCount Number of instructions to disassemble
	  * @return Disassembly result
	  */
	 DebugCommandResult Disassemble(CPUType cpuType, uint32_t startAddress, 
								  uint32_t instructionCount);
	 
	 /**
	  * Dump memory region
	  * @param startAddress Start address
	  * @param byteCount Number of bytes to dump
	  * @return Memory dump result
	  */
	 DebugCommandResult DumpMemory(uint32_t startAddress, uint32_t byteCount);
	 
	 /**
	  * Set memory values
	  * @param address Start address
	  * @param values Vector of byte values to set
	  * @return True if successful
	  */
	 bool SetMemory(uint32_t address, const std::vector<uint8_t>& values);
	 
	 /**
	  * Get the most recent debug event
	  * @return Most recent debug event
	  */
	 const DebugEvent& GetLastEvent() const;
	 
	 /**
	  * Check if debugger is currently active (paused)
	  * @return True if debugger is active
	  */
	 bool IsActive() const;
	 
	 /**
	  * Set log filtering options
	  * @param component Component to filter (empty for all)
	  * @param level Minimum log level to show
	  */
	 void SetLogFilter(const std::string& component, LogLevel level);
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to memory manager
	 MemoryManager& m_memoryManager;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // Debugging active flag
	 bool m_active;
	 
	 // Breakpoints
	 std::vector<Breakpoint> m_breakpoints;
	 int m_nextBreakpointId;
	 
	 // Memory watches
	 struct MemoryWatch {
		 int id;
		 uint32_t address;
		 uint8_t size;
		 std::string description;
		 uint32_t lastValue;
	 };
	 std::vector<MemoryWatch> m_watches;
	 int m_nextWatchId;
	 
	 // CPU debuggers
	 std::unique_ptr<CPUDebugger> m_mainCpuDebugger;
	 std::unique_ptr<CPUDebugger> m_audioCpuDebugger;
	 
	 // Memory viewer
	 std::unique_ptr<MemoryViewer> m_memoryViewer;
	 
	 // Event callbacks
	 std::unordered_map<int, std::function<void(const DebugEvent&)>> m_eventCallbacks;
	 int m_nextCallbackId;
	 
	 // Most recent debug event
	 DebugEvent m_lastEvent;
	 
	 // Temporary execution breakpoints
	 std::unordered_set<uint32_t> m_tempBreakpoints;
	 
	 /**
	  * Check if a breakpoint should trigger
	  * @param breakpoint Breakpoint to check
	  * @param address Address being accessed
	  * @param value Value being accessed
	  * @param cpuType CPU performing the access
	  * @return True if breakpoint should trigger
	  */
	 bool ShouldBreakpointTrigger(const Breakpoint& breakpoint, uint32_t address,
								uint32_t value, CPUType cpuType);
	 
	 /**
	  * Evaluate a condition expression
	  * @param expression Condition expression
	  * @param cpuType CPU context for evaluation
	  * @return True if condition is met
	  */
	 bool EvaluateCondition(const std::string& expression, CPUType cpuType);
	 
	 /**
	  * Check for changes in watched memory
	  */
	 void UpdateWatches();
	 
	 /**
	  * Parse a debug command
	  * @param command Command string
	  * @param args Output vector for command arguments
	  * @return Command name
	  */
	 std::string ParseCommand(const std::string& command, std::vector<std::string>& args);
	 
	 /**
	  * Trigger a debug event
	  * @param event Event to trigger
	  */
	 void TriggerEvent(const DebugEvent& event);
	 
	 /**
	  * Find breakpoint by ID
	  * @param id Breakpoint ID
	  * @return Index in breakpoints vector, or -1 if not found
	  */
	 int FindBreakpointIndex(int id) const;
	 
	 /**
	  * Find watch by ID
	  * @param id Watch ID
	  * @return Index in watches vector, or -1 if not found
	  */
	 int FindWatchIndex(int id) const;
 };
 
 } // namespace NiXX32