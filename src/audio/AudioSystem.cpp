/**
 * AudioSystem.cpp
 * Audio system implementation for NiXX-32 arcade board emulation
 * 
 * This file implements the audio system that handles sound generation,
 * FM synthesis, PCM playback, and sound effects for the NiXX-32
 * arcade hardware.
 */

 #include "AudioSystem.h"
 #include "NiXX32System.h"
 #include "Z80CPU.h"
 #include "YM2151.h"
 #include "PCMPlayer.h"
 #include "QSound.h"
 
 #include <algorithm>
 #include <cmath>
 #include <cstring>
 #include <iostream>
 #include <stdexcept>
 
 namespace NiXX32 {
 
 // Memory addresses for audio registers
 constexpr uint32_t AUDIO_CONTROL_REG = 0xA00000;
 constexpr uint32_t AUDIO_STATUS_REG = 0xA00001;
 constexpr uint32_t AUDIO_IRQ_ENABLE_REG = 0xA00002;
 constexpr uint32_t AUDIO_IRQ_STATUS_REG = 0xA00003;
 constexpr uint32_t AUDIO_MASTER_VOL_REG = 0xA00004;
 constexpr uint32_t AUDIO_FM_CONTROL_REG = 0xA00005;
 constexpr uint32_t AUDIO_PCM_CONTROL_REG = 0xA00006;
 constexpr uint32_t AUDIO_QSOUND_CONTROL_REG = 0xA00007;
 
 // Audio chip port addresses
 constexpr uint8_t YM2151_ADDRESS_PORT = 0x20;
 constexpr uint8_t YM2151_DATA_PORT = 0x21;
 constexpr uint8_t PCM_CONTROL_PORT = 0x40;
 constexpr uint8_t PCM_DATA_PORT = 0x41;
 constexpr uint8_t QSOUND_ADDRESS_PORT = 0x60;
 constexpr uint8_t QSOUND_DATA_PORT = 0x61;
 
 // Other constants
 constexpr uint8_t MAX_VOLUME = 255;
 constexpr uint8_t MAX_CHANNELS = 24;
 constexpr uint16_t MAX_SAMPLES = 256;
 constexpr uint16_t MAX_INSTRUMENTS = 128;
 
 /**
  * Constructor
  */
 AudioSystem::AudioSystem(System& system, MemoryManager& memoryManager, Logger& logger)
	 : m_system(system),
	   m_memoryManager(memoryManager),
	   m_logger(logger),
	   m_audioCPU(nullptr),
	   m_variant(AudioHardwareVariant::NIXX32_ORIGINAL),
	   m_masterVolume(MAX_VOLUME),
	   m_paused(false),
	   m_nextChannelId(1)  // Start from 1, 0 can be used as invalid ID
 {
	 // Initialize registers to zero
	 std::memset(&m_registers, 0, sizeof(m_registers));
	 
	 // Set default master volume
	 m_registers.masterVolume = MAX_VOLUME;
	 
	 m_logger.Info("AudioSystem", "Audio system constructed");
 }
 
 /**
  * Destructor
  */
 AudioSystem::~AudioSystem() {
	 m_logger.Info("AudioSystem", "Audio system destructed");
 }
 
 /**
  * Initialize the audio system
  */
 bool AudioSystem::Initialize(AudioHardwareVariant variant) {
	 m_variant = variant;
	 
	 // Configure system based on hardware variant
	 if (m_variant == AudioHardwareVariant::NIXX32_ORIGINAL) {
		 ConfigureOriginalHardware();
	 } else {
		 ConfigurePlusHardware();
	 }
	 
	 // Create audio components
	 try {
		 m_ym2151 = std::make_unique<YM2151>(*this, *this, m_logger);
		 m_pcmPlayer = std::make_unique<PCMPlayer>(*this, *this, m_memoryManager, m_logger);
		 m_qSound = std::make_unique<QSound>(*this, *this, m_memoryManager, m_logger);
	 } catch (const std::exception& e) {
		 m_logger.Error("AudioSystem", "Failed to create audio components: " + std::string(e.what()));
		 return false;
	 }
	 
	 // Initialize YM2151
	 uint32_t ymClockRate = (m_variant == AudioHardwareVariant::NIXX32_ORIGINAL) ? 3579545 : 4000000;
	 if (!m_ym2151->Initialize(ymClockRate, m_config.sampleRate)) {
		 m_logger.Error("AudioSystem", "Failed to initialize YM2151");
		 return false;
	 }
	 
	 // Initialize PCM player
	 PCMPlayerConfig pcmConfig;
	 pcmConfig.channels = m_config.pcmChannels;
	 pcmConfig.sampleRate = m_config.sampleRate;
	 pcmConfig.maxSampleBits = m_config.bitDepth;
	 pcmConfig.stereoSupported = (m_variant == AudioHardwareVariant::NIXX32_PLUS);
	 pcmConfig.sampleROMSize = m_config.soundRamSize;
	 pcmConfig.interpolation = (m_variant == AudioHardwareVariant::NIXX32_PLUS);
	 
	 uint32_t sampleROMBaseAddress = 0x100000;  // Arbitrary base address for sample ROM
	 if (!m_pcmPlayer->Initialize(pcmConfig, sampleROMBaseAddress)) {
		 m_logger.Error("AudioSystem", "Failed to initialize PCM player");
		 return false;
	 }
	 
	 // Initialize QSound (only for NiXX-32+)
	 if (m_variant == AudioHardwareVariant::NIXX32_PLUS) {
		 QSoundConfig qsConfig;
		 qsConfig.channels = 16;
		 qsConfig.reverbSupported = true;
		 qsConfig.delaySupported = true;
		 qsConfig.filteringSupported = true;
		 qsConfig.spatialModeSupported = true;
		 
		 if (!m_qSound->Initialize(qsConfig)) {
			 m_logger.Error("AudioSystem", "Failed to initialize QSound");
			 return false;
		 }
	 }
	 
	 // Initialize FM instruments and PCM samples
	 m_fmInstruments.resize(MAX_INSTRUMENTS);
	 m_pcmSamples.resize(MAX_SAMPLES);
	 
	 // Set up Z80 port handlers
	 RegisterPortHandler(YM2151_ADDRESS_PORT, 
		 [this]() -> uint8_t { return m_ym2151->ReadReg(m_registers.fmControl); },
		 [this](uint8_t value) { m_registers.fmControl = value; }
	 );
	 
	 RegisterPortHandler(YM2151_DATA_PORT, 
		 [this]() -> uint8_t { return m_ym2151->ReadReg(m_registers.fmControl); },
		 [this](uint8_t value) { m_ym2151->WriteReg(m_registers.fmControl, value); }
	 );
	 
	 RegisterPortHandler(PCM_CONTROL_PORT, 
		 [this]() -> uint8_t { return m_pcmPlayer->HandleRegisterRead(m_registers.pcmControl); },
		 [this](uint8_t value) { m_registers.pcmControl = value; }
	 );
	 
	 RegisterPortHandler(PCM_DATA_PORT, 
		 [this]() -> uint8_t { return m_pcmPlayer->HandleRegisterRead(m_registers.pcmControl); },
		 [this](uint8_t value) { m_pcmPlayer->HandleRegisterWrite(m_registers.pcmControl, value); }
	 );
	 
	 if (m_variant == AudioHardwareVariant::NIXX32_PLUS) {
		 RegisterPortHandler(QSOUND_ADDRESS_PORT, 
			 [this]() -> uint8_t { return m_qSound->ReadReg(m_registers.qsoundControl); },
			 [this](uint8_t value) { m_registers.qsoundControl = value; }
		 );
		 
		 RegisterPortHandler(QSOUND_DATA_PORT, 
			 [this]() -> uint8_t { return m_qSound->ReadReg(m_registers.qsoundControl); },
			 [this](uint8_t value) { m_qSound->WriteReg(m_registers.qsoundControl, value); }
		 );
	 }
	 
	 m_logger.Info("AudioSystem", "Audio system initialized for " + 
				   (m_variant == AudioHardwareVariant::NIXX32_ORIGINAL ? "original" : "enhanced") + 
				   " hardware");
	 
	 return true;
 }
 
 /**
  * Configure audio system for original NiXX-32 hardware
  */
 void AudioSystem::ConfigureOriginalHardware() {
	 m_config.sampleRate = 44100;
	 m_config.fmChannels = 8;
	 m_config.pcmChannels = 8;
	 m_config.maxSamples = 64;
	 m_config.soundRamSize = 128 * 1024;  // 128 KB
	 m_config.spatialAudio = false;
	 m_config.bitDepth = 8;
	 
	 m_logger.Info("AudioSystem", "Configured for original NiXX-32 hardware");
 }
 
 /**
  * Configure audio system for enhanced NiXX-32+ hardware
  */
 void AudioSystem::ConfigurePlusHardware() {
	 m_config.sampleRate = 48000;
	 m_config.fmChannels = 8;
	 m_config.pcmChannels = 16;
	 m_config.maxSamples = 256;
	 m_config.soundRamSize = 256 * 1024;  // 256 KB
	 m_config.spatialAudio = true;
	 m_config.bitDepth = 16;
	 
	 m_logger.Info("AudioSystem", "Configured for enhanced NiXX-32+ hardware");
 }
 
 /**
  * Reset the audio system to initial state
  */
 void AudioSystem::Reset() {
	 // Reset components
	 m_ym2151->Reset();
	 m_pcmPlayer->Reset();
	 
	 if (m_variant == AudioHardwareVariant::NIXX32_PLUS) {
		 m_qSound->Reset();
	 }
	 
	 // Reset registers
	 std::memset(&m_registers, 0, sizeof(m_registers));
	 m_registers.masterVolume = MAX_VOLUME;
	 
	 // Reset channels
	 m_channels.clear();
	 m_nextChannelId = 1;
	 
	 // Reset state
	 m_masterVolume = MAX_VOLUME;
	 m_paused = false;
	 
	 m_logger.Info("AudioSystem", "Audio system reset");
 }
 
 /**
  * Update audio state for the current frame
  */
 void AudioSystem::Update(float deltaTime) {
	 if (m_paused) {
		 return;
	 }
	 
	 // Update components
	 m_ym2151->Update(deltaTime);
	 m_pcmPlayer->Update(deltaTime);
	 
	 if (m_variant == AudioHardwareVariant::NIXX32_PLUS) {
		 m_qSound->Update(deltaTime);
	 }
	 
	 // Update channel statuses
	 for (auto it = m_channels.begin(); it != m_channels.end();) {
		 UpdateChannelStatus(it->first);
		 
		 // Remove inactive channels
		 if (!it->second.status.active) {
			 it = m_channels.erase(it);
		 } else {
			 ++it;
		 }
	 }
	 
	 // Check for interrupts
	 HandleInterrupts();
 }
 
 /**
  * Process audio and fill the output buffer
  */
 void AudioSystem::FillAudioBuffer(int16_t* buffer, int sampleCount) {
	 if (m_paused) {
		 // If paused, fill buffer with silence
		 std::memset(buffer, 0, sampleCount * 2 * sizeof(int16_t));
		 return;
	 }
	 
	 // Clear buffer first
	 std::memset(buffer, 0, sampleCount * 2 * sizeof(int16_t));
	 
	 // Mix channels into buffer
	 MixChannels(buffer, sampleCount);
	 
	 // Apply master volume and effects
	 ApplyMasterVolumeAndEffects(buffer, sampleCount);
 }
 
 /**
  * Mix all active channels
  */
 void AudioSystem::MixChannels(int16_t* buffer, int sampleCount) {
	 // Temporary buffers for each component
	 std::vector<int16_t> fmBuffer(sampleCount * 2, 0);
	 std::vector<int16_t> pcmBuffer(sampleCount * 2, 0);
	 
	 // Generate FM synthesis output
	 m_ym2151->Generate(fmBuffer.data(), sampleCount);
	 
	 // Generate PCM output
	 m_pcmPlayer->Generate(pcmBuffer.data(), sampleCount);
	 
	 // If using QSound (NiXX-32+ only), process through spatial audio
	 if (m_variant == AudioHardwareVariant::NIXX32_PLUS && m_config.spatialAudio) {
		 // Extract left and right channels for QSound processing
		 std::vector<int16_t> fmLeft(sampleCount);
		 std::vector<int16_t> fmRight(sampleCount);
		 std::vector<int16_t> pcmLeft(sampleCount);
		 std::vector<int16_t> pcmRight(sampleCount);
		 
		 // De-interleave stereo channels
		 for (int i = 0; i < sampleCount; i++) {
			 fmLeft[i] = fmBuffer[i * 2];
			 fmRight[i] = fmBuffer[i * 2 + 1];
			 pcmLeft[i] = pcmBuffer[i * 2];
			 pcmRight[i] = pcmBuffer[i * 2 + 1];
		 }
		 
		 // Process FM through QSound
		 std::vector<int16_t> fmLeftProcessed(sampleCount);
		 std::vector<int16_t> fmRightProcessed(sampleCount);
		 m_qSound->Process(fmLeft.data(), fmRight.data(), 
						 fmLeftProcessed.data(), fmRightProcessed.data(), 
						 sampleCount);
		 
		 // Process PCM through QSound
		 std::vector<int16_t> pcmLeftProcessed(sampleCount);
		 std::vector<int16_t> pcmRightProcessed(sampleCount);
		 m_qSound->Process(pcmLeft.data(), pcmRight.data(), 
						 pcmLeftProcessed.data(), pcmRightProcessed.data(), 
						 sampleCount);
		 
		 // Mix processed FM and PCM and interleave into output buffer
		 for (int i = 0; i < sampleCount; i++) {
			 int32_t leftSample = static_cast<int32_t>(fmLeftProcessed[i]) + 
								 static_cast<int32_t>(pcmLeftProcessed[i]);
			 int32_t rightSample = static_cast<int32_t>(fmRightProcessed[i]) + 
								 static_cast<int32_t>(pcmRightProcessed[i]);
			 
			 buffer[i * 2] = static_cast<int16_t>(std::clamp(leftSample, -32768, 32767));
			 buffer[i * 2 + 1] = static_cast<int16_t>(std::clamp(rightSample, -32768, 32767));
		 }
	 } else {
		 // Without QSound, just mix FM and PCM directly
		 for (int i = 0; i < sampleCount * 2; i++) {
			 int32_t sample = static_cast<int32_t>(fmBuffer[i]) + static_cast<int32_t>(pcmBuffer[i]);
			 buffer[i] = static_cast<int16_t>(std::clamp(sample, -32768, 32767));
		 }
	 }
 }
 
 /**
  * Apply master volume and effects
  */
 void AudioSystem::ApplyMasterVolumeAndEffects(int16_t* buffer, int sampleCount) {
	 // Apply master volume
	 float volumeFactor = static_cast<float>(m_masterVolume) / static_cast<float>(MAX_VOLUME);
	 
	 for (int i = 0; i < sampleCount * 2; i++) {
		 int32_t sample = static_cast<int32_t>(static_cast<float>(buffer[i]) * volumeFactor);
		 buffer[i] = static_cast<int16_t>(std::clamp(sample, -32768, 32767));
	 }
 }
 
 /**
  * Set the master volume
  */
 void AudioSystem::SetMasterVolume(uint8_t volume) {
	 m_masterVolume = volume;
	 m_registers.masterVolume = volume;
	 
	 m_logger.Debug("AudioSystem", "Master volume set to " + std::to_string(volume));
 }
 
 /**
  * Get the master volume
  */
 uint8_t AudioSystem::GetMasterVolume() const {
	 return m_masterVolume;
 }
 
 /**
  * Play a sound effect
  */
 int AudioSystem::PlaySoundEffect(uint16_t sampleIndex, const SoundEffectParams& params) {
	 // Find an available PCM channel
	 int pcmChannel = FindAvailablePCMChannel(params.priority);
	 if (pcmChannel < 0) {
		 m_logger.Warning("AudioSystem", "No available PCM channel for sample " + std::to_string(sampleIndex));
		 return -1;
	 }
	 
	 // Start playback
	 bool success = m_pcmPlayer->PlaySample(pcmChannel, sampleIndex, params.frequency,
											params.volume, params.pan, params.loop, params.priority);
	 
	 if (!success) {
		 m_logger.Error("AudioSystem", "Failed to play sample " + std::to_string(sampleIndex));
		 return -1;
	 }
	 
	 // Set loop points if specified
	 if (params.loop) {
		 m_pcmPlayer->SetChannelLoop(pcmChannel, params.loopStart, params.loopEnd, true);
	 }
	 
	 // Assign a channel ID and store channel info
	 int channelId = m_nextChannelId++;
	 
	 ChannelInfo channelInfo;
	 channelInfo.type = SoundChannelType::PCM;
	 channelInfo.internalIndex = pcmChannel;
	 channelInfo.status = m_pcmPlayer->GetChannelInfo(pcmChannel);
	 
	 m_channels[channelId] = channelInfo;
	 
	 m_logger.Debug("AudioSystem", "Sound effect " + std::to_string(sampleIndex) + 
					" started on channel " + std::to_string(channelId));
	 
	 return channelId;
 }
 
 /**
  * Stop a playing sound effect
  */
 bool AudioSystem::StopSoundEffect(int channelId, bool immediate) {
	 ChannelInfo* channel = FindChannelInfo(channelId);
	 if (!channel || channel->type != SoundChannelType::PCM) {
		 m_logger.Warning("AudioSystem", "Invalid channel ID for stopping sound effect: " + std::to_string(channelId));
		 return false;
	 }
	 
	 bool success = m_pcmPlayer->StopChannel(channel->internalIndex, immediate);
	 
	 if (success) {
		 if (immediate) {
			 // If immediate, remove the channel
			 m_channels.erase(channelId);
		 }
		 
		 m_logger.Debug("AudioSystem", "Sound effect stopped on channel " + std::to_string(channelId));
	 } else {
		 m_logger.Warning("AudioSystem", "Failed to stop sound effect on channel " + std::to_string(channelId));
	 }
	 
	 return success;
 }
 
 /**
  * Play an FM note
  */
 int AudioSystem::PlayFMNote(uint8_t instrumentIndex, uint8_t note, uint8_t velocity, uint16_t duration) {
	 // Find an available FM channel
	 int fmChannel = FindAvailableFMChannel(velocity);
	 if (fmChannel < 0) {
		 m_logger.Warning("AudioSystem", "No available FM channel for instrument " + std::to_string(instrumentIndex));
		 return -1;
	 }
	 
	 // Get the instrument
	 const FMInstrument& instrument = m_fmInstruments[instrumentIndex];
	 
	 // Set up the instrument
	 m_ym2151->SetInstrument(fmChannel, instrument.algorithm, instrument.feedback, instrument.operators);
	 
	 // Set note and volume
	 m_ym2151->SetNote(fmChannel, note, 0);
	 uint8_t volume = static_cast<uint8_t>((velocity * 127) / 255);
	 m_ym2151->SetVolume(fmChannel, volume);
	 
	 // Key on
	 m_ym2151->SetKeyOnOff(fmChannel, true);
	 
	 // Assign a channel ID and store channel info
	 int channelId = m_nextChannelId++;
	 
	 ChannelInfo channelInfo;
	 channelInfo.type = SoundChannelType::FM;
	 channelInfo.internalIndex = fmChannel;
	 
	 // Set up status
	 channelInfo.status.active = true;
	 channelInfo.status.volume = velocity;
	 channelInfo.status.frequency = 0;  // Frequency is determined by note
	 channelInfo.status.pan = 128;  // Center
	 channelInfo.status.currentPos = 0;
	 channelInfo.status.priority = velocity;
	 channelInfo.status.note = note;
	 channelInfo.status.keyOn = true;
	 
	 m_channels[channelId] = channelInfo;
	 
	 m_logger.Debug("AudioSystem", "FM note " + std::to_string(note) + 
					" started on channel " + std::to_string(channelId));
	 
	 return channelId;
 }
 
 /**
  * Stop an FM note
  */
 bool AudioSystem::StopFMNote(int channelId, bool immediate) {
	 ChannelInfo* channel = FindChannelInfo(channelId);
	 if (!channel || channel->type != SoundChannelType::FM) {
		 m_logger.Warning("AudioSystem", "Invalid channel ID for stopping FM note: " + std::to_string(channelId));
		 return false;
	 }
	 
	 // Key off or immediately stop
	 m_ym2151->SetKeyOnOff(channel->internalIndex, false);
	 
	 if (immediate) {
		 // If immediate, remove the channel
		 m_channels.erase(channelId);
	 } else {
		 // Otherwise, just update the status
		 channel->status.keyOn = false;
	 }
	 
	 m_logger.Debug("AudioSystem", "FM note stopped on channel " + std::to_string(channelId));
	 
	 return true;
 }
 
 /**
  * Load PCM sample data
  */
 bool AudioSystem::LoadPCMSample(uint16_t index, uint32_t address, uint32_t length, 
								uint16_t frequency, uint8_t bits, bool stereo) {
	 if (index >= m_pcmSamples.size()) {
		 m_logger.Error("AudioSystem", "Sample index out of range: " + std::to_string(index));
		 return false;
	 }
	 
	 bool success = m_pcmPlayer->RegisterSample(index, address, length, bits, stereo, frequency);
	 
	 if (success) {
		 // Update our sample information
		 PCMSample& sample = m_pcmSamples[index];
		 sample.address = address;
		 sample.length = length;
		 sample.frequency = frequency;
		 sample.bits = bits;
		 sample.stereo = stereo;
		 sample.loopStart = 0;
		 sample.loopEnd = 0;
		 sample.loop = false;
		 
		 m_logger.Debug("AudioSystem", "PCM sample " + std::to_string(index) + " loaded");
	 } else {
		 m_logger.Error("AudioSystem", "Failed to load PCM sample " + std::to_string(index));
	 }
	 
	 return success;
 }
 
 /**
  * Set PCM sample loop points
  */
 bool AudioSystem::SetPCMSampleLoop(uint16_t index, uint16_t loopStart, uint16_t loopEnd, bool loop) {
	 if (index >= m_pcmSamples.size()) {
		 m_logger.Error("AudioSystem", "Sample index out of range: " + std::to_string(index));
		 return false;
	 }
	 
	 bool success = m_pcmPlayer->SetSampleLoop(index, loopStart, loopEnd, loop);
	 
	 if (success) {
		 // Update our sample information
		 PCMSample& sample = m_pcmSamples[index];
		 sample.loopStart = loopStart;
		 sample.loopEnd = loopEnd;
		 sample.loop = loop;
		 
		 m_logger.Debug("AudioSystem", "PCM sample " + std::to_string(index) + " loop points set");
	 } else {
		 m_logger.Error("AudioSystem", "Failed to set PCM sample loop points for " + std::to_string(index));
	 }
	 
	 return success;
 }
 
 /**
  * Get PCM sample information
  */
 PCMSample AudioSystem::GetPCMSample(uint16_t index) const {
	 if (index < m_pcmSamples.size()) {
		 return m_pcmSamples[index];
	 }
	 
	 return PCMSample();
 }
 
 /**
  * Define an FM instrument
  */
 bool AudioSystem::DefineFMInstrument(uint8_t index, const FMInstrument& instrument) {
	 if (index >= m_fmInstruments.size()) {
		 m_logger.Error("AudioSystem", "Instrument index out of range: " + std::to_string(index));
		 return false;
	 }
	 
	 m_fmInstruments[index] = instrument;
	 
	 m_logger.Debug("AudioSystem", "FM instrument " + std::to_string(index) + " defined");
	 
	 return true;
 }
 
 /**
  * Get FM instrument definition
  */
 FMInstrument AudioSystem::GetFMInstrument(uint8_t index) const {
	 if (index < m_fmInstruments.size()) {
		 return m_fmInstruments[index];
	 }
	 
	 return FMInstrument();
 }
 
 /**
  * Set channel volume
  */
 bool AudioSystem::SetChannelVolume(int channelId, uint8_t volume) {
	 ChannelInfo* channel = FindChannelInfo(channelId);
	 if (!channel) {
		 m_logger.Warning("AudioSystem", "Invalid channel ID for volume setting: " + std::to_string(channelId));
		 return false;
	 }
	 
	 bool success = false;
	 
	 switch (channel->type) {
		 case SoundChannelType::FM:
			 m_ym2151->SetVolume(channel->internalIndex, volume);
			 success = true;
			 break;
			 
		 case SoundChannelType::PCM:
			 success = m_pcmPlayer->SetChannelVolume(channel->internalIndex, volume);
			 break;
			 
		 case SoundChannelType::QSOUND:
			 if (m_variant == AudioHardwareVariant::NIXX32_PLUS) {
				 success = m_qSound->SetChannelVolume(channel->internalIndex, volume);
			 }
			 break;
			 
		 default:
			 break;
	 }
	 
	 if (success) {
		 channel->status.volume = volume;
		 m_logger.Debug("AudioSystem", "Channel " + std::to_string(channelId) + " volume set to " + std::to_string(volume));
	 } else {
		 m_logger.Warning("AudioSystem", "Failed to set volume for channel " + std::to_string(channelId));
	 }
	 
	 return success;
 }
 
 /**
  * Set channel panning
  */
 bool AudioSystem::SetChannelPan(int channelId, uint8_t pan) {
	 ChannelInfo* channel = FindChannelInfo(channelId);
	 if (!channel) {
		 m_logger.Warning("AudioSystem", "Invalid channel ID for pan setting: " + std::to_string(channelId));
		 return false;
	 }
	 
	 bool success = false;
	 
	 switch (channel->type) {
		 case SoundChannelType::PCM:
			 success = m_pcmPlayer->SetChannelPan(channel->internalIndex, pan);
			 break;
			 
		 case SoundChannelType::QSOUND:
			 if (m_variant == AudioHardwareVariant::NIXX32_PLUS) {
				 // Convert pan (0-255) to QSound X position (-128 to 127)
				 int8_t x = static_cast<int8_t>(pan) - 128;
				 success = m_qSound->SetChannelSpatialPosition(channel->internalIndex, x, 0, 0);
			 }
			 break;
			 
		 case SoundChannelType::FM:
			 // FM doesn't support direct panning in YM2151, so we use QSound if available
			 if (m_variant == AudioHardwareVariant::NIXX32_PLUS && m_config.spatialAudio) {
				 int8_t x = static_cast<int8_t>(pan) - 128;
				 success = m_qSound->SetChannelSpatialPosition(channel->internalIndex, x, 0, 0);
			 } else {
				 // Otherwise, we ignore the pan setting for FM
				 success = true;
			 }
			 break;
			 
		 default:
			 break;
	 }
	 
	 if (success) {
		 channel->status.pan = pan;
		 m_logger.Debug("AudioSystem", "Channel " + std::to_string(channelId) + " pan set to " + std::to_string(pan));
	 } else {
		 m_logger.Warning("AudioSystem", "Failed to set pan for channel " + std::to_string(channelId));
	 }
	 
	 return success;
 }
 
 /**
  * Set channel frequency
  */
 bool AudioSystem::SetChannelFrequency(int channelId, uint16_t frequency) {
	 ChannelInfo* channel = FindChannelInfo(channelId);
	 if (!channel) {
		 m_logger.Warning("AudioSystem", "Invalid channel ID for frequency setting: " + std::to_string(channelId));
		 return false;
	 }
	 
	 bool success = false;
	 
	 switch (channel->type) {
		 case SoundChannelType::PCM:
			 success = m_pcmPlayer->SetChannelFrequency(channel->internalIndex, frequency);
			 break;
			 
		 case SoundChannelType::FM:
			 // For FM, frequency is determined by note, so we ignore direct frequency setting
			 success = true;
			 break;
			 
		 default:
			 break;
	 }
	 
	 if (success) {
		 channel->status.frequency = frequency;
		 m_logger.Debug("AudioSystem", "Channel " + std::to_string(channelId) + " frequency set to " + std::to_string(frequency));
	 } else {
		 m_logger.Warning("AudioSystem", "Failed to set frequency for channel " + std::to_string(channelId));
	 }
	 
	 return success;
 }
 
 /**
  * Get channel status
  */
 ChannelStatus AudioSystem::GetChannelStatus(int channelId) const {
	 auto it = m_channels.find(channelId);
	 if (it != m_channels.end()) {
		 return it->second.status;
	 }
	 
	 // Return empty status for invalid channel
	 ChannelStatus emptyStatus{};
	 emptyStatus.active = false;
	 
	 return emptyStatus;
 }
 
 /**
  * Set spatial position for a channel (Q-Sound)
  */
 bool AudioSystem::SetChannelPosition(int channelId, int8_t x, int8_t y, int8_t z) {
	 // Only available on NiXX-32+ with spatial audio
	 if (m_variant != AudioHardwareVariant::NIXX32_PLUS || !m_config.spatialAudio) {
		 m_logger.Warning("AudioSystem", "Spatial audio not supported on this hardware");
		 return false;
	 }
	 
	 ChannelInfo* channel = FindChannelInfo(channelId);
	 if (!channel) {
		 m_logger.Warning("AudioSystem", "Invalid channel ID for spatial position: " + std::to_string(channelId));
		 return false;
	 }
	 
	 bool success = m_qSound->SetChannelSpatialPosition(channel->internalIndex, x, y, z);
	 
	 if (success) {
		 m_logger.Debug("AudioSystem", "Channel " + std::to_string(channelId) + " spatial position set");
	 } else {
		 m_logger.Warning("AudioSystem", "Failed to set spatial position for channel " + std::to_string(channelId));
	 }
	 
	 return success;
 }
 
 /**
  * Pause or resume audio
  */
 void AudioSystem::SetPaused(bool paused) {
	 m_paused = paused;
	 
	 m_logger.Info("AudioSystem", paused ? "Audio paused" : "Audio resumed");
 }
 
 /**
  * Check if audio is paused
  */
 bool AudioSystem::IsPaused() const {
	 return m_paused;
 }
 
 /**
  * Get YM2151 FM synthesis chip
  */
 YM2151& AudioSystem::GetYM2151() {
	 return *m_ym2151;
 }
 
 /**
  * Get PCM player
  */
 PCMPlayer& AudioSystem::GetPCMPlayer() {
	 return *m_pcmPlayer;
 }
 
 /**
  * Get Q-Sound spatial audio
  */
 QSound& AudioSystem::GetQSound() {
	 return *m_qSound;
 }
 
 /**
  * Handle sound register write
  */
 void AudioSystem::HandleRegisterWrite(uint32_t address, uint8_t value) {
	 switch (address) {
		 case AUDIO_CONTROL_REG:
			 m_registers.control = value;
			 break;
		 
		 case AUDIO_STATUS_REG:
			 // Status register is typically read-only
			 break;
		 
		 case AUDIO_IRQ_ENABLE_REG:
			 m_registers.interruptEnable = value;
			 break;
		 
		 case AUDIO_IRQ_STATUS_REG:
			 // Writing to IRQ status register typically clears interrupts
			 m_registers.interruptStatus &= ~value;
			 break;
		 
		 case AUDIO_MASTER_VOL_REG:
			 SetMasterVolume(value);
			 break;
		 
		 case AUDIO_FM_CONTROL_REG:
			 m_registers.fmControl = value;
			 break;
		 
		 case AUDIO_PCM_CONTROL_REG:
			 m_registers.pcmControl = value;
			 break;
		 
		 case AUDIO_QSOUND_CONTROL_REG:
			 m_registers.qsoundControl = value;
			 break;
		 
		 default:
			 m_logger.Warning("AudioSystem", "Write to unknown audio register: " + std::to_string(address));
			 break;
	 }
 }
 
 /**
  * Read from sound register
  */
 uint8_t AudioSystem::HandleRegisterRead(uint32_t address) {
	 switch (address) {
		 case AUDIO_CONTROL_REG:
			 return m_registers.control;
		 
		 case AUDIO_STATUS_REG:
			 return m_registers.status;
		 
		 case AUDIO_IRQ_ENABLE_REG:
			 return m_registers.interruptEnable;
		 
		 case AUDIO_IRQ_STATUS_REG:
			 return m_registers.interruptStatus;
		 
		 case AUDIO_MASTER_VOL_REG:
			 return m_registers.masterVolume;
		 
		 case AUDIO_FM_CONTROL_REG:
			 return m_registers.fmControl;
		 
		 case AUDIO_PCM_CONTROL_REG:
			 return m_registers.pcmControl;
		 
		 case AUDIO_QSOUND_CONTROL_REG:
			 return m_registers.qsoundControl;
		 
		 default:
			 m_logger.Warning("AudioSystem", "Read from unknown audio register: " + std::to_string(address));
			 return 0xFF;
	 }
 }
 
 /**
  * Register a Z80 port handler
  */
 bool AudioSystem::RegisterPortHandler(uint8_t port, 
									  std::function<uint8_t()> readHandler,
									  std::function<void(uint8_t)> writeHandler) {
	 if (m_portReadHandlers.find(port) != m_portReadHandlers.end() ||
		 m_portWriteHandlers.find(port) != m_portWriteHandlers.end()) {
		 m_logger.Warning("AudioSystem", "Port " + std::to_string(port) + " already has handlers");
		 return false;
	 }
	 
	 m_portReadHandlers[port] = readHandler;
	 m_portWriteHandlers[port] = writeHandler;
	 
	 if (m_audioCPU) {
		 // Register with Z80 CPU
		 m_audioCPU->RegisterPortHooks(port, readHandler, writeHandler);
	 }
	 
	 m_logger.Debug("AudioSystem", "Port handlers registered for port " + std::to_string(port));
	 
	 return true;
 }
 
 /**
  * Update from sound RAM
  */
 void AudioSystem::UpdateFromSoundRAM() {
	 // This method would be called periodically to update audio components
	 // based on changes to sound RAM made by the main CPU or audio CPU
	 
	 // For a real implementation, we would:
	 // 1. Check for changes in FM registers
	 // 2. Check for changes in PCM registers
	 // 3. Check for changes in QSound registers (if supported)
	 
	 // For now, just log that update was called
	 m_logger.Debug("AudioSystem", "UpdateFromSoundRAM called");
 }
 
 /**
  * Get the audio configuration
  */
 const AudioConfig& AudioSystem::GetConfig() const {
	 return m_config;
 }
 
 /**
  * Get the hardware variant
  */
 AudioHardwareVariant AudioSystem::GetVariant() const {
	 return m_variant;
 }
 
 /**
  * Set audio CPU
  */
 void AudioSystem::SetAudioCPU(Z80CPU* cpu) {
	 m_audioCPU = cpu;
	 
	 // Register port handlers with the CPU
	 if (m_audioCPU) {
		 for (const auto& handler : m_portReadHandlers) {
			 uint8_t port = handler.first;
			 m_audioCPU->RegisterPortHooks(port, m_portReadHandlers[port], m_portWriteHandlers[port]);
		 }
		 
		 m_logger.Info("AudioSystem", "Audio CPU set and port handlers registered");
	 } else {
		 m_logger.Warning("AudioSystem", "Audio CPU set to nullptr");
	 }
 }
 
 // Private helper methods
 
 /**
  * Find an available FM channel
  */
 int AudioSystem::FindAvailableFMChannel(uint8_t priority) {
	 int lowestPriorityChannel = -1;
	 uint8_t lowestPriority = 255;
	 
	 // Check for empty channels
	 for (int i = 0; i < m_config.fmChannels; i++) {
		 bool found = false;
		 
		 for (const auto& pair : m_channels) {
			 const ChannelInfo& info = pair.second;
			 if (info.type == SoundChannelType::FM && info.internalIndex == i) {
				 // Channel is in use
				 found = true;
				 
				 // Track the lowest priority channel in case we need to steal one
				 if (info.status.priority < lowestPriority) {
					 lowestPriority = info.status.priority;
					 lowestPriorityChannel = i;
				 }
				 
				 break;
			 }
		 }
		 
		 if (!found) {
			 // Found unused channel
			 return i;
		 }
	 }
	 
	 // No empty channels, check if we can steal one based on priority
	 if (lowestPriorityChannel >= 0 && lowestPriority < priority) {
		 // Find the channel ID for this internal index
		 for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
			 const ChannelInfo& info = it->second;
			 if (info.type == SoundChannelType::FM && info.internalIndex == lowestPriorityChannel) {
				 // Stop the channel and remove it
				 m_ym2151->SetKeyOnOff(lowestPriorityChannel, false);
				 m_channels.erase(it);
				 break;
			 }
		 }
		 
		 return lowestPriorityChannel;
	 }
	 
	 // No available channel
	 return -1;
 }
 
 /**
  * Find an available PCM channel
  */
 int AudioSystem::FindAvailablePCMChannel(uint8_t priority) {
	 // First check for inactive channels
	 for (int i = 0; i < m_config.pcmChannels; i++) {
		 if (!m_pcmPlayer->IsChannelActive(i)) {
			 return i;
		 }
	 }
	 
	 // If no inactive channels, try to find one to steal based on priority
	 return m_pcmPlayer->FindChannelToSteal(priority);
 }
 
 /**
  * Find channel info by ID
  */
 AudioSystem::ChannelInfo* AudioSystem::FindChannelInfo(int channelId) {
	 auto it = m_channels.find(channelId);
	 if (it != m_channels.end()) {
		 return &it->second;
	 }
	 
	 return nullptr;
 }
 
 /**
  * Update channel status
  */
 void AudioSystem::UpdateChannelStatus(int channelId) {
	 ChannelInfo* channel = FindChannelInfo(channelId);
	 if (!channel) {
		 return;
	 }
	 
	 switch (channel->type) {
		 case SoundChannelType::FM: {
			 // For FM channels, check the current output to determine if still active
			 int16_t output = m_ym2151->GetChannelOutput(channel->internalIndex);
			 
			 // If key is off and output is close to zero, consider the channel inactive
			 if (!channel->status.keyOn && std::abs(output) < 10) {
				 channel->status.active = false;
			 }
			 
			 // Update frequency information
			 channel->status.frequency = static_cast<uint16_t>(
				 m_ym2151->GetChannelFrequency(channel->internalIndex));
			 
			 break;
		 }
			 
		 case SoundChannelType::PCM:
			 // Update from PCM player
			 channel->status = m_pcmPlayer->GetChannelInfo(channel->internalIndex);
			 break;
			 
		 case SoundChannelType::QSOUND:
			 // QSound channels are controlled by the source channels
			 channel->status.active = m_qSound->IsChannelActive(channel->internalIndex);
			 break;
			 
		 case SoundChannelType::NOISE:
			 // Noise generator channels (if implemented)
			 // For now, just keep the current status
			 break;
	 }
 }
 
 /**
  * Handle interrupts
  */
 void AudioSystem::HandleInterrupts() {
	 // Check for YM2151 timer interrupts
	 if (m_ym2151->IsTimerExpired(true) || m_ym2151->IsTimerExpired(false)) {
		 m_registers.interruptStatus |= 0x01;  // Set FM interrupt bit
		 
		 // Trigger interrupt on audio CPU if enabled
		 if ((m_registers.interruptEnable & 0x01) && m_audioCPU) {
			 m_audioCPU->TriggerInterrupt(Z80InterruptType::INT, 0xFF);
		 }
	 }
	 
	 // Check for PCM player interrupts (e.g., sample end, loop point)
	 for (int i = 0; i < m_config.pcmChannels; i++) {
		 if (m_pcmPlayer->IsChannelActive(i)) {
			 PCMPlaybackInfo info = m_pcmPlayer->GetChannelInfo(i);
			 
			 // Check if sample has reached end or loop point
			 if (info.position >= info.length || 
				 (info.loop && info.position >= info.loopEnd)) {
				 
				 m_registers.interruptStatus |= 0x02;  // Set PCM interrupt bit
				 
				 // Trigger interrupt on audio CPU if enabled
				 if ((m_registers.interruptEnable & 0x02) && m_audioCPU) {
					 m_audioCPU->TriggerInterrupt(Z80InterruptType::INT, 0xFF);
				 }
				 
				 break;  // One interrupt is enough
			 }
		 }
	 }
	 
	 // Update status register
	 m_registers.status = m_registers.interruptStatus;
 }
 
 } // namespace NiXX32