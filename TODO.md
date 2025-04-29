# TODO
## Components:
| Category | Component | Interface Created | Implementation Created | Implementation Complete? | Notes |
| --- | --- | --- | --- | --- | --- |
| Core | NiXX32System | YES | YES | WORKING | Main functionality complete, extra features TBC
| Core | MemoryManager | YES | NO | NO |
| Util | Config | YES | NO | NO |
| Debug | Logger | YES | NO | NO |
| Core | M68000CPU | YES | NO | NO |
| Core | Z80CPU | YES | NO | NO |
| Util | FileSystem | YES | NO | NO |
| Rom | ROMLoader | YES | NO | NO |
| Graphics | GraphicsSystem | YES | NO | NO |
| Graphics | BackgroundLayer | YES | NO | NO |
| Graphics | Sprite | YES | NO | NO |
| Graphics | Effects | YES | NO | NO |
| Audio | AudioSystem | YES | NO | NO |
| Audio | YM2151 | YES | NO | NO |
| Audio | PCMPlayer | YES | NO | NO |
| Audio | QSound | YES | NO | NO |
| Input | InputSystem | YES | NO | NO |
| Platform | SDLRenderer | YES | NO | NO |
| Platform | SDLAudioOutput | YES | NO | NO |
| Debug | Debugger | YES | NO | NO |
| Debug | CPUDebugger | YES | NO | NO |
| Debug | MemoryViewer | YES | NO | NO |
| Main | EmulatorApp | YES | NO | NO |
| Network | NetworkSystem | NO | NO | NO |
| Security | SecuritySystem | NO | NO | NO |
## Detailed RoadMap
### NiXX32System - WORKING
#### Implementation Summary
Most of the primary features are implemented, some additional features like power state management and interrupt handler also implemented. Most other features for NiXX32System implementation can be added later, so will merge for now. Features likely to be implemented next include:
- DMA Support: Many arcade systems of that era had DMA (Direct Memory Access) capabilities for faster data transfers, which isn't explicitly implemented.
- Self-tests and Diagnostics: Arcade hardware typically had self-test routines that aren't included in this implementation.
- Save State Support: While there's error handling for ROM loading, a complete emulator would need comprehensive save state functionality.
- Documentation: More detailed inline documentation would be beneficial for maintaining such a complex codebase.
#### Recorded Progress
1. Initial Setup - COMPLETE
	- ~~Define the hardware variants (NIXX32_ORIGINAL, NIXX32_PLUS)~~
 	- ~~Define power states (FULL_POWER, IDLE, LOW_POWER, SLEEP)~~
	- ~~Create memory address constants for each hardware variant~~
	- ~~Define I/O register address ranges~~
2. Component Creation - COMPLETE
	- ~~Initialize logger first (allows other components to log messages)~~
	- ~~Create configuration manager and load settings~~
	- ~~Create memory manager to handle all memory operations~~
	- ~~Create ROM loader for game ROM management~~
	- ~~Create audio system (prior to audio CPU due to dependency)~~
	- ~~Create main CPU (68000) and audio CPU (Z80)~~
	- ~~Link audio CPU to audio system~~
	- ~~Create graphics system for display rendering~~
	- ~~Create input system for controls~~
3. System Initialization - COMPLETE
	- ~~Apply hardware variant-specific configurations~~
	- ~~Set up memory mappings for both CPUs~~
	- ~~Configure memory regions (ROM, RAM, VRAM, I/O)~~
	- ~~Set up 68000 interrupt vector table~~
	- ~~Install default exception handlers~~
	- ~~Install I/O register handlers~~
 	- ~~Connect subsystems via callbacks and memory-mapped I/O~~
 	- ~~Initialize power management with appropriate thresholds~~
4. Memory Management - ALMOST COMPLETE
	- ~~Define ROM region (read-only)~~
	- ~~Define main RAM region (read-write)~~
	- ~~Define video RAM region (read-write)~~
	- ~~Define I/O registers region (read-write)~~
	- ~~Define sound RAM region (shared with Z80)~~
	- ~~Define Z80 program ROM and RAM regions~~
	- Implement DMA controller for efficient memory transfers
5. Interrupt and I/O Handling - COMPLETE
	- ~~Set up VBLANK interrupt handler for timing~~
	- ~~Configure input system interrupt triggers~~
	- ~~Implement memory-mapped I/O read handlers~~
	- ~~Implement memory-mapped I/O write handlers~~
	- ~~Connect audio commands between CPUs~~
6. Emulation Control Functions - ALMOST COMPLETE
	- ~~Implement RunCycle() for main emulation loop~~
	- ~~Calculate and maintain accurate CPU timing~~
	- ~~Synchronize audio CPU with main CPU~~
	- ~~Implement system Reset() function~~
	- ~~Implement Pause/Resume functionality~~
	- ~~Add debugger attachment support~~
	- Implement save state functionality
7. Power Management Implementation - COMPLETE
	- ~~Create activity monitoring system~~
	- ~~Implement idle/sleep thresholds~~
	- ~~Implement CPU power scaling~~
	- ~~Implement graphics power scaling~~
	- ~~Implement audio power scaling~~
	- ~~Implement power state transitions~~
8. Security & Protection - NOT STARTED
	- Implement copy protection system (PALs, encryption)
	- Add suicide battery emulation
	- Implement secure ROM validation
9. Arcade-Specific Features - NOT STARTED
	- Implement JAMMA interface emulation
	- Create test/service mode with DIP switch access
	- Implement operator settings storage (NVRAM)
	- Add multi-board communication for linked cabinets (NiXX-32 Only)
	- Implement attract mode management
	- Create coin handling and accounting system
10. Audio Enhancements - WAS IN INITIAL SCOPE, NOW UNLIKELY TO BE IMPLEMENTED
	- Add sound amplifier emulation
	- Implement stereo panning specific to cabinet
	- Model speaker/cabinet resonance effects
11. Visual Enhancements - WAS IN INITIAL SCOPE, NOW UNLIKELY TO BE IMPLEMENTED
	- Add CRT-specific rendering (phosphor persistence, bloom)
	- Implement scan line and shadow mask effects
	- Add monitor calibration controls
12. Self-Test & Diagnostics - NOT STARTED
	- Implement ROM checksum verification
	- Create RAM test patterns and verification
	- Add I/O port test functionality
	- Implement sound system diagnostics
	- Create graphics test patterns
8. Resource Management - COMPLETE
	- ~~Implement proper component initialization order~~
	- ~~Implement proper shutdown sequence~~
	- ~~Ensure exception safety throughout codebase~~
	- ~~Implement proper error handling and logging~~
10. Testing and Validation - NOT STARTED
	- Verify memory map configuration
	- Test CPU synchronization
	- Validate interrupt handling
	- Test power management state transitions
	- Validate component interactions
10. Documentation - NOT STARTED
	- Document public API functions
	- Document memory map and I/O regions
	- Document power management configuration
	- Document hardware variant differences
	- Create implementation diagrams
	- Document operator settings and diagnostics
	- Create JAMMA pinout reference
11. Final Steps - STARTED
	- Verify all dependencies are correctly managed
	- Ensure all components can be properly reset
	- ~~Test ROM loading and validation~~
	- ~~Implement final error reporting mechanisms~~
	- Add performance profiling support
	- Implement benchmarking tools for accuracy testing

### MemoryManager - IN PROGRESS
#### Recorded Progress
1. Core Architecture
	- ~~Basic class structure and initialization~~	
	- ~~Constructor/Destructor~~
	- ~~Hardware variant detection~~
	- ~~Memory region management system~~
	- ~~Error logging and reporting~~
	- ~~Thread safety mechanisms~~
	- ~~Self-test and validation routines~~
	- ~~Memory refresh handling (for DRAM)~~
2. Memory Map Configuration
	- ~~Original NiXX-32 (1989) memory layout~~
	- ~~Enhanced NiXX-32+ (1992) memory layout~~
	- Dynamic region reconfiguration
	- Memory mirroring support
	- Memory banking support
	- Address decoding simulation (for accurate timing)
3. Memory Access Implementation
	- ~~8-bit read/write operations~~
	- ~~16-bit read/write operations~~
	- ~~32-bit read/write operations~~
	- ~~Unaligned access detection~~
	- ~~Big-endian format handling~~
	- Proper CPU exception triggering for illegal access
	- Cache simulation
	- Access timing and latency emulation
	- Bus arbitration (for multi-processor access)
	- Bus error simulation
	- Wait states for different memory regions
4. Memory Regions and I/O
	- ~~Region definition and validation~~
	- ~~Memory access permissions enforcement~~
	- ~~Region lookup by address and name~~
	- ~~Memory-mapped I/O support via custom handlers~~
	- DMA (Direct Memory Access) support
	- Hardware interrupt triggering from memory access
	- Memory-mapped peripheral emulation interfaces
	- Memory controller registers emulation
	- VRAM/GRAM specific access patterns
5. ROM Management
	- ~~ROM loading functionality~~
	- ROM signature validation
	- ROM patching support
	- Support for multiple ROM banks
	- Anti-piracy/security chip emulation
	- Boot sequence ROM handling
6. Debugging and Development Features
	- ~~Direct memory pointer access for efficient operations~~
	- Memory breakpoint support
	- Memory watch support
	- Memory state saving/restoration (for save states)
	- Memory visualization and hex dump utilities
	- Memory initialization patterns
	- Memory corruption detection
	- Cheat system integration
7. Advanced Features
	- Power state management (standby, power down)
	- Memory protection/security features
	- Endianness conversion utilities
	- Performance optimization for frequent region access
	- Memory snapshot and comparison tools
	- Battery-backed memory handling (for game saves)
	- Custom memory controller emulation
	- Hardware-specific memory timing
8. Integration
	- ~~System class integration~~
	- ~~Logger integration~~
	- CPU exception integration
	- Debugger integration
	- System state serialization (save/load)
	- Synchronization with audio/video subsystems
	- Real-time memory mapping visualization
9. Arcade-Specific Features
	- Memory-mapped input handling
	- Memory-mapped coin/credit system
	- Operator settings in NVRAM
	- High score table persistence
	- Game-specific memory hacks/patches