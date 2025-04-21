/**
 * Z80CPU.cpp
 * Implementation of the Zilog Z80 CPU emulation for NiXX-32 arcade board audio subsystem
 */

 #include "Z80CPU.h"
 #include "NiXX32System.h"
 #include "MemoryManager.h"
 #include "Logger.h"
 #include "AudioSystem.h"
 #include <iostream>
 #include <sstream>
 #include <iomanip>
 #include <algorithm>
 #include <unordered_map>
 #include <cmath>
 
 namespace NiXX32 {
 
 // Instruction execution time table (in T-states)
 // This is a simplified table; actual Z80 has more complex timing
 static const std::unordered_map<uint8_t, int> s_instructionCycles = {
	 {0x00, 4},  // NOP
	 {0x01, 10}, // LD BC,nn
	 {0x02, 7},  // LD (BC),A
	 {0x03, 6},  // INC BC
	 {0x04, 4},  // INC B
	 {0x05, 4},  // DEC B
	 {0x06, 7},  // LD B,n
	 {0x07, 4},  // RLCA
	 {0x08, 4},  // EX AF,AF'
	 {0x09, 11}, // ADD HL,BC
	 {0x0A, 7},  // LD A,(BC)
	 {0x0B, 6},  // DEC BC
	 {0x0C, 4},  // INC C
	 {0x0D, 4},  // DEC C
	 {0x0E, 7},  // LD C,n
	 {0x0F, 4},  // RRCA
	 {0xC3, 10}, // JP nn
	 {0xC9, 10}, // RET
	 {0xCD, 17}, // CALL nn
	 // Default cycles for other instructions
	 {0xFF, 4}   // Default for instructions not in the table
 };
 
 // Extended table for CB-prefixed instructions
 static const std::unordered_map<uint8_t, int> s_cbPrefixCycles = {
	 {0x00, 8},  // RLC B
	 {0x01, 8},  // RLC C
	 // Default cycles for other CB instructions
	 {0xFF, 8}   // Default for CB instructions not in the table
 };
 
 // Extended table for ED-prefixed instructions
 static const std::unordered_map<uint8_t, int> s_edPrefixCycles = {
	 {0x40, 12}, // IN B,(C)
	 {0x41, 12}, // OUT (C),B
	 {0x42, 15}, // SBC HL,BC
	 {0x43, 20}, // LD (nn),BC
	 {0x44, 8},  // NEG
	 {0x45, 8},  // RETN
	 {0x46, 8},  // IM 0
	 {0x47, 9},  // LD I,A
	 // Default cycles for other ED instructions
	 {0xFF, 12}  // Default for ED instructions not in the table
 };
 
 Z80CPU::Z80CPU(System& system, MemoryManager& memoryManager, Logger& logger, AudioSystem& audioSystem)
	 : m_system(system),
	   m_memoryManager(memoryManager),
	   m_logger(logger),
	   m_audioSystem(audioSystem),
	   m_state(Z80State::RESET),
	   m_interruptMode(InterruptMode::IM0),
	   m_iff1(false),
	   m_iff2(false),
	   m_pendingInterrupt(Z80InterruptType::NONE),
	   m_interruptData(0),
	   m_clockSpeed(0),
	   m_cycleCount(0),
	   m_pendingCycles(0),
	   m_nextCallbackId(1) {
	 
	 // Initialize registers
	 Reset();
 }
 
 Z80CPU::~Z80CPU() {
	 m_logger.Info("Z80CPU", "CPU shutdown");
 }
 
 bool Z80CPU::Initialize(uint32_t clockSpeed) {
	 m_logger.Info("Z80CPU", "Initializing Z80 CPU with clock speed: " + std::to_string(clockSpeed) + " Hz");
	 m_clockSpeed = clockSpeed;
	 m_state = Z80State::RUNNING;
	 return true;
 }
 
 void Z80CPU::Reset() {
	 m_logger.Info("Z80CPU", "Resetting Z80 CPU");
	 
	 // Clear all registers
	 memset(&m_registers, 0, sizeof(m_registers));
	 
	 // Set program counter to 0
	 m_registers.pc = 0;
	 
	 // Reset interrupt mode
	 m_interruptMode = InterruptMode::IM0;
	 m_iff1 = false;
	 m_iff2 = false;
	 m_pendingInterrupt = Z80InterruptType::NONE;
	 
	 // Reset cycle count
	 m_cycleCount = 0;
	 m_pendingCycles = 0;
	 
	 // Set running state
	 m_state = Z80State::RUNNING;
 }
 
 int Z80CPU::Execute(int cycles) {
	 if (m_state != Z80State::RUNNING && m_state != Z80State::INTERRUPT) {
		 return 0;
	 }
	 
	 int executedCycles = 0;
	 m_pendingCycles = cycles;
	 
	 // Check for pending interrupts first
	 if (m_state == Z80State::RUNNING && CheckPendingInterrupts()) {
		 executedCycles += 11; // Typical interrupt overhead
		 m_pendingCycles -= 11;
	 }
	 
	 // Execute instructions until we've used up the requested cycles
	 while (m_pendingCycles > 0 && (m_state == Z80State::RUNNING || m_state == Z80State::INTERRUPT)) {
		 int instructionCycles = ExecuteInstruction();
		 executedCycles += instructionCycles;
		 m_pendingCycles -= instructionCycles;
		 
		 // Check for hooks at the current PC
		 auto hookIt = m_executionHooks.find(m_registers.pc);
		 if (hookIt != m_executionHooks.end()) {
			 hookIt->second();
		 }
	 }
	 
	 // Update total cycle count
	 m_cycleCount += executedCycles;
	 return executedCycles;
 }
 
 int Z80CPU::ExecuteInstruction() {
	 if (m_state == Z80State::HALTED) {
		 // If CPU is halted, just burn cycles without executing instructions
		 return 4; // Typical NOP cycle count
	 }
	 
	 // Fetch the next instruction
	 uint8_t opcode = ReadByte(m_registers.pc++);
	 
	 // Find base opcode cycles
	 int cycles = s_instructionCycles.count(opcode) ? 
				 s_instructionCycles.at(opcode) : 
				 s_instructionCycles.at(0xFF); // Default cycles
	 
	 // Execute instruction based on opcode
	 switch (opcode) {
		 case 0x00: // NOP
			 // Do nothing
			 break;
			 
		 case 0x01: // LD BC,nn
			 {
				 uint16_t value = FetchWord();
				 m_registers.bc = value;
			 }
			 break;
			 
		 case 0x02: // LD (BC),A
			 WriteByte(m_registers.bc, m_registers.a);
			 break;
			 
		 case 0x03: // INC BC
			 m_registers.bc++;
			 break;
			 
		 case 0x04: // INC B
			 {
				 uint8_t result = m_registers.b + 1;
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_N | FLAG_Z | FLAG_H | FLAG_P);
				 if (result == 0) m_registers.f |= FLAG_Z;
				 if (result & 0x80) m_registers.f |= FLAG_S;
				 if ((m_registers.b & 0x0F) == 0x0F) m_registers.f |= FLAG_H;
				 if (result == 0x80) m_registers.f |= FLAG_P;
				 
				 m_registers.b = result;
			 }
			 break;
			 
		 case 0x05: // DEC B
			 {
				 uint8_t result = m_registers.b - 1;
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_Z | FLAG_H | FLAG_P);
				 m_registers.f |= FLAG_N;
				 if (result == 0) m_registers.f |= FLAG_Z;
				 if (result & 0x80) m_registers.f |= FLAG_S;
				 if ((m_registers.b & 0x0F) == 0x00) m_registers.f |= FLAG_H;
				 if (result == 0x7F) m_registers.f |= FLAG_P;
				 
				 m_registers.b = result;
			 }
			 break;
			 
		 case 0x06: // LD B,n
			 m_registers.b = FetchByte();
			 break;
			 
		 case 0x07: // RLCA
			 {
				 bool carry = (m_registers.a & 0x80) != 0;
				 m_registers.a = (m_registers.a << 1) | (carry ? 1 : 0);
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_N | FLAG_H | FLAG_C);
				 if (carry) m_registers.f |= FLAG_C;
			 }
			 break;
			 
		 case 0x08: // EX AF,AF'
			 {
				 std::swap(m_registers.af, m_registers.af_);
			 }
			 break;
			 
		 case 0x09: // ADD HL,BC
			 {
				 uint32_t result = m_registers.hl + m_registers.bc;
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_N | FLAG_C | FLAG_H);
				 if (result > 0xFFFF) m_registers.f |= FLAG_C;
				 if ((m_registers.hl & 0x0FFF) + (m_registers.bc & 0x0FFF) > 0x0FFF) m_registers.f |= FLAG_H;
				 
				 m_registers.hl = result & 0xFFFF;
			 }
			 break;
			 
		 case 0x0A: // LD A,(BC)
			 m_registers.a = ReadByte(m_registers.bc);
			 break;
			 
		 case 0x0B: // DEC BC
			 m_registers.bc--;
			 break;
			 
		 case 0x0C: // INC C
			 {
				 uint8_t result = m_registers.c + 1;
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_N | FLAG_Z | FLAG_H | FLAG_P);
				 if (result == 0) m_registers.f |= FLAG_Z;
				 if (result & 0x80) m_registers.f |= FLAG_S;
				 if ((m_registers.c & 0x0F) == 0x0F) m_registers.f |= FLAG_H;
				 if (result == 0x80) m_registers.f |= FLAG_P;
				 
				 m_registers.c = result;
			 }
			 break;
			 
		 case 0x0D: // DEC C
			 {
				 uint8_t result = m_registers.c - 1;
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_Z | FLAG_H | FLAG_P);
				 m_registers.f |= FLAG_N;
				 if (result == 0) m_registers.f |= FLAG_Z;
				 if (result & 0x80) m_registers.f |= FLAG_S;
				 if ((m_registers.c & 0x0F) == 0x00) m_registers.f |= FLAG_H;
				 if (result == 0x7F) m_registers.f |= FLAG_P;
				 
				 m_registers.c = result;
			 }
			 break;
			 
		 case 0x0E: // LD C,n
			 m_registers.c = FetchByte();
			 break;
			 
		 case 0x0F: // RRCA
			 {
				 bool carry = (m_registers.a & 0x01) != 0;
				 m_registers.a = (m_registers.a >> 1) | (carry ? 0x80 : 0);
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_N | FLAG_H | FLAG_C);
				 if (carry) m_registers.f |= FLAG_C;
			 }
			 break;
			 
		 case 0xC3: // JP nn
			 {
				 uint16_t address = FetchWord();
				 m_registers.pc = address;
			 }
			 break;
			 
		 case 0xC9: // RET
			 m_registers.pc = Pop();
			 break;
			 
		 case 0xCD: // CALL nn
			 {
				 uint16_t address = FetchWord();
				 Push(m_registers.pc);
				 m_registers.pc = address;
			 }
			 break;
			 
		 case 0xCB: // CB prefix
			 cycles = ExecuteCBPrefixedInstruction(FetchByte());
			 break;
			 
		 case 0xDD: // DD prefix (IX instructions)
			 cycles = ExecuteDDPrefixedInstruction(FetchByte());
			 break;
			 
		 case 0xED: // ED prefix
			 cycles = ExecuteEDPrefixedInstruction(FetchByte());
			 break;
			 
		 case 0xFD: // FD prefix (IY instructions)
			 cycles = ExecuteFDPrefixedInstruction(FetchByte());
			 break;
			 
		 default:
			 // Default implementation for other opcodes
			 m_logger.Warning("Z80CPU", "Unimplemented opcode: 0x" + 
							 std::to_string(opcode));
			 break;
	 }
	 
	 return cycles;
 }
 
 bool Z80CPU::TriggerInterrupt(Z80InterruptType type, uint8_t data) {
	 // Store the interrupt to be processed on the next instruction
	 m_pendingInterrupt = type;
	 m_interruptData = data;
	 
	 // For debugging
	 m_logger.Debug("Z80CPU", "Interrupt triggered: Type=" + std::to_string(static_cast<int>(type)) + 
				   ", Data=0x" + std::to_string(data));
	 
	 // Return true if the interrupt can be accepted (IFF1 enabled)
	 return m_iff1 || type == Z80InterruptType::NMI;
 }
 
 Z80State Z80CPU::GetState() const {
	 return m_state;
 }
 
 InterruptMode Z80CPU::GetInterruptMode() const {
	 return m_interruptMode;
 }
 
 bool Z80CPU::AreInterruptsEnabled() const {
	 return m_iff1;
 }
 
 uint16_t Z80CPU::GetPC() const {
	 return m_registers.pc;
 }
 
 void Z80CPU::SetPC(uint16_t newPC) {
	 m_registers.pc = newPC;
 }
 
 Z80Registers Z80CPU::GetRegisters() const {
	 return m_registers;
 }
 
 void Z80CPU::SetRegisters(const Z80Registers& registers) {
	 m_registers = registers;
 }
 
 bool Z80CPU::IsFlagSet(Z80FlagBits flag) const {
	 return (m_registers.f & flag) != 0;
 }
 
 void Z80CPU::SetFlag(Z80FlagBits flag, bool value) {
	 if (value) {
		 m_registers.f |= flag;
	 } else {
		 m_registers.f &= ~flag;
	 }
 }
 
 uint64_t Z80CPU::GetCycleCount() const {
	 return m_cycleCount;
 }
 
 uint32_t Z80CPU::GetClockSpeed() const {
	 return m_clockSpeed;
 }
 
 std::string Z80CPU::DisassembleInstruction(uint16_t address, int& instructionSize) {
	 // Default instruction size is 1 byte
	 instructionSize = 1;
	 
	 // Read the opcode
	 uint8_t opcode = ReadByte(address);
	 
	 // Create a stringstream for the disassembly
	 std::stringstream ss;
	 
	 // Disassemble based on opcode
	 switch (opcode) {
		 case 0x00:
			 ss << "NOP";
			 break;
			 
		 case 0x01:
			 {
				 uint16_t value = ReadWord(address + 1);
				 ss << "LD BC,$" << std::hex << std::setw(4) << std::setfill('0') << value;
				 instructionSize = 3;
			 }
			 break;
			 
		 case 0x02:
			 ss << "LD (BC),A";
			 break;
			 
		 case 0x03:
			 ss << "INC BC";
			 break;
			 
		 case 0x04:
			 ss << "INC B";
			 break;
			 
		 case 0x05:
			 ss << "DEC B";
			 break;
			 
		 case 0x06:
			 {
				 uint8_t value = ReadByte(address + 1);
				 ss << "LD B,$" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value);
				 instructionSize = 2;
			 }
			 break;
			 
		 case 0x07:
			 ss << "RLCA";
			 break;
			 
		 case 0x08:
			 ss << "EX AF,AF'";
			 break;
			 
		 case 0x09:
			 ss << "ADD HL,BC";
			 break;
			 
		 case 0x0A:
			 ss << "LD A,(BC)";
			 break;
			 
		 case 0x0B:
			 ss << "DEC BC";
			 break;
			 
		 case 0x0C:
			 ss << "INC C";
			 break;
			 
		 case 0x0D:
			 ss << "DEC C";
			 break;
			 
		 case 0x0E:
			 {
				 uint8_t value = ReadByte(address + 1);
				 ss << "LD C,$" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value);
				 instructionSize = 2;
			 }
			 break;
			 
		 case 0x0F:
			 ss << "RRCA";
			 break;
			 
		 case 0xC3:
			 {
				 uint16_t value = ReadWord(address + 1);
				 ss << "JP $" << std::hex << std::setw(4) << std::setfill('0') << value;
				 instructionSize = 3;
			 }
			 break;
			 
		 case 0xC9:
			 ss << "RET";
			 break;
			 
		 case 0xCD:
			 {
				 uint16_t value = ReadWord(address + 1);
				 ss << "CALL $" << std::hex << std::setw(4) << std::setfill('0') << value;
				 instructionSize = 3;
			 }
			 break;
			 
		 case 0xCB:
			 {
				 // CB prefix
				 uint8_t cbOpcode = ReadByte(address + 1);
				 // Disassembly for CB-prefixed instructions would go here
				 ss << "CB " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(cbOpcode);
				 instructionSize = 2;
			 }
			 break;
			 
		 case 0xDD:
			 {
				 // DD prefix (IX instructions)
				 uint8_t ddOpcode = ReadByte(address + 1);
				 // Disassembly for DD-prefixed instructions would go here
				 ss << "DD " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ddOpcode);
				 instructionSize = 2;
			 }
			 break;
			 
		 case 0xED:
			 {
				 // ED prefix
				 uint8_t edOpcode = ReadByte(address + 1);
				 // Disassembly for ED-prefixed instructions would go here
				 ss << "ED " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(edOpcode);
				 instructionSize = 2;
			 }
			 break;
			 
		 case 0xFD:
			 {
				 // FD prefix (IY instructions)
				 uint8_t fdOpcode = ReadByte(address + 1);
				 // Disassembly for FD-prefixed instructions would go here
				 ss << "FD " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(fdOpcode);
				 instructionSize = 2;
			 }
			 break;
			 
		 default:
			 ss << "Unknown: " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(opcode);
			 break;
	 }
	 
	 return ss.str();
 }
 
 bool Z80CPU::RegisterPortHooks(uint8_t port, 
							  std::function<uint8_t()> readCallback,
							  std::function<void(uint8_t)> writeCallback) {
	 if (readCallback) {
		 m_portReadHooks[port] = readCallback;
	 }
	 
	 if (writeCallback) {
		 m_portWriteHooks[port] = writeCallback;
	 }
	 
	 m_logger.Debug("Z80CPU", "Port hooks registered for port 0x" + 
				   std::to_string(port));
				   
	 return true;
 }
 
 bool Z80CPU::RemovePortHooks(uint8_t port) {
	 bool removed = false;
	 
	 auto readIt = m_portReadHooks.find(port);
	 if (readIt != m_portReadHooks.end()) {
		 m_portReadHooks.erase(readIt);
		 removed = true;
	 }
	 
	 auto writeIt = m_portWriteHooks.find(port);
	 if (writeIt != m_portWriteHooks.end()) {
		 m_portWriteHooks.erase(writeIt);
		 removed = true;
	 }
	 
	 if (removed) {
		 m_logger.Debug("Z80CPU", "Port hooks removed for port 0x" + 
					   std::to_string(port));
	 }
	 
	 return removed;
 }
 
 bool Z80CPU::RegisterExecutionHook(uint16_t address, std::function<void()> callback) {
	 if (!callback) {
		 return false;
	 }
	 
	 m_executionHooks[address] = callback;
	 
	 m_logger.Debug("Z80CPU", "Execution hook registered at address 0x" + 
				   std::to_string(address));
				   
	 return true;
 }
 
 bool Z80CPU::RemoveExecutionHook(uint16_t address) {
	 auto it = m_executionHooks.find(address);
	 if (it != m_executionHooks.end()) {
		 m_executionHooks.erase(it);
		 
		 m_logger.Debug("Z80CPU", "Execution hook removed from address 0x" + 
					   std::to_string(address));
					   
		 return true;
	 }
	 
	 return false;
 }
 
 uint8_t Z80CPU::ReadByte(uint16_t address) {
	 return m_memoryManager.Read8(address);
 }
 
 void Z80CPU::WriteByte(uint16_t address, uint8_t value) {
	 m_memoryManager.Write8(address, value);
 }
 
 uint16_t Z80CPU::ReadWord(uint16_t address) {
	 // Z80 is little-endian
	 uint8_t low = ReadByte(address);
	 uint8_t high = ReadByte(address + 1);
	 return (high << 8) | low;
 }
 
 void Z80CPU::WriteWord(uint16_t address, uint16_t value) {
	 // Z80 is little-endian
	 WriteByte(address, value & 0xFF);
	 WriteByte(address + 1, (value >> 8) & 0xFF);
 }
 
 uint8_t Z80CPU::InPort(uint8_t port) {
	 // Check if we have a registered callback for this port
	 auto it = m_portReadHooks.find(port);
	 if (it != m_portReadHooks.end()) {
		 return it->second();
	 }
	 
	 // Default implementation for ports without callbacks
	 m_logger.Debug("Z80CPU", "Reading from unhandled port: 0x" + 
				   std::to_string(port));
				   
	 return 0xFF; // Default value for unhandled ports
 }
 
 void Z80CPU::OutPort(uint8_t port, uint8_t value) {
	 // Check if we have a registered callback for this port
	 auto it = m_portWriteHooks.find(port);
	 if (it != m_portWriteHooks.end()) {
		 it->second(value);
		 return;
	 }
	 
	 // Default implementation for ports without callbacks
	 m_logger.Debug("Z80CPU", "Writing to unhandled port: 0x" + 
				   std::to_string(port) + " = 0x" + std::to_string(value));
 }
 
 void Z80CPU::Push(uint16_t value) {
	 m_registers.sp -= 2;
	 WriteWord(m_registers.sp, value);
 }
 
 uint16_t Z80CPU::Pop() {
	 uint16_t value = ReadWord(m_registers.sp);
	 m_registers.sp += 2;
	 return value;
 }
 
 uint8_t Z80CPU::FetchByte() {
	 return ReadByte(m_registers.pc++);
 }
 
 uint16_t Z80CPU::FetchWord() {
	 uint16_t value = ReadWord(m_registers.pc);
	 m_registers.pc += 2;
	 return value;
 }
 
 int Z80CPU::ExecuteCBPrefixedInstruction(uint8_t opcode) {
	 // Find cycles for CB-prefixed instruction
	 int cycles = s_cbPrefixCycles.count(opcode) ? 
				 s_cbPrefixCycles.at(opcode) : 
				 s_cbPrefixCycles.at(0xFF); // Default cycles
	 
	 // Execute CB-prefixed instruction
	 switch (opcode) {
		 case 0x00: // RLC B
			 {
				 bool carry = (m_registers.b & 0x80) != 0;
				 m_registers.b = (m_registers.b << 1) | (carry ? 1 : 0);
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_N | FLAG_H | FLAG_C | FLAG_Z | FLAG_S | FLAG_P);
				 if (carry) m_registers.f |= FLAG_C;
				 if (m_registers.b == 0) m_registers.f |= FLAG_Z;
				 if (m_registers.b & 0x80) m_registers.f |= FLAG_S;
				 if (CalculateParityFlag(m_registers.b)) m_registers.f |= FLAG_P;
			 }
			 break;
			 
		 case 0x01: // RLC C
			 {
				 bool carry = (m_registers.c & 0x80) != 0;
				 m_registers.c = (m_registers.c << 1) | (carry ? 1 : 0);
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_N | FLAG_H | FLAG_C | FLAG_Z | FLAG_S | FLAG_P);
				 if (carry) m_registers.f |= FLAG_C;
				 if (m_registers.c == 0) m_registers.f |= FLAG_Z;
				 if (m_registers.c & 0x80) m_registers.f |= FLAG_S;
				 if (CalculateParityFlag(m_registers.c)) m_registers.f |= FLAG_P;
			 }
			 break;
			 
		 default:
			 // Default implementation for other CB-prefixed instructions
			 m_logger.Warning("Z80CPU", "Unimplemented CB-prefixed opcode: 0x" + 
							 std::to_string(opcode));
			 break;
	 }
	 
	 return cycles;
 }
 
 int Z80CPU::ExecuteDDPrefixedInstruction(uint8_t opcode) {
	 // DD prefix is for IX register instructions
	 // This is a simplified implementation
	 
	 m_logger.Warning("Z80CPU", "Unimplemented DD-prefixed opcode: 0x" + 
					 std::to_string(opcode));
	 
	 // Default cycles for DD-prefixed instructions
	 return 8;
 }
 
 int Z80CPU::ExecuteEDPrefixedInstruction(uint8_t opcode) {
	 // Find cycles for ED-prefixed instruction
	 int cycles = s_edPrefixCycles.count(opcode) ? 
				 s_edPrefixCycles.at(opcode) : 
				 s_edPrefixCycles.at(0xFF); // Default cycles
	 
	 // Execute ED-prefixed instruction
	 switch (opcode) {
		 case 0x40: // IN B,(C)
			 m_registers.b = InPort(m_registers.c);
			 
			 // Update flags
			 m_registers.f &= ~(FLAG_N | FLAG_H | FLAG_Z | FLAG_S | FLAG_P);
			 if (m_registers.b == 0) m_registers.f |= FLAG_Z;
			 if (m_registers.b & 0x80) m_registers.f |= FLAG_S;
			 if (CalculateParityFlag(m_registers.b)) m_registers.f |= FLAG_P;
			 break;
			 
		 case 0x41: // OUT (C),B
			 OutPort(m_registers.c, m_registers.b);
			 break;
			 
		 case 0x42: // SBC HL,BC
			 {
				 uint16_t value = m_registers.bc;
				 bool carry = (m_registers.f & FLAG_C) != 0;
				 uint32_t result = m_registers.hl - value - (carry ? 1 : 0);
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_N | FLAG_Z | FLAG_S | FLAG_H | FLAG_P | FLAG_C);
				 m_registers.f |= FLAG_N;
				 if ((result & 0xFFFF) == 0) m_registers.f |= FLAG_Z;
				 if (result & 0x8000) m_registers.f |= FLAG_S;
				 if ((m_registers.hl & 0x0FFF) < (value & 0x0FFF) + carry) m_registers.f |= FLAG_H;
				 if (((m_registers.hl ^ value) & (m_registers.hl ^ (result & 0xFFFF)) & 0x8000) != 0) m_registers.f |= FLAG_P;
				 if (result > 0xFFFF) m_registers.f |= FLAG_C;
				 
				 m_registers.hl = result & 0xFFFF;
			 }
			 break;
			 
		 case 0x43: // LD (nn),BC
			 {
				 uint16_t address = FetchWord();
				 WriteWord(address, m_registers.bc);
			 }
			 break;
			 
		 case 0x44: // NEG
			 {
				 uint8_t value = m_registers.a;
				 m_registers.a = 0 - value;
				 
				 // Update flags
				 m_registers.f &= ~(FLAG_N | FLAG_Z | FLAG_S | FLAG_H | FLAG_P | FLAG_C);
				 m_registers.f |= FLAG_N;
				 if (m_registers.a == 0) m_registers.f |= FLAG_Z;
				 if (m_registers.a & 0x80) m_registers.f |= FLAG_S;
				 if ((value & 0x0F) != 0) m_registers.f |= FLAG_H;
				 if (value == 0x80) m_registers.f |= FLAG_P;
				 if (value != 0) m_registers.f |= FLAG_C;
			 }
			 break;
			 
		 case 0x45: // RETN
			 m_registers.pc = Pop();
			 m_iff1 = m_iff2;
			 break;
			 
		 case 0x46: // IM 0
			 m_interruptMode = InterruptMode::IM0;
			 break;
			 
		 case 0x47: // LD I,A
			 m_registers.i = m_registers.a;
			 break;
			 
		 default:
			 // Default implementation for other ED-prefixed instructions
			 m_logger.Warning("Z80CPU", "Unimplemented ED-prefixed opcode: 0x" + 
							 std::to_string(opcode));
			 break;
	 }
	 
	 return cycles;
 }
 
 int Z80CPU::ExecuteFDPrefixedInstruction(uint8_t opcode) {
	 // FD prefix is for IY register instructions
	 // This is a simplified implementation
	 
	 m_logger.Warning("Z80CPU", "Unimplemented FD-prefixed opcode: 0x" + 
					 std::to_string(opcode));
	 
	 // Default cycles for FD-prefixed instructions
	 return 8;
 }
 
 int Z80CPU::ExecuteDDCBPrefixedInstruction(uint8_t displacement, uint8_t opcode) {
	 // DDCB prefix is for IX bit instructions with displacement
	 // This is a simplified implementation
	 
	 m_logger.Warning("Z80CPU", "Unimplemented DDCB-prefixed opcode: 0x" + 
					 std::to_string(opcode) + " with displacement 0x" + 
					 std::to_string(displacement));
	 
	 // Default cycles for DDCB-prefixed instructions
	 return 20;
 }
 
 int Z80CPU::ExecuteFDCBPrefixedInstruction(uint8_t displacement, uint8_t opcode) {
	 // FDCB prefix is for IY bit instructions with displacement
	 // This is a simplified implementation
	 
	 m_logger.Warning("Z80CPU", "Unimplemented FDCB-prefixed opcode: 0x" + 
					 std::to_string(opcode) + " with displacement 0x" + 
					 std::to_string(displacement));
	 
	 // Default cycles for FDCB-prefixed instructions
	 return 20;
 }
 
 uint8_t Z80CPU::CalculateParityFlag(uint8_t value) {
	 // Count number of 1 bits
	 int count = 0;
	 for (int i = 0; i < 8; i++) {
		 if (value & (1 << i)) count++;
	 }
	 
	 // Return true if parity is even (P=1 for even parity)
	 return (count % 2) == 0;
 }
 
 uint8_t Z80CPU::CalculateSignZeroFlags(uint8_t value) {
	 uint8_t flags = 0;
	 
	 // Set Z flag if result is zero
	 if (value == 0) flags |= FLAG_Z;
	 
	 // Set S flag if result is negative (bit 7 is set)
	 if (value & 0x80) flags |= FLAG_S;
	 
	 return flags;
 }
 
 void Z80CPU::UpdateFlagsAfterLogicalOp(uint8_t result) {
	 // Update flags after logical operations (AND, OR, XOR)
	 m_registers.f &= ~(FLAG_S | FLAG_Z | FLAG_H | FLAG_P | FLAG_N | FLAG_C);
	 
	 // Set H flag for AND, reset for OR/XOR
	 if (m_registers.f & FLAG_H) m_registers.f |= FLAG_H;
	 
	 // Set S flag if result is negative
	 if (result & 0x80) m_registers.f |= FLAG_S;
	 
	 // Set Z flag if result is zero
	 if (result == 0) m_registers.f |= FLAG_Z;
	 
	 // Set P flag based on parity
	 if (CalculateParityFlag(result)) m_registers.f |= FLAG_P;
 }
 
 void Z80CPU::UpdateFlagsAfterArithmeticOp(uint8_t result, uint8_t operand1, uint8_t operand2, bool isAdd) {
	 // Update flags after arithmetic operations (ADD, SUB, etc.)
	 m_registers.f &= ~(FLAG_S | FLAG_Z | FLAG_H | FLAG_P | FLAG_N | FLAG_C);
	 
	 // Set N flag for subtraction
	 if (!isAdd) m_registers.f |= FLAG_N;
	 
	 // Set S flag if result is negative
	 if (result & 0x80) m_registers.f |= FLAG_S;
	 
	 // Set Z flag if result is zero
	 if (result == 0) m_registers.f |= FLAG_Z;
	 
	 // Set H flag (half-carry)
	 if (isAdd) {
		 if (((operand1 & 0x0F) + (operand2 & 0x0F)) & 0x10) m_registers.f |= FLAG_H;
	 } else {
		 if (((operand1 & 0x0F) - (operand2 & 0x0F)) & 0x10) m_registers.f |= FLAG_H;
	 }
	 
	 // Set P flag for overflow
	 if (isAdd) {
		 if (((operand1 & 0x80) == (operand2 & 0x80)) && ((result & 0x80) != (operand1 & 0x80)))
			 m_registers.f |= FLAG_P;
	 } else {
		 if (((operand1 & 0x80) != (operand2 & 0x80)) && ((result & 0x80) != (operand1 & 0x80)))
			 m_registers.f |= FLAG_P;
	 }
	 
	 // Set C flag for carry
	 if (isAdd) {
		 if ((static_cast<uint16_t>(operand1) + static_cast<uint16_t>(operand2)) > 0xFF)
			 m_registers.f |= FLAG_C;
	 } else {
		 if (operand2 > operand1)
			 m_registers.f |= FLAG_C;
	 }
 }
 
 bool Z80CPU::CheckPendingInterrupts() {
	 // No interrupts if they're disabled (except NMI)
	 if (!m_iff1 && m_pendingInterrupt != Z80InterruptType::NMI) {
		 return false;
	 }
	 
	 // Process pending interrupt
	 if (m_pendingInterrupt != Z80InterruptType::NONE) {
		 if (m_pendingInterrupt == Z80InterruptType::NMI) {
			 // Non-maskable interrupt
			 HandleNMI();
			 m_pendingInterrupt = Z80InterruptType::NONE;
			 return true;
		 } else {
			 // Maskable interrupt
			 HandleInterrupt();
			 m_pendingInterrupt = Z80InterruptType::NONE;
			 return true;
		 }
	 }
	 
	 return false;
 }
 
 void Z80CPU::HandleNMI() {
	 // Save the current IFF1 state in IFF2
	 m_iff2 = m_iff1;
	 
	 // Disable interrupts
	 m_iff1 = false;
	 
	 // Push current PC onto stack
	 Push(m_registers.pc);
	 
	 // Jump to NMI handler address
	 m_registers.pc = 0x0066;
	 
	 // Set state to INTERRUPT
	 m_state = Z80State::INTERRUPT;
	 
	 m_logger.Debug("Z80CPU", "NMI handled, jumping to 0x0066");
 }
 
 void Z80CPU::HandleInterrupt() {
	 // Disable interrupts
	 m_iff1 = false;
	 m_iff2 = false;
	 
	 // Handle interrupt based on current mode
	 switch (m_interruptMode) {
		 case InterruptMode::IM0:
			 // In IM0, the interrupting device supplies an instruction
			 // Usually an RST instruction
			 // For simplicity, we'll treat it as RST 38h
			 Push(m_registers.pc);
			 m_registers.pc = 0x0038;
			 break;
			 
		 case InterruptMode::IM1:
			 // In IM1, CPU always executes RST 38h
			 Push(m_registers.pc);
			 m_registers.pc = 0x0038;
			 break;
			 
		 case InterruptMode::IM2:
			 // In IM2, jump to address in I register (high byte) and byte from device (low byte)
			 {
				 uint16_t vectorAddress = (m_registers.i << 8) | m_interruptData;
				 uint16_t jumpAddress = ReadWord(vectorAddress);
				 Push(m_registers.pc);
				 m_registers.pc = jumpAddress;
			 }
			 break;
	 }
	 
	 // Set state to INTERRUPT
	 m_state = Z80State::INTERRUPT;
	 
	 m_logger.Debug("Z80CPU", "Interrupt handled in mode " + 
				   std::to_string(static_cast<int>(m_interruptMode)));
 }
 
 } // namespace NiXX32