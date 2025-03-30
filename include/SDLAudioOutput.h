// SDLAudioOutput.h - SDL2-based audio output for the NiXX-32 emulator
#pragma once

#include <SDL.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

// Forward declarations
class AudioSystem;

class SDLAudioOutput {
public:
    // Audio callback function type
    using AudioCallback = std::function<void(int16_t*, int)>;

private:
    // SDL audio device
    SDL_AudioDeviceID m_audioDevice;
    
    // Audio specifications
    SDL_AudioSpec m_audioSpec;
    
    // Audio buffer
    std::vector<int16_t> m_audioBuffer;
    
    // Audio thread synchronization
    std::mutex m_audioMutex;
    std::atomic<bool> m_audioEnabled;
    
    // Reference to the audio system
    AudioSystem* m_audioSystem;
    
    // Audio callback
    AudioCallback m_callback;
    
    // Volume control
    float m_masterVolume;

public:
    SDLAudioOutput(int frequency = 44100, int bufferSize = 2048);
    ~SDLAudioOutput();
    
    // Initialization
    bool initialize();
    
    // Audio control
    void start();
    void stop();
    void pause(bool paused);
    
    // Volume control
    void setVolume(float volume);
    float getVolume() const { return m_masterVolume; }
    
    // Configuration
    void setAudioSystem(AudioSystem* audioSystem);
    void setCallback(const AudioCallback& callback);
    
    // Audio buffer access (for debugging)
    const std::vector<int16_t>& getAudioBuffer() const { return m_audioBuffer; }
    
    // Direct SDL audio device access
    SDL_AudioDeviceID getAudioDevice() const { return m_audioDevice; }
    const SDL_AudioSpec& getAudioSpec() const { return m_audioSpec; }
    
    // Static callback for SDL
    static void audioCallback(void* userdata, Uint8* stream, int len);
};

// SDLAudioOutput.cpp - Implementation of the SDL audio output
#include "SDLAudioOutput.h"
#include "AudioSystem.h"
#include <iostream>
#include <algorithm>

SDLAudioOutput::SDLAudioOutput(int frequency, int bufferSize)
    : m_audioDevice(0)
    , m_audioSystem(nullptr)
    , m_audioEnabled(false)
    , m_masterVolume(1.0f)
{
    // Initialize audio spec
    SDL_zero(m_audioSpec);
    m_audioSpec.freq = frequency;
    m_audioSpec.format = AUDIO_S16SYS;  // Signed 16-bit samples
    m_audioSpec.channels = 2;           // Stereo
    m_audioSpec.samples = bufferSize;
    m_audioSpec.callback = SDLAudioOutput::audioCallback;
    m_audioSpec.userdata = this;
    
    // Allocate audio buffer
    m_audioBuffer.resize(bufferSize * 2, 0);  // Stereo, so *2
}

SDLAudioOutput::~SDLAudioOutput() {
    // Close the audio device
    if (m_audioDevice != 0) {
        SDL_CloseAudioDevice(m_audioDevice);
        m_audioDevice = 0;
    }
}

bool SDLAudioOutput::initialize() {
    // Initialize SDL audio subsystem if not already initialized
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            std::cerr << "Failed to initialize SDL audio: " << SDL_GetError() << std::endl;
            return false;
        }
    }
    
    // Open the audio device
    SDL_AudioSpec obtainedSpec;
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &m_audioSpec, &obtainedSpec, 0);
    
    if (m_audioDevice == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Update our spec with the actual values
    m_audioSpec = obtainedSpec;
    
    // Resize buffer if necessary
    if (m_audioSpec.samples != m_audioBuffer.size() / 2) {
        m_audioBuffer.resize(m_audioSpec.samples * 2, 0);  // Stereo, so *2
    }
    
    std::cout << "Audio initialized: " << m_audioSpec.freq << "Hz, ";
    std::cout << (m_audioSpec.channels == 2 ? "Stereo" : "Mono") << ", ";
    std::cout << "Buffer size: " << m_audioSpec.samples << std::endl;
    
    return true;
}

void SDLAudioOutput::start() {
    if (m_audioDevice != 0) {
        m_audioEnabled.store(true);
        SDL_PauseAudioDevice(m_audioDevice, 0);  // Unpause
    }
}

void SDLAudioOutput::stop() {
    if (m_audioDevice != 0) {
        m_audioEnabled.store(false);
        SDL_PauseAudioDevice(m_audioDevice, 1);  // Pause
    }
}

void SDLAudioOutput::pause(bool paused) {
    if (m_audioDevice != 0) {
        m_audioEnabled.store(!paused);
        SDL_PauseAudioDevice(m_audioDevice, paused ? 1 : 0);
    }
}

void SDLAudioOutput::setVolume(float volume) {
    // Clamp volume to [0.0, 1.0]
    m_masterVolume = std::max(0.0f, std::min(1.0f, volume));
}

void SDLAudioOutput::setAudioSystem(AudioSystem* audioSystem) {
    std::lock_guard<std::mutex> lock(m_audioMutex);
    m_audioSystem = audioSystem;
}

void SDLAudioOutput::setCallback(const AudioCallback& callback) {
    std::lock_guard<std::mutex> lock(m_audioMutex);
    m_callback = callback;
}

void SDLAudioOutput::audioCallback(void* userdata, Uint8* stream, int len) {
    // Get the audio output instance
    SDLAudioOutput* audio = static_cast<SDLAudioOutput*>(userdata);
    
    // Convert byte length to sample count (16-bit stereo)
    int sampleCount = len / sizeof(int16_t);
    
    // Cast stream to int16_t pointer
    int16_t* samples = reinterpret_cast<int16_t*>(stream);
    
    // Clear the buffer
    SDL_memset(stream, 0, len);
    
    // If audio is disabled, just return silence
    if (!audio->m_audioEnabled.load()) {
        return;
    }
    
    // Lock to prevent audio system changes during callback
    std::lock_guard<std::mutex> lock(audio->m_audioMutex);
    
    // If we have an audio system, generate samples
    if (audio->m_audioSystem) {
        audio->m_audioSystem->generateSamples(samples, sampleCount / 2);  // Divide by 2 for stereo
    }
    // Or use the callback if set
    else if (audio->m_callback) {
        audio->m_callback(samples, sampleCount / 2);
    }
    
    // Apply master volume
    if (audio->m_masterVolume != 1.0f) {
        for (int i = 0; i < sampleCount; ++i) {
            samples[i] = static_cast<int16_t>(samples[i] * audio->m_masterVolume);
        }
    }
}