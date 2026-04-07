#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <mutex>
#include <cmath>

// Forward declare miniaudio types
struct ma_device;

enum class SFX {
    Jump,
    Land,
    Death,
    Checkpoint,
    LevelComplete,
    MenuClick,
    COUNT
};

constexpr int AUDIO_SAMPLE_RATE = 44100;
constexpr int MAX_VOICES = 16;

struct Voice {
    const float* data = nullptr;
    int length = 0;
    int cursor = 0;
    float volume = 1.0f;
    bool active = false;
};

class AudioEngine {
public:
    AudioEngine() = default;
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool init();
    void shutdown();

    // Play a sound effect. Pass position for 3D spatial audio.
    void play(SFX sound, float volume = 1.0f);
    void play3D(SFX sound, const glm::vec3& soundPos, float maxDist = 50.0f);

    void setListenerPos(const glm::vec3& pos) { m_listenerPos = pos; }

    // Master volume (0.0 to 1.0)
    float masterVolume = 0.8f;

private:
    friend void audioCallback(void* pDevice, void* pOutput, const void* pInput, unsigned int frameCount);

    void generateSounds();
    void generateTone(std::vector<float>& buf, float freq, float duration, float decay);
    void generateChirp(std::vector<float>& buf, float freqStart, float freqEnd, float duration, float decay);
    void generateNoise(std::vector<float>& buf, float duration, float decay);
    void addToBuffer(std::vector<float>& dest, const std::vector<float>& src, float volume = 1.0f);

    ma_device* m_device = nullptr;
    bool m_initialized = false;
    glm::vec3 m_listenerPos{0.0f};

    std::vector<float> m_soundBuffers[static_cast<int>(SFX::COUNT)];
    Voice m_voices[MAX_VOICES];
    std::mutex m_voiceMutex;
};
