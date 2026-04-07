#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio/AudioEngine.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>

void audioCallback(void* pDevice, void* pOutput, const void* /*pInput*/, unsigned int frameCount) {
    ma_device* dev = static_cast<ma_device*>(pDevice);
    AudioEngine* engine = static_cast<AudioEngine*>(dev->pUserData);
    float* output = static_cast<float*>(pOutput);

    std::memset(output, 0, frameCount * 2 * sizeof(float)); // stereo

    std::lock_guard<std::mutex> lock(engine->m_voiceMutex);

    for (auto& voice : engine->m_voices) {
        if (!voice.active) continue;

        for (unsigned int i = 0; i < frameCount; i++) {
            if (voice.cursor >= voice.length) {
                voice.active = false;
                break;
            }
            float sample = voice.data[voice.cursor] * voice.volume * engine->masterVolume;
            output[i * 2 + 0] += sample; // left
            output[i * 2 + 1] += sample; // right
            voice.cursor++;
        }
    }

    // Clamp output
    for (unsigned int i = 0; i < frameCount * 2; i++) {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::init() {
    if (m_initialized) return true;

    m_device = new ma_device;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.dataCallback = reinterpret_cast<ma_device_data_proc>(audioCallback);
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, m_device) != MA_SUCCESS) {
        std::cerr << "[Audio] Failed to initialize audio device\n";
        delete m_device;
        m_device = nullptr;
        return false;
    }

    if (ma_device_start(m_device) != MA_SUCCESS) {
        std::cerr << "[Audio] Failed to start audio device\n";
        ma_device_uninit(m_device);
        delete m_device;
        m_device = nullptr;
        return false;
    }

    generateSounds();
    m_initialized = true;
    std::cout << "[Audio] Initialized successfully\n";
    return true;
}

void AudioEngine::shutdown() {
    if (!m_initialized) return;
    if (m_device) {
        ma_device_uninit(m_device);
        delete m_device;
        m_device = nullptr;
    }
    m_initialized = false;
}

void AudioEngine::play(SFX sound, float volume) {
    if (!m_initialized) return;
    int idx = static_cast<int>(sound);
    if (idx < 0 || idx >= static_cast<int>(SFX::COUNT)) return;
    if (m_soundBuffers[idx].empty()) return;

    std::lock_guard<std::mutex> lock(m_voiceMutex);
    for (auto& voice : m_voices) {
        if (!voice.active) {
            voice.data = m_soundBuffers[idx].data();
            voice.length = static_cast<int>(m_soundBuffers[idx].size());
            voice.cursor = 0;
            voice.volume = volume;
            voice.active = true;
            return;
        }
    }
    // All voices busy — skip
}

void AudioEngine::play3D(SFX sound, const glm::vec3& soundPos, float maxDist) {
    if (!m_initialized) return;
    float dist = glm::length(soundPos - m_listenerPos);
    if (dist > maxDist) return;

    // Distance attenuation (inverse distance, clamped)
    float attenuation = 1.0f / (1.0f + dist * 0.15f);
    play(sound, attenuation);
}

// ---------- Procedural Sound Generation ----------

void AudioEngine::generateTone(std::vector<float>& buf, float freq, float duration, float decay) {
    int samples = static_cast<int>(AUDIO_SAMPLE_RATE * duration);
    buf.resize(samples);
    for (int i = 0; i < samples; i++) {
        float t = static_cast<float>(i) / AUDIO_SAMPLE_RATE;
        float envelope = std::exp(-decay * t);
        buf[i] = std::sin(2.0f * 3.14159265f * freq * t) * envelope * 0.5f;
    }
}

void AudioEngine::generateChirp(std::vector<float>& buf, float freqStart, float freqEnd,
                                  float duration, float decay) {
    int samples = static_cast<int>(AUDIO_SAMPLE_RATE * duration);
    buf.resize(samples);
    float phase = 0.0f;
    for (int i = 0; i < samples; i++) {
        float t = static_cast<float>(i) / AUDIO_SAMPLE_RATE;
        float frac = t / duration;
        float freq = freqStart + (freqEnd - freqStart) * frac;
        float envelope = std::exp(-decay * t);
        phase += 2.0f * 3.14159265f * freq / AUDIO_SAMPLE_RATE;
        buf[i] = std::sin(phase) * envelope * 0.4f;
    }
}

void AudioEngine::generateNoise(std::vector<float>& buf, float duration, float decay) {
    int samples = static_cast<int>(AUDIO_SAMPLE_RATE * duration);
    buf.resize(samples);
    for (int i = 0; i < samples; i++) {
        float t = static_cast<float>(i) / AUDIO_SAMPLE_RATE;
        float envelope = std::exp(-decay * t);
        float noise = (static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f);
        buf[i] = noise * envelope * 0.2f;
    }
}

void AudioEngine::addToBuffer(std::vector<float>& dest, const std::vector<float>& src, float volume) {
    if (src.size() > dest.size()) dest.resize(src.size(), 0.0f);
    for (size_t i = 0; i < src.size(); i++) {
        dest[i] += src[i] * volume;
    }
}

void AudioEngine::generateSounds() {
    // Jump: quick rising chirp
    generateChirp(m_soundBuffers[static_cast<int>(SFX::Jump)],
                  300.0f, 800.0f, 0.12f, 8.0f);

    // Land: low thump + noise
    {
        auto& buf = m_soundBuffers[static_cast<int>(SFX::Land)];
        std::vector<float> thump, noise;
        generateTone(thump, 80.0f, 0.08f, 15.0f);
        generateNoise(noise, 0.05f, 20.0f);
        buf = thump;
        addToBuffer(buf, noise, 0.5f);
    }

    // Death: descending sweep + noise
    {
        auto& buf = m_soundBuffers[static_cast<int>(SFX::Death)];
        std::vector<float> sweep, noise;
        generateChirp(sweep, 600.0f, 100.0f, 0.5f, 3.0f);
        generateNoise(noise, 0.3f, 5.0f);
        buf = sweep;
        addToBuffer(buf, noise, 0.4f);
    }

    // Checkpoint: two-note chime (A5 + E6)
    {
        auto& buf = m_soundBuffers[static_cast<int>(SFX::Checkpoint)];
        std::vector<float> note1, note2;
        generateTone(note1, 880.0f, 0.2f, 4.0f);
        generateTone(note2, 1320.0f, 0.3f, 3.0f);
        buf = note1;
        // Offset note2 by ~0.1 seconds
        int offset = AUDIO_SAMPLE_RATE / 10;
        if (buf.size() < note2.size() + offset) buf.resize(note2.size() + offset, 0.0f);
        for (size_t i = 0; i < note2.size(); i++) {
            buf[i + offset] += note2[i];
        }
    }

    // Level Complete: major chord arpeggio (C5, E5, G5, C6)
    {
        auto& buf = m_soundBuffers[static_cast<int>(SFX::LevelComplete)];
        float freqs[] = {523.0f, 659.0f, 784.0f, 1047.0f};
        buf.clear();
        for (int n = 0; n < 4; n++) {
            std::vector<float> note;
            generateTone(note, freqs[n], 0.3f, 2.5f);
            int offset = n * AUDIO_SAMPLE_RATE / 8; // stagger by 0.125s
            if (buf.size() < note.size() + offset) buf.resize(note.size() + offset, 0.0f);
            for (size_t i = 0; i < note.size(); i++) {
                buf[i + offset] += note[i] * 0.6f;
            }
        }
    }

    // Menu Click: short blip
    generateTone(m_soundBuffers[static_cast<int>(SFX::MenuClick)],
                 1000.0f, 0.03f, 30.0f);
}
