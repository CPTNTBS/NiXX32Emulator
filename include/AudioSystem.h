// AudioSystem.h - NiXX-32 Audio Emulation
#pragma once

#include <cstdint>
#include <vector>
#include <queue>
#include <memory>
#include <functional>

class MemoryManager;
class AudioOutput;  // Abstract interface to your audio API (SDL_Audio, PortAudio, etc.)

// Forward declarations for emulated sound chips
class YM2151;
class QSound;
class PCMPlayer;
class Z80CPU;

class AudioSystem {
public:
    // Constants
    static constexpr int BASE_SAMPLE_RATE = 44100;  // Output sample rate
    static constexpr int BASE_FM_CHANNELS = 8;      // YM2151 channels
    static constexpr int ENHANCED_FM_CHANNELS = 12; // Enhanced model channels
    static constexpr int BASE_PCM_QUALITY = 11025;  // Original PCM sample rate
    static constexpr int ENHANCED_PCM_QUALITY = 22050; // Enhanced PCM sample rate

private:
    MemoryManager* m_memory;
    AudioOutput* m_audioOutput;
    bool m_isEnhancedModel;
    
    // Sound chip emulation components
    std::unique_ptr<YM2151> m_ym2151;
    std::unique_ptr<QSound> m_qsound;
    std::unique_ptr<PCMPlayer> m_pcmPlayer;
    std::unique_ptr<Z80CPU> m_z80; // Audio coprocessor
    
    // Audio mixing and output
    std::vector<int16_t> m_mixBuffer;
    float m_masterVolume;
    
    // Communication with main CPU
    std::queue<uint8_t> m_mainToAudioQueue;
    std::queue<uint8_t> m_audioToMainQueue;
    
    // Sound ROM (samples)
    std::vector<uint8_t> m_sampleROM;
    
    // Status flags
    bool m_audioEnabled;
    bool m_z80Enabled;

public:
    AudioSystem(MemoryManager* memory, AudioOutput* audioOutput, bool isEnhancedModel);
    ~AudioSystem();
    
    // System interface
    void initialize();
    void reset();
    void update(int cycles); // Run audio processing for specified number of cycles
    
    // Audio stream generation
    void generateSamples(int16_t* buffer, int numSamples);
    
    // Memory-mapped register access
    uint8_t readRegister(uint32_t address);
    void writeRegister(uint32_t address, uint8_t value);
    
    // YM2151 FM Synthesis registers
    uint8_t readYM2151(uint8_t reg);
    void writeYM2151(uint8_t reg, uint8_t value);
    
    // Q-Sound spatial audio registers
    uint8_t readQSound(uint8_t reg);
    void writeQSound(uint8_t reg, uint8_t value);
    
    // PCM player registers
    uint8_t readPCM(uint8_t reg);
    void writePCM(uint8_t reg, uint8_t value);
    
    // Z80 control
    void resetZ80();
    void haltZ80();
    void resumeZ80();
    bool isZ80Halted() const;
    
    // Communication ports
    void writeToZ80(uint8_t value);
    uint8_t readFromZ80();
    bool hasZ80Message() const;
    
    // Sample ROM access
    uint8_t readSampleROM(uint32_t address) const;
    void loadSampleROM(const std::vector<uint8_t>& data);
    
private:
    // Internal audio processing
    void mixChannels(int16_t* output, int numSamples);
    void processFMSynthesis(int numSamples);
    void processQSound(int numSamples);
    void processPCM(int numSamples);
    
    // Z80 program execution
    void executeZ80(int cycles);
};

//------------------------------------------------------------------
// YM2151.h - Yamaha FM synthesis chip emulation
//------------------------------------------------------------------
class YM2151 {
public:
    YM2151(int clockRate, int sampleRate);
    ~YM2151();
    
    void reset();
    void setRegister(uint8_t reg, uint8_t value);
    uint8_t getRegister(uint8_t reg) const;
    void generateSamples(int16_t* buffer, int numSamples);
    
private:
    // FM synthesis implementation details
    // Would include operator calculations, envelope generators, etc.
    // This is quite complex and would require significant DSP knowledge
    struct FMChannel {
        // Channel parameters
        bool enabled;
        uint8_t algorithm;
        uint8_t feedback;
        
        // Operator parameters (4 operators per channel)
        struct Operator {
            uint8_t detune;
            uint8_t multiple;
            uint8_t totalLevel;
            uint8_t keyScale;
            uint8_t attackRate;
            uint8_t decayRate;
            uint8_t sustainRate;
            uint8_t releaseRate;
            uint8_t sustainLevel;
            bool AMEnabled;
            
            // Runtime state
            float phase;
            float envelope;
            int state; // Attack, decay, sustain, release
        } operators[4];
    };
    
    std::vector<FMChannel> m_channels;
    int m_clockRate;
    int m_sampleRate;
    std::vector<uint8_t> m_registers;
};

//------------------------------------------------------------------
// QSound.h - Q-Sound spatial audio DSP emulation
//------------------------------------------------------------------
class QSound {
public:
    QSound(int sampleRate);
    ~QSound();
    
    void reset();
    void setRegister(uint8_t reg, uint8_t value);
    uint8_t getRegister(uint8_t reg) const;
    void processSamples(int16_t* buffer, int numSamples);
    
private:
    // Spatial audio processing
    struct Channel {
        int16_t position;  // -128 to 127 (left to right)
        uint8_t depth;     // Front to back depth
        uint8_t volume;
    };
    
    std::vector<Channel> m_channels;
    std::vector<uint8_t> m_registers;
    int m_sampleRate;
};

//------------------------------------------------------------------
// PCMPlayer.h - NiXX-B PCM sample playback
//------------------------------------------------------------------
class PCMPlayer {
public:
    PCMPlayer(int sampleRate, const uint8_t* sampleROM, size_t romSize);
    ~PCMPlayer();
    
    void reset();
    void setRegister(uint8_t reg, uint8_t value);
    uint8_t getRegister(uint8_t reg) const;
    void generateSamples(int16_t* buffer, int numSamples);
    
    // Sample playback control
    void playSample(uint32_t address, uint32_t length, int frequency, int volume, bool loop);
    void stopSample(int channel);
    void stopAllSamples();
    
private:
    struct PCMChannel {
        bool active;
        uint32_t currentAddress;
        uint32_t startAddress;
        uint32_t endAddress;
        int frequency;
        int volume;
        bool loop;
        float position; // Fractional sample position
    };
    
    std::vector<PCMChannel> m_channels;
    std::vector<uint8_t> m_registers;
    const uint8_t* m_sampleROM;
    size_t m_romSize;
    int m_sampleRate;
};