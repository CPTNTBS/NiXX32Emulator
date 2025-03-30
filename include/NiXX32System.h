// NiXX-32 Emulation System Architecture
// This file outlines the core components needed to emulate the NiXX-32 arcade board

#include <memory>
#include <vector>
#include <string>

// Forward declarations
class M68000CPU;
class Z80CPU;
class GraphicsSystem;
class AudioSystem;
class InputSystem;
class MemoryManager;
class ROMLoader;

class NiXX32System {
private:
    // Core system components
    std::unique_ptr<M68000CPU> m_mainCPU;
    std::unique_ptr<Z80CPU> m_audioCPU;
    std::unique_ptr<GraphicsSystem> m_graphics;
    std::unique_ptr<AudioSystem> m_audio;
    std::unique_ptr<InputSystem> m_input;
    std::unique_ptr<MemoryManager> m_memory;
    std::unique_ptr<ROMLoader> m_romLoader;
    
    // System state
    bool m_isNiXXPlus;  // Is this the enhanced NiXX-32+ version?
    bool m_running;
    uint64_t m_cycleCount;
    
    // Synchronization variables
    double m_mainCpuClock;  // 16MHz or 20MHz
    double m_audioCpuClock; // 8MHz or 10MHz

public:
    // Constructor determines if we're emulating base or enhanced model
    explicit NiXX32System(bool isEnhancedModel = false);
    ~NiXX32System();
    
    // System lifecycle
    bool initialize();
    bool loadROM(const std::string& romPath);
    void reset();
    void run();
    void pause();
    void shutdown();
    
    // Single step execution for debugging
    void stepFrame();
    void stepInstruction();
    
    // Accessor methods
    M68000CPU* getMainCPU() const { return m_mainCPU.get(); }
    Z80CPU* getAudioCPU() const { return m_audioCPU.get(); }
    GraphicsSystem* getGraphics() const { return m_graphics.get(); }
    AudioSystem* getAudio() const { return m_audio.get(); }
    InputSystem* getInput() const { return m_input.get(); }
    MemoryManager* getMemory() const { return m_memory.get(); }
    
    // System configuration
    bool isEnhancedModel() const { return m_isNiXXPlus; }
};