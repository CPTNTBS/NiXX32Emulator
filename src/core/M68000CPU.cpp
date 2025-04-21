/**
 * M68000CPU.cpp
 * Implementation of the Motorola 68000 CPU emulation for NiXX-32 arcade board
 */

 #include "M68000CPU.h"
 #include "NiXX32System.h"
 #include "MemoryManager.h"
 #include "Logger.h"
 #include <iostream>
 #include <sstream>
 #include <iomanip>
 #include <algorithm>
 #include <unordered_map>
 #include <cmath>
 
 namespace NiXX32 {
 
 // Instruction execution time table (in cycles)
 // This is a simplified table; actual 68000 has more complex timing
 static const std::unordered_map<uint16_t, int> s_instructionCycles = {
	 {0x003C, 8},  // ORI to CCR
	 {0x007C, 12}, // ORI to SR
	 {0x023C, 8},  // ANDI to CCR
	 {0x027C, 12}, // ANDI to SR
	 {0x0A3C, 8},  // EORI to CCR
	 {0x0A7C, 12}, // EORI to SR
	 {0x4AFC, 4},  // ILLEGAL
	 {0x4E70, 4},  // RESET
	 {0x4E71, 4},  // NOP
	 {0x4E72, 4},  // STOP
	 {0x4E73, 4},  // RTE
	 {0x4E75, 16}, // RTS
	 {0x4E76, 4},  // TRAPV
	 {0x4E77, 12}, // RTR
	 {0x4E4E, 4},  // TRAP
	 // Default cycles for other instructions
	 {0xFFFF, 4}   // Default for instructions not in the table
 };
 
 M68000CPU::M68000CPU(System& system, MemoryManager& memoryManager, Logger& logger)
	 : m_system(system),
	   m_memoryManager(memoryManager),
	   m_logger(logger),
	   m_state(CPUState::RESET),
	   m_interruptLevel(IPL_NONE),
	   m_clockSpeed(0),
	   m_cycleCount(0),
	   m_pendingCycles(0),
	   m_nextHookId(1) {
	 
	 // Initialize registers
	 Reset();
 }
 
 M68000CPU::~M68000CPU() {
	 m_logger.Info("M68000CPU", "CPU shutdown");
 }
 
 bool M68000CPU::Initialize(uint32_t clockSpeed) {
	 m_logger.Info("M68000CPU", "Initializing 68000 CPU with clock speed: " + std::to_string(clockSpeed) + " Hz");
	 m_clockSpeed = clockSpeed;
	 m_state = CPUState::RUNNING;
	 return true;
 }
 
 void M68000CPU::Reset() {
	 m_logger.Info("M68000CPU", "Resetting 68000 CPU");
	 
	 // Clear all registers
	 memset(&m_registers, 0, sizeof(m_registers));
	 
	 // Set supervisor mode
	 m_registers.sr = SR_S;
	 
	 // Reset cycle count
	 m_cycleCount = 0;
	 m_pendingCycles = 0;
	 
	 // Set interrupt level to none
	 m_interruptLevel = IPL_NONE;
	 
	 // Set reset state
	 m_state = CPUState::RESET;
	 
	 // Read the reset vector from memory
	 uint32_t resetSSP = m_memoryManager.Read32(0x000000);
	 uint32_t resetPC = m_memoryManager.Read32(0x000004);
	 
	 m_logger.Debug("M68000CPU", "Reset vector: SSP=" + std::to_string(resetSSP) + ", PC=" + std::to_string(resetPC));
	 
	 // Set initial SSP and PC
	 m_registers.a[7] = resetSSP;  // A7 is the SSP in supervisor mode
	 m_registers.ssp = resetSSP;
	 m_registers.pc = resetPC;
	 
	 // Fill prefetch queue
	 RefillPrefetchQueue();
	 
	 // Set running state
	 m_state = CPUState::RUNNING;
 }
 
 int M68000CPU::Execute(int cycles) {
	 if (m_state != CPUState::RUNNING) {
		 return 0;
	 }
	 
	 int executedCycles = 0;
	 m_pendingCycles = cycles;
	 
	 // Check for pending interrupts first
	 if (CheckPendingInterrupts()) {
		 executedCycles += 44; // Typical interrupt overhead
		 m_pendingCycles -= 44;
	 }
	 
	 // Execute instructions until we've used up the requested cycles
	 while (m_pendingCycles > 0 && m_state == CPUState::RUNNING) {
		 int instructionCycles = ExecuteInstruction();
		 executedCycles += instructionCycles;
		 m_pendingCycles -= instructionCycles;
		 
		 // Check for hooks at the current PC
		 auto hookIt = m_hooks.find(m_registers.pc);
		 if (hookIt != m_hooks.end()) {
			 hookIt->second();
		 }
	 }
	 
	 // Update total cycle count
	 m_cycleCount += executedCycles;
	 return executedCycles;
 }
 
 int M68000CPU::ExecuteInstruction() {
	 // Fetch the next instruction from the prefetch queue
	 uint16_t opcode = FetchNextInstruction();
	 
	 // Find base opcode cycles
	 int cycles = s_instructionCycles.count(opcode) ? 
				 s_instructionCycles.at(opcode) : 
				 s_instructionCycles.at(0xFFFF); // Default cycles
	 
	 // Decode and execute the instruction based on the upper 4 bits
	 switch ((opcode >> 12) & 0xF) {
		 case 0x0: ExecuteGroup0(opcode); break; // Bit manipulation, MOVEP, immediate
		 case 0x1: // MOVE.B
		 case 0x2: // MOVE.L
		 case 0x3: ExecuteGroup3(opcode); break; // MOVE.W
		 case 0x4: ExecuteGroup4(opcode); break; // Miscellaneous
		 case 0x5: ExecuteGroup5(opcode); break; // ADDQ, SUBQ, Scc, DBcc
		 case 0x6: ExecuteGroup6(opcode); break; // Bcc, BSR, BRA
		 case 0x7: ExecuteGroup7(opcode); break; // MOVEQ
		 case 0x8: ExecuteGroup8(opcode); break; // OR, DIV, SBCD
		 case 0x9: ExecuteGroup9(opcode); break; // SUB, SUBX
		 case 0xA: ExecuteGroupA(opcode); break; // (Unassigned in 68000)
		 case 0xB: ExecuteGroupB(opcode); break; // CMP, CMPM, EOR
		 case 0xC: ExecuteGroupC(opcode); break; // AND, MUL, ABCD, EXG
		 case 0xD: ExecuteGroupD(opcode); break; // ADD, ADDX
		 case 0xE: ExecuteGroupE(opcode); break; // Shift/rotate, bit operations
		 case 0xF: ExecuteGroupF(opcode); break; // FLine
		 default:
			 // Should never happen due to 4-bit mask
			 m_logger.Error("M68000CPU", "Invalid opcode group: " + std::to_string(opcode));
			 TriggerException(ExceptionType::ILLEGAL_INSTRUCTION);
			 break;
	 }
	 
	 // Check for pending interrupts after each instruction
	 CheckPendingInterrupts();
	 
	 return cycles;
 }
 
 void M68000CPU::TriggerException(ExceptionType type, int vector) {
	 m_logger.Debug("M68000CPU", "Triggering exception: " + std::to_string(static_cast<int>(type)) + 
				   ", vector: " + std::to_string(vector));
	 
	 // Save current state
	 uint32_t pc = m_registers.pc;
	 uint16_t sr = m_registers.sr;
	 
	 // Switch to supervisor mode if not already
	 if (!(sr & SR_S)) {
		 // Save user stack pointer
		 m_registers.usp = m_registers.a[7];
		 // Load supervisor stack pointer
		 m_registers.a[7] = m_registers.ssp;
		 // Set supervisor bit
		 sr |= SR_S;
	 }
	 
	 // Push PC and SR to stack
	 PushLong(pc);
	 PushWord(sr);
	 
	 // Update status register
	 m_registers.sr = sr;
	 
	 // Determine vector number
	 int vectorNum = 0;
	 switch (type) {
		 case ExceptionType::RESET:
			 vectorNum = 0;
			 break;
		 case ExceptionType::BUS_ERROR:
			 vectorNum = 2;
			 break;
		 case ExceptionType::ADDRESS_ERROR:
			 vectorNum = 3;
			 break;
		 case ExceptionType::ILLEGAL_INSTRUCTION:
			 vectorNum = 4;
			 break;
		 case ExceptionType::ZERO_DIVIDE:
			 vectorNum = 5;
			 break;
		 case ExceptionType::CHK_INSTRUCTION:
			 vectorNum = 6;
			 break;
		 case ExceptionType::TRAPV_INSTRUCTION:
			 vectorNum = 7;
			 break;
		 case ExceptionType::PRIVILEGE_VIOLATION:
			 vectorNum = 8;
			 break;
		 case ExceptionType::TRACE:
			 vectorNum = 9;
			 break;
		 case ExceptionType::LINE_A_EMULATOR:
			 vectorNum = 10;
			 break;
		 case ExceptionType::LINE_F_EMULATOR:
			 vectorNum = 11;
			 break;
		 case ExceptionType::INTERRUPT:
			 // For interrupts, vector is passed as parameter
			 vectorNum = vector;
			 break;
		 case ExceptionType::TRAP:
			 // For TRAP instructions, vector is 32 + trap number
			 vectorNum = 32 + vector;
			 break;
		 default:
			 vectorNum = 15; // Uninitialized interrupt
			 break;
	 }
	 
	 // Load exception vector
	 uint32_t vectorAddress = vectorNum * 4;
	 uint32_t newPC = m_memoryManager.Read32(vectorAddress);
	 
	 m_logger.Debug("M68000CPU", "Exception vector address: 0x" + 
				   std::to_string(vectorAddress) + ", new PC: 0x" + std::to_string(newPC));
	 
	 // Set new PC
	 m_registers.pc = newPC;
	 
	 // Refill prefetch queue
	 RefillPrefetchQueue();
	 
	 // Set CPU state back to running
	 m_state = CPUState::RUNNING;
 }
 
 void M68000CPU::SetInterruptLevel(InterruptLevel level) {
	 m_interruptLevel = level;
 }
 
 CPUState M68000CPU::GetState() const {
	 return m_state;
 }
 
 PrivilegeMode M68000CPU::GetPrivilegeMode() const {
	 return (m_registers.sr & SR_S) ? PrivilegeMode::SUPERVISOR : PrivilegeMode::USER;
 }
 
 uint32_t M68000CPU::GetPC() const {
	 return m_registers.pc;
 }
 
 void M68000CPU::SetPC(uint32_t newPC) {
	 m_registers.pc = newPC;
	 RefillPrefetchQueue();
 }
 
 M68000Registers M68000CPU::GetRegisters() const {
	 return m_registers;
 }
 
 void M68000CPU::SetRegisters(const M68000Registers& registers) {
	 m_registers = registers;
	 RefillPrefetchQueue();
 }
 
 uint32_t M68000CPU::GetDataRegister(int regNum) const {
	 if (regNum < 0 || regNum > 7) {
		 m_logger.Warning("M68000CPU", "Invalid data register number: " + std::to_string(regNum));
		 return 0;
	 }
	 return m_registers.d[regNum];
 }
 
 void M68000CPU::SetDataRegister(int regNum, uint32_t value) {
	 if (regNum < 0 || regNum > 7) {
		 m_logger.Warning("M68000CPU", "Invalid data register number: " + std::to_string(regNum));
		 return;
	 }
	 m_registers.d[regNum] = value;
 }
 
 uint32_t M68000CPU::GetAddressRegister(int regNum) const {
	 if (regNum < 0 || regNum > 7) {
		 m_logger.Warning("M68000CPU", "Invalid address register number: " + std::to_string(regNum));
		 return 0;
	 }
	 return m_registers.a[regNum];
 }
 
 void M68000CPU::SetAddressRegister(int regNum, uint32_t value) {
	 if (regNum < 0 || regNum > 7) {
		 m_logger.Warning("M68000CPU", "Invalid address register number: " + std::to_string(regNum));
		 return;
	 }
	 m_registers.a[regNum] = value;
 }
 
 uint16_t M68000CPU::GetSR() const {
	 return m_registers.sr;
 }
 
 void M68000CPU::SetSR(uint16_t value) {
	 uint16_t oldSR = m_registers.sr;
	 m_registers.sr = value;
	 
	 // Check if privilege mode changed
	 bool wasSuper = (oldSR & SR_S) != 0;
	 bool isSuper = (value & SR_S) != 0;
	 
	 if (wasSuper != isSuper) {
		 // Privilege mode changed, swap stack pointers
		 if (isSuper) {
			 // USER -> SUPERVISOR
			 m_registers.usp = m_registers.a[7];
			 m_registers.a[7] = m_registers.ssp;
		 } else {
			 // SUPERVISOR -> USER
			 m_registers.ssp = m_registers.a[7];
			 m_registers.a[7] = m_registers.usp;
		 }
	 }
 }
 
 bool M68000CPU::IsFlagSet(StatusRegisterBits flag) const {
	 return (m_registers.sr & flag) != 0;
 }
 
 void M68000CPU::SetFlag(StatusRegisterBits flag, bool value) {
	 if (value) {
		 m_registers.sr |= flag;
	 } else {
		 m_registers.sr &= ~flag;
	 }
 }
 
 uint64_t M68000CPU::GetCycleCount() const {
	 return m_cycleCount;
 }
 
 uint32_t M68000CPU::GetClockSpeed() const {
	 return m_clockSpeed;
 }
 
 std::string M68000CPU::DisassembleInstruction(uint32_t address, int& instructionSize) {
	 // Default instruction size is 2 bytes (one word)
	 instructionSize = 2;
	 
	 // Read the opcode
	 uint16_t opcode = m_memoryManager.Read16(address);
	 
	 // Create a stringstream for the disassembly
	 std::stringstream ss;
	 
	 // Disassemble based on opcode pattern
	 switch ((opcode >> 12) & 0xF) {
		 case 0x0: {
			 // Group 0: Bit manipulation, MOVEP, immediate
			 if ((opcode & 0xFF00) == 0x0000) {
				 ss << "ORI.B  #$" << std::hex << std::setw(2) << std::setfill('0') 
					<< m_memoryManager.Read16(address + 2) << ",D" << (opcode & 0x7);
				 instructionSize = 4;
			 } else if ((opcode & 0xFF00) == 0x0200) {
				 ss << "ANDI.B #$" << std::hex << std::setw(2) << std::setfill('0') 
					<< m_memoryManager.Read16(address + 2) << ",D" << (opcode & 0x7);
				 instructionSize = 4;
			 } else if (opcode == 0x003C) {
				 ss << "ORI    #$" << std::hex << std::setw(4) << std::setfill('0') 
					<< m_memoryManager.Read16(address + 2) << ",CCR";
				 instructionSize = 4;
			 } else if (opcode == 0x007C) {
				 ss << "ORI    #$" << std::hex << std::setw(4) << std::setfill('0') 
					<< m_memoryManager.Read16(address + 2) << ",SR";
				 instructionSize = 4;
			 } else {
				 ss << "Unknown Group 0: $" << std::hex << std::setw(4) << std::setfill('0') << opcode;
			 }
			 break;
		 }
		 case 0x1:
		 case 0x2:
		 case 0x3: {
			 // MOVE instructions
			 std::string size;
			 switch ((opcode >> 12) & 0x3) {
				 case 0x1: size = "B"; break;  // MOVE.B
				 case 0x2: size = "L"; break;  // MOVE.L
				 case 0x3: size = "W"; break;  // MOVE.W
			 }
			 
			 // Source mode and register
			 int srcMode = (opcode >> 3) & 0x7;
			 int srcReg = opcode & 0x7;
			 
			 // Destination mode and register
			 int destMode = (opcode >> 6) & 0x7;
			 int destReg = (opcode >> 9) & 0x7;
			 
			 ss << "MOVE." << size << "  ";
			 
			 // Source addressing mode
			 switch (srcMode) {
				 case 0: ss << "D" << srcReg; break;                  // Data register direct
				 case 1: ss << "A" << srcReg; break;                  // Address register direct
				 case 2: ss << "(A" << srcReg << ")"; break;          // Address register indirect
				 case 3: ss << "(A" << srcReg << ")+"; break;         // Address register indirect with post-increment
				 case 4: ss << "-(A" << srcReg << ")"; break;         // Address register indirect with pre-decrement
				 case 5: ss << "($" << std::hex << std::setw(4) << std::setfill('0') 
						  << m_memoryManager.Read16(address + 2) << ",A" << srcReg << ")";
						  instructionSize += 2; break;               // Address register indirect with displacement
				 case 6: ss << "Complex mode"; break;                 // More complex modes
				 case 7:
					 switch (srcReg) {
						 case 0: ss << "($" << std::hex << std::setw(4) << std::setfill('0') 
								  << m_memoryManager.Read16(address + 2) << ").W";
								  instructionSize += 2; break;
						 case 1: ss << "($" << std::hex << std::setw(8) << std::setfill('0') 
								  << m_memoryManager.Read32(address + 2) << ").L";
								  instructionSize += 4; break;
						 case 4: ss << "#$";
								 if (size == "L") {
									 ss << std::hex << std::setw(8) << std::setfill('0') 
										<< m_memoryManager.Read32(address + 2);
									 instructionSize += 4;
								 } else {
									 ss << std::hex << std::setw(4) << std::setfill('0') 
										<< m_memoryManager.Read16(address + 2);
									 instructionSize += 2;
								 }
								 break;
						 default: ss << "Unknown mode 7/" << srcReg; break;
					 }
					 break;
			 }
			 
			 ss << ",";
			 
			 // Destination addressing mode
			 switch (destMode) {
				 case 0: ss << "D" << destReg; break;                // Data register direct
				 case 1: ss << "A" << destReg; break;                // Address register direct
				 case 2: ss << "(A" << destReg << ")"; break;        // Address register indirect
				 case 3: ss << "(A" << destReg << ")+"; break;       // Address register indirect with post-increment
				 case 4: ss << "-(A" << destReg << ")"; break;       // Address register indirect with pre-decrement
				 case 5: ss << "($" << std::hex << std::setw(4) << std::setfill('0') 
						  << m_memoryManager.Read16(address + instructionSize) << ",A" << destReg << ")";
						  instructionSize += 2; break;               // Address register indirect with displacement
				 case 6: ss << "Complex mode"; break;                // More complex modes
				 case 7:
					 switch (destReg) {
						 case 0: ss << "($" << std::hex << std::setw(4) << std::setfill('0') 
								  << m_memoryManager.Read16(address + instructionSize) << ").W";
								  instructionSize += 2; break;
						 case 1: ss << "($" << std::hex << std::setw(8) << std::setfill('0') 
								  << m_memoryManager.Read32(address + instructionSize) << ").L";
								  instructionSize += 4; break;
						 default: ss << "Unknown mode 7/" << destReg; break;
					 }
					 break;
			 }
			 break;
		 }
		 case 0x4: {
			 // Miscellaneous instructions
			 if ((opcode & 0xFFC0) == 0x4E80) {
				 // JSR
				 int mode = (opcode >> 3) & 0x7;
				 int reg = opcode & 0x7;
				 ss << "JSR    ";
				 switch (mode) {
					 case 2: ss << "(A" << reg << ")"; break;
					 case 5: ss << "($" << std::hex << std::setw(4) << std::setfill('0') 
							  << m_memoryManager.Read16(address + 2) << ",A" << reg << ")";
							  instructionSize += 2; break;
					 case 7: 
						 if (reg == 1) {
							 ss << "($" << std::hex << std::setw(8) << std::setfill('0') 
								<< m_memoryManager.Read32(address + 2) << ").L";
							 instructionSize += 4;
						 }
						 break;
					 default: ss << "Unknown mode"; break;
				 }
			 } else if ((opcode & 0xFFC0) == 0x4EC0) {
				 // JMP
				 int mode = (opcode >> 3) & 0x7;
				 int reg = opcode & 0x7;
				 ss << "JMP    ";
				 switch (mode) {
					 case 2: ss << "(A" << reg << ")"; break;
					 case 5: ss << "($" << std::hex << std::setw(4) << std::setfill('0') 
							  << m_memoryManager.Read16(address + 2) << ",A" << reg << ")";
							  instructionSize += 2; break;
					 case 7: 
						 if (reg == 1) {
							 ss << "($" << std::hex << std::setw(8) << std::setfill('0') 
								<< m_memoryManager.Read32(address + 2) << ").L";
							 instructionSize += 4;
						 }
						 break;
					 default: ss << "Unknown mode"; break;
				 }
			 } else if (opcode == 0x4E71) {
				 ss << "NOP";
			 } else if (opcode == 0x4E75) {
				 ss << "RTS";
			 } else if (opcode == 0x4E77) {
				 ss << "RTR";
			 } else if ((opcode & 0xFFF0) == 0x4E40) {
				 ss << "TRAP   #" << (opcode & 0xF);
			 } else {
				 ss << "Unknown Group 4: $" << std::hex << std::setw(4) << std::setfill('0') << opcode;
			 }
			 break;
		 }
		 case 0x6: {
			 // Branch instructions
			 uint16_t condition = (opcode >> 8) & 0xF;
			 int8_t displacement = opcode & 0xFF;
			 uint32_t targetAddress = address + 2 + static_cast<int8_t>(displacement);
			 
			 std::string condStr;
			 switch (condition) {
				 case 0x0: condStr = "RA"; break;  // BRA
				 case 0x1: condStr = "SR"; break;  // BSR
				 case 0x2: condStr = "HI"; break;  // BHI
				 case 0x3: condStr = "LS"; break;  // BLS
				 case 0x4: condStr = "CC"; break;  // BCC
				 case 0x5: condStr = "CS"; break;  // BCS
				 case 0x6: condStr = "NE"; break;  // BNE
				 case 0x7: condStr = "EQ"; break;  // BEQ
				 case 0x8: condStr = "VC"; break;  // BVC
				 case 0x9: condStr = "VS"; break;  // BVS
				 case 0xA: condStr = "PL"; break;  // BPL
				 case 0xB: condStr = "MI"; break;  // BMI
				 case 0xC: condStr = "GE"; break;  // BGE
				 case 0xD: condStr = "LT"; break;  // BLT
				 case 0xE: condStr = "GT"; break;  // BGT
				 case 0xF: condStr = "LE"; break;  // BLE
				 default: condStr = "??"; break;
			 }
			 
			 // Handle word displacement
			 if (displacement == 0) {
				 int16_t wordDisplacement = m_memoryManager.Read16(address + 2);
				 targetAddress = address + 2 + wordDisplacement;
				 instructionSize += 2;
			 }
			 
			 ss << "B" << condStr << "    $" << std::hex << std::setw(8) << std::setfill('0') << targetAddress;
			 break;
		 }
		 case 0x7: {
			 // MOVEQ
			 int reg = (opcode >> 9) & 0x7;
			 int8_t data = opcode & 0xFF;
			 ss << "MOVEQ  #$" << std::hex << std::setw(2) << std::setfill('0') 
				<< (data & 0xFF) << ",D" << reg;
			 break;
		 }
		 default:
			 ss << "Unknown opcode: $" << std::hex << std::setw(4) << std::setfill('0') << opcode;
			 break;
	 }
	 
	 return ss.str();
 }
 
 bool M68000CPU::RegisterHook(uint32_t address, std::function<void()> callback) {
	 m_hooks[address] = callback;
	 m_logger.Debug("M68000CPU", "Hook registered at address 0x" + std::to_string(address));
	 return true;
 }
 
 bool M68000CPU::RemoveHook(uint32_t address) {
	 auto it = m_hooks.find(address);
	 if (it != m_hooks.end()) {
		 m_hooks.erase(it);
		 m_logger.Debug("M68000CPU", "Hook removed from address 0x" + std::to_string(address));
		 return true;
	 }
	 return false;
 }
 
 void M68000CPU::RefillPrefetchQueue() {
	 // The 68000 has a two-word prefetch queue
	 m_registers.prefetch[0] = m_memoryManager.Read16(m_registers.pc);
	 m_registers.prefetch[1] = m_memoryManager.Read16(m_registers.pc + 2);
 }
 
 uint16_t M68000CPU::FetchNextInstruction() {
	 // Get the first word from the prefetch queue
	 uint16_t instruction = m_registers.prefetch[0];
	 
	 // Advance PC
	 m_registers.pc += 2;
	 
	 // Shift the queue
	 m_registers.prefetch[0] = m_registers.prefetch[1];
	 
	 // Fetch the next word for the queue
	 m_registers.prefetch[1] = m_memoryManager.Read16(m_registers.pc + 2);
	 
	 return instruction;
 }
 
 void M68000CPU::ExecuteGroup0(uint16_t opcode) {
	 // Group 0: Bit manipulation, MOVEP, immediate operations
	 // This is a simplified implementation
	 if ((opcode & 0xFF00) == 0x0000) {
		 // ORI.B #imm,Dn
		 uint8_t imm = m_memoryManager.Read16(m_registers.pc) & 0xFF;
		 uint8_t reg = opcode & 0x7;
		 m_registers.pc += 2; // Skip immediate word
		 
		 uint8_t result = (m_registers.d[reg] & 0xFF) | imm;
		 m_registers.d[reg] = (m_registers.d[reg] & 0xFFFFFF00) | result;
		 
		 // Update flags
		 m_registers.sr &= ~(SR_N | SR_Z | SR_V | SR_C);
		 if (result == 0) m_registers.sr |= SR_Z;
		 if (result & 0x80) m_registers.sr |= SR_N;
	 } else if (opcode == 0x003C) {
		 // ORI to CCR
		 uint16_t imm = m_memoryManager.Read16(m_registers.pc);
		 m_registers.pc += 2; // Skip immediate word
		 
		 m_registers.sr |= (imm & 0x1F); // Only affect lower 5 bits (CCR)
	 } else if (opcode == 0x007C) {
		 // ORI to SR (privileged)
		 if (!(m_registers.sr & SR_S)) {
			 // Privilege violation if not in supervisor mode
			 TriggerException(ExceptionType::PRIVILEGE_VIOLATION);
			 return;
		 }
		 uint16_t imm = m_memoryManager.Read16(m_registers.pc);
		 m_registers.pc += 2; // Skip immediate word
		 
		 m_registers.sr |= imm; // Affect entire SR
	 } else {
		 // Default implementation for other Group 0 instructions
		 m_logger.Warning("M68000CPU", "Unimplemented Group 0 opcode: " + 
						 std::to_string(opcode));
	 }
 }
 
 void M68000CPU::ExecuteGroup1(uint16_t opcode) {
	 // Group 1: MOVE.B
	 // These are handled in ExecuteGroup3 since MOVE.B, MOVE.W, and MOVE.L
	 // can be processed by the same code with appropriate size adjustments
	 ExecuteGroup3(opcode);
 }
 
 void M68000CPU::ExecuteGroup2(uint16_t opcode) {
	 // Group 2: MOVE.L
	 // These are handled in ExecuteGroup3 since MOVE.B, MOVE.W, and MOVE.L
	 // can be processed by the same code with appropriate size adjustments
	 ExecuteGroup3(opcode);
 }
 
 void M68000CPU::ExecuteGroup3(uint16_t opcode) {
	 // Group 3: MOVE instructions (MOVE.B, MOVE.W, MOVE.L)
	 
	 // Determine operation size
	 int size = 2; // Default to word
	 switch ((opcode >> 12) & 0x3) {
		 case 0x1: size = 1; break; // Byte
		 case 0x2: size = 4; break; // Long
		 case 0x3: size = 2; break; // Word
	 }
	 
	 // Source mode and register
	 int srcMode = (opcode >> 3) & 0x7;
	 int srcReg = opcode & 0x7;
	 
	 // Destination mode and register
	 int destMode = (opcode >> 6) & 0x7;
	 int destReg = (opcode >> 9) & 0x7;
	 
	 // Calculate effective addresses (simplified)
	 uint32_t srcAddr = CalculateEffectiveAddress(srcMode, srcReg);
	 uint32_t destAddr = CalculateEffectiveAddress(destMode, destReg);
	 
	 // Read source value
	 uint32_t value = 0;
	 switch (size) {
		 case 1: // Byte
			 if (srcMode == 0) {
				 // Data register direct
				 value = m_registers.d[srcReg] & 0xFF;
			 } else {
				 value = m_memoryManager.Read8(srcAddr);
			 }
			 break;
		 case 2: // Word
			 if (srcMode == 0) {
				 // Data register direct
				 value = m_registers.d[srcReg] & 0xFFFF;
			 } else if (srcMode == 1) {
				 // Address register direct
				 value = m_registers.a[srcReg] & 0xFFFF;
			 } else {
				 value = m_memoryManager.Read16(srcAddr);
			 }
			 break;
		 case 4: // Long
			 if (srcMode == 0) {
				 // Data register direct
				 value = m_registers.d[srcReg];
			 } else if (srcMode == 1) {
				 // Address register direct
				 value = m_registers.a[srcReg];
			 } else {
				 value = m_memoryManager.Read32(srcAddr);
			 }
			 break;
	 }
	 
	 // Write to destination
	 switch (size) {
		 case 1: // Byte
			 if (destMode == 0) {
				 // Data register direct
				 m_registers.d[destReg] = (m_registers.d[destReg] & 0xFFFFFF00) | (value & 0xFF);
			 } else {
				 m_memoryManager.Write8(destAddr, value & 0xFF);
			 }
			 break;
		 case 2: // Word
			 if (destMode == 0) {
				 // Data register direct
				 m_registers.d[destReg] = (m_registers.d[destReg] & 0xFFFF0000) | (value & 0xFFFF);
			 } else if (destMode == 1) {
				 // Address register direct
				 m_registers.a[destReg] = (m_registers.a[destReg] & 0xFFFF0000) | (value & 0xFFFF);
			 } else {
				 m_memoryManager.Write16(destAddr, value & 0xFFFF);
			 }
			 break;
		 case 4: // Long
			 if (destMode == 0) {
				 // Data register direct
				 m_registers.d[destReg] = value;
			 } else if (destMode == 1) {
				 // Address register direct
				 m_registers.a[destReg] = value;
			 } else {
				 m_memoryManager.Write32(destAddr, value);
			 }
			 break;
	 }
	 
	 // Update flags (except for moves to A registers)
	 if (destMode != 1) {
		 // Clear C and V flags
		 m_registers.sr &= ~(SR_C | SR_V);
		 
		 // Set N flag if result is negative
		 if ((size == 1 && (value & 0x80)) ||
			 (size == 2 && (value & 0x8000)) ||
			 (size == 4 && (value & 0x80000000))) {
			 m_registers.sr |= SR_N;
		 } else {
			 m_registers.sr &= ~SR_N;
		 }
		 
		 // Set Z flag if result is zero
		 if (((size == 1) && ((value & 0xFF) == 0)) ||
			 ((size == 2) && ((value & 0xFFFF) == 0)) ||
			 ((size == 4) && (value == 0))) {
			 m_registers.sr |= SR_Z;
		 } else {
			 m_registers.sr &= ~SR_Z;
		 }
	 }
 }
 
 void M68000CPU::ExecuteGroup4(uint16_t opcode) {
	 // Group 4: Miscellaneous instructions
	 
	 if ((opcode & 0xFFC0) == 0x4E80) {
		 // JSR instruction
		 int mode = (opcode >> 3) & 0x7;
		 int reg = opcode & 0x7;
		 
		 uint32_t effectiveAddr = CalculateEffectiveAddress(mode, reg);
		 
		 // Push current PC to stack
		 PushLong(m_registers.pc);
		 
		 // Set PC to target address
		 m_registers.pc = effectiveAddr;
		 
		 // Refill prefetch queue
		 RefillPrefetchQueue();
	 } else if ((opcode & 0xFFC0) == 0x4EC0) {
		 // JMP instruction
		 int mode = (opcode >> 3) & 0x7;
		 int reg = opcode & 0x7;
		 
		 uint32_t effectiveAddr = CalculateEffectiveAddress(mode, reg);
		 
		 // Set PC to target address
		 m_registers.pc = effectiveAddr;
		 
		 // Refill prefetch queue
		 RefillPrefetchQueue();
	 } else if (opcode == 0x4E71) {
		 // NOP - No operation
		 // Do nothing
	 } else if (opcode == 0x4E75) {
		 // RTS - Return from subroutine
		 m_registers.pc = PopLong();
		 
		 // Refill prefetch queue
		 RefillPrefetchQueue();
	 } else if (opcode == 0x4E77) {
		 // RTR - Return and restore condition codes
		 uint16_t ccr = PopWord() & 0xFF; // Lower byte only
		 m_registers.pc = PopLong();
		 
		 // Update condition codes (lower byte of SR)
		 m_registers.sr = (m_registers.sr & 0xFF00) | ccr;
		 
		 // Refill prefetch queue
		 RefillPrefetchQueue();
	 } else if ((opcode & 0xFFF0) == 0x4E40) {
		 // TRAP instruction
		 uint8_t vector = opcode & 0xF; // Vector number 0-15
		 TriggerException(ExceptionType::TRAP, vector);
	 } else {
		 // Default implementation for other Group 4 instructions
		 m_logger.Warning("M68000CPU", "Unimplemented Group 4 opcode: " + 
						 std::to_string(opcode));
	 }
 }
 
 void M68000CPU::ExecuteGroup5(uint16_t opcode) {
	 // Group 5: ADDQ, SUBQ, Scc, DBcc
	 
	 // Default implementation
	 m_logger.Warning("M68000CPU", "Unimplemented Group 5 opcode: " + 
					 std::to_string(opcode));
 }
 
 void M68000CPU::ExecuteGroup6(uint16_t opcode) {
	 // Group 6: Branch instructions (Bcc, BSR, BRA)
	 
	 // Extract condition code and displacement
	 uint8_t condition = (opcode >> 8) & 0xF;
	 int8_t displacement = opcode & 0xFF;
	 
	 // Determine target address
	 uint32_t targetAddr;
	 if (displacement == 0) {
		 // 16-bit displacement
		 int16_t wordDisplacement = m_memoryManager.Read16(m_registers.pc);
		 m_registers.pc += 2; // Skip displacement word
		 targetAddr = m_registers.pc + wordDisplacement;
	 } else {
		 // 8-bit displacement
		 targetAddr = m_registers.pc + static_cast<int8_t>(displacement);
	 }
	 
	 bool takeBranch = false;
	 
	 // Evaluate condition
	 switch (condition) {
		 case 0x0: // BRA - Branch always
			 takeBranch = true;
			 break;
		 case 0x1: // BSR - Branch to subroutine
			 PushLong(m_registers.pc); // Push return address
			 takeBranch = true;
			 break;
		 case 0x2: // BHI - Branch if higher (C=0 and Z=0)
			 takeBranch = !IsFlagSet(SR_C) && !IsFlagSet(SR_Z);
			 break;
		 case 0x3: // BLS - Branch if lower or same (C=1 or Z=1)
			 takeBranch = IsFlagSet(SR_C) || IsFlagSet(SR_Z);
			 break;
		 case 0x4: // BCC - Branch if carry clear (C=0)
			 takeBranch = !IsFlagSet(SR_C);
			 break;
		 case 0x5: // BCS - Branch if carry set (C=1)
			 takeBranch = IsFlagSet(SR_C);
			 break;
		 case 0x6: // BNE - Branch if not equal (Z=0)
			 takeBranch = !IsFlagSet(SR_Z);
			 break;
		 case 0x7: // BEQ - Branch if equal (Z=1)
			 takeBranch = IsFlagSet(SR_Z);
			 break;
		 case 0x8: // BVC - Branch if overflow clear (V=0)
			 takeBranch = !IsFlagSet(SR_V);
			 break;
		 case 0x9: // BVS - Branch if overflow set (V=1)
			 takeBranch = IsFlagSet(SR_V);
			 break;
		 case 0xA: // BPL - Branch if plus (N=0)
			 takeBranch = !IsFlagSet(SR_N);
			 break;
		 case 0xB: // BMI - Branch if minus (N=1)
			 takeBranch = IsFlagSet(SR_N);
			 break;
		 case 0xC: // BGE - Branch if greater or equal (N=V)
			 takeBranch = (IsFlagSet(SR_N) && IsFlagSet(SR_V)) || 
						  (!IsFlagSet(SR_N) && !IsFlagSet(SR_V));
			 break;
		 case 0xD: // BLT - Branch if less than (N≠V)
			 takeBranch = (IsFlagSet(SR_N) && !IsFlagSet(SR_V)) || 
						  (!IsFlagSet(SR_N) && IsFlagSet(SR_V));
			 break;
		 case 0xE: // BGT - Branch if greater than (N=V and Z=0)
			 takeBranch = ((IsFlagSet(SR_N) && IsFlagSet(SR_V)) || 
						   (!IsFlagSet(SR_N) && !IsFlagSet(SR_V))) && 
						  !IsFlagSet(SR_Z);
			 break;
		 case 0xF: // BLE - Branch if less or equal (Z=1 or N≠V)
			 takeBranch = IsFlagSet(SR_Z) || 
						  (IsFlagSet(SR_N) && !IsFlagSet(SR_V)) || 
						  (!IsFlagSet(SR_N) && IsFlagSet(SR_V));
			 break;
	 }
	 
	 // If condition is true, branch to target address
	 if (takeBranch) {
		 m_registers.pc = targetAddr;
		 RefillPrefetchQueue();
	 }
 }
 
 void M68000CPU::ExecuteGroup7(uint16_t opcode) {
	 // Group 7: MOVEQ instruction
	 
	 // Extract register and data
	 int reg = (opcode >> 9) & 0x7;
	 int8_t data = opcode & 0xFF;
	 
	 // Sign-extend to 32 bits and store in data register
	 m_registers.d[reg] = static_cast<int32_t>(data);
	 
	 // Update flags
	 m_registers.sr &= ~(SR_N | SR_Z | SR_V | SR_C);
	 
	 // Set N flag if result is negative
	 if (data < 0) {
		 m_registers.sr |= SR_N;
	 }
	 
	 // Set Z flag if result is zero
	 if (data == 0) {
		 m_registers.sr |= SR_Z;
	 }
 }
 
 void M68000CPU::ExecuteGroup8(uint16_t opcode) {
	 // Group 8: OR, DIV, SBCD
	 
	 // Default implementation
	 m_logger.Warning("M68000CPU", "Unimplemented Group 8 opcode: " + 
					 std::to_string(opcode));
 }
 
 void M68000CPU::ExecuteGroup9(uint16_t opcode) {
	 // Group 9: SUB, SUBX
	 
	 // Default implementation
	 m_logger.Warning("M68000CPU", "Unimplemented Group 9 opcode: " + 
					 std::to_string(opcode));
 }
 
 void M68000CPU::ExecuteGroupA(uint16_t opcode) {
	 // Group A: A-Line
	 TriggerException(ExceptionType::LINE_A_EMULATOR);
 }
 
 void M68000CPU::ExecuteGroupB(uint16_t opcode) {
	 // Group B: CMP, CMPM, EOR
	 
	 // Default implementation
	 m_logger.Warning("M68000CPU", "Unimplemented Group B opcode: " + 
					 std::to_string(opcode));
 }
 
 void M68000CPU::ExecuteGroupC(uint16_t opcode) {
	 // Group C: AND, MUL, ABCD, EXG
	 
	 // Default implementation
	 m_logger.Warning("M68000CPU", "Unimplemented Group C opcode: " + 
					 std::to_string(opcode));
 }
 
 void M68000CPU::ExecuteGroupD(uint16_t opcode) {
	 // Group D: ADD, ADDX
	 
	 // Default implementation
	 m_logger.Warning("M68000CPU", "Unimplemented Group D opcode: " + 
					 std::to_string(opcode));
 }
 
 void M68000CPU::ExecuteGroupE(uint16_t opcode) {
	 // Group E: Shift/rotate, bit operations
	 
	 // Default implementation
	 m_logger.Warning("M68000CPU", "Unimplemented Group E opcode: " + 
					 std::to_string(opcode));
 }
 
 void M68000CPU::ExecuteGroupF(uint16_t opcode) {
	 // Group F: F-Line
	 TriggerException(ExceptionType::LINE_F_EMULATOR);
 }
 
 void M68000CPU::PushLong(uint32_t value) {
	 m_registers.a[7] -= 4;
	 m_memoryManager.Write32(m_registers.a[7], value);
 }
 
 void M68000CPU::PushWord(uint16_t value) {
	 m_registers.a[7] -= 2;
	 m_memoryManager.Write16(m_registers.a[7], value);
 }
 
 uint32_t M68000CPU::PopLong() {
	 uint32_t value = m_memoryManager.Read32(m_registers.a[7]);
	 m_registers.a[7] += 4;
	 return value;
 }
 
 uint16_t M68000CPU::PopWord() {
	 uint16_t value = m_memoryManager.Read16(m_registers.a[7]);
	 m_registers.a[7] += 2;
	 return value;
 }
 
 uint32_t M68000CPU::CalculateEffectiveAddress(uint16_t mode, uint16_t reg) {
	 uint32_t address = 0;
	 
	 switch (mode) {
		 case 0: // Data register direct - not a memory address
		 case 1: // Address register direct - not a memory address
			 break;
			 
		 case 2: // Address register indirect
			 address = m_registers.a[reg];
			 break;
			 
		 case 3: // Address register indirect with post-increment
			 address = m_registers.a[reg];
			 // Post-increment handled by the caller
			 break;
			 
		 case 4: // Address register indirect with pre-decrement
			 // Pre-decrement handled by the caller
			 break;
			 
		 case 5: // Address register indirect with displacement
			 {
				 int16_t displacement = m_memoryManager.Read16(m_registers.pc);
				 m_registers.pc += 2;
				 address = m_registers.a[reg] + displacement;
			 }
			 break;
			 
		 case 6: // Address register indirect with index
			 // Not implemented
			 m_logger.Warning("M68000CPU", "Unimplemented addressing mode 6");
			 break;
			 
		 case 7: // Extended addressing modes
			 switch (reg) {
				 case 0: // Absolute short
					 {
						 int16_t addr = m_memoryManager.Read16(m_registers.pc);
						 m_registers.pc += 2;
						 address = static_cast<uint32_t>(addr) & 0xFFFF;
					 }
					 break;
					 
				 case 1: // Absolute long
					 address = m_memoryManager.Read32(m_registers.pc);
					 m_registers.pc += 4;
					 break;
					 
				 case 2: // PC with displacement
					 {
						 int16_t displacement = m_memoryManager.Read16(m_registers.pc);
						 m_registers.pc += 2;
						 address = m_registers.pc + displacement;
					 }
					 break;
					 
				 case 3: // PC with index
					 // Not implemented
					 m_logger.Warning("M68000CPU", "Unimplemented addressing mode 7/3");
					 break;
					 
				 case 4: // Immediate
					 address = m_registers.pc;
					 // Size-dependent advancement handled by the caller
					 break;
					 
				 default:
					 m_logger.Warning("M68000CPU", "Unimplemented addressing mode 7/" + std::to_string(reg));
					 break;
			 }
			 break;
			 
		 default:
			 m_logger.Warning("M68000CPU", "Invalid addressing mode: " + std::to_string(mode));
			 break;
	 }
	 
	 return address;
 }
 
 void M68000CPU::SwitchPrivilegeMode(PrivilegeMode newMode) {
	 bool currentSupervisor = (m_registers.sr & SR_S) != 0;
	 bool newSupervisor = (newMode == PrivilegeMode::SUPERVISOR);
	 
	 if (currentSupervisor != newSupervisor) {
		 if (newSupervisor) {
			 // USER -> SUPERVISOR
			 m_registers.usp = m_registers.a[7];
			 m_registers.a[7] = m_registers.ssp;
			 m_registers.sr |= SR_S;
		 } else {
			 // SUPERVISOR -> USER
			 m_registers.ssp = m_registers.a[7];
			 m_registers.a[7] = m_registers.usp;
			 m_registers.sr &= ~SR_S;
		 }
	 }
 }
 
 bool M68000CPU::CheckPendingInterrupts() {
	 // Check if interrupts are masked
	 int currentIPL = (m_registers.sr >> 8) & 0x7;
	 
	 // Only process interrupt if its level is higher than the current mask
	 if (m_interruptLevel > currentIPL) {
		 // For simplicity, we'll use a fixed vector number based on the interrupt level
		 int vector = 24 + m_interruptLevel;
		 
		 // Set the interrupt mask to the current interrupt level
		 m_registers.sr &= ~0x0700;  // Clear current IPL
		 m_registers.sr |= (m_interruptLevel << 8);  // Set new IPL
		 
		 // Trigger the interrupt
		 TriggerException(ExceptionType::INTERRUPT, vector);
		 
		 return true;
	 }
	 
	 return false;
 }
 
 void M68000CPU::UpdateConditionCodes(uint32_t src, uint32_t dst, uint32_t result, uint16_t size, bool isAdd) {
	 // Clear all relevant flags
	 m_registers.sr &= ~(SR_X | SR_N | SR_Z | SR_V | SR_C);
	 
	 // Calculate masks based on size
	 uint32_t signMask, valueMask;
	 switch (size) {
		 case 1: // Byte
			 signMask = 0x80;
			 valueMask = 0xFF;
			 break;
		 case 2: // Word
			 signMask = 0x8000;
			 valueMask = 0xFFFF;
			 break;
		 case 4: // Long
			 signMask = 0x80000000;
			 valueMask = 0xFFFFFFFF;
			 break;
		 default:
			 signMask = 0;
			 valueMask = 0;
			 break;
	 }
	 
	 // Calculate N flag (negative)
	 if (result & signMask) {
		 m_registers.sr |= SR_N;
	 }
	 
	 // Calculate Z flag (zero)
	 if ((result & valueMask) == 0) {
		 m_registers.sr |= SR_Z;
	 }
	 
	 // Calculate V flag (overflow)
	 // For addition: Set if signs of operands are the same and sign of result differs
	 // For subtraction: Set if signs of operands differ and sign of result differs from destination
	 if (isAdd) {
		 if (((src & signMask) == (dst & signMask)) && 
			 ((src & signMask) != (result & signMask))) {
			 m_registers.sr |= SR_V;
		 }
	 } else {
		 if (((src & signMask) != (dst & signMask)) && 
			 ((dst & signMask) != (result & signMask))) {
			 m_registers.sr |= SR_V;
		 }
	 }
	 
	 // Calculate C and X flags (carry and extend)
	 // For addition: Set if result is less than either operand
	 // For subtraction: Set if result is greater than destination
	 if (isAdd) {
		 if ((result & valueMask) < (src & valueMask) || 
			 (result & valueMask) < (dst & valueMask)) {
			 m_registers.sr |= SR_C | SR_X;
		 }
	 } else {
		 if ((src & valueMask) > (dst & valueMask)) {
			 m_registers.sr |= SR_C | SR_X;
		 }
	 }
 }
 
 } // namespace NiXX32