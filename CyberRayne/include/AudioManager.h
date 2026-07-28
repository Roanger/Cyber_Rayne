#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// AudioManager — stub-based audio system with no external dependencies.
// Provides a clean interface for music and SFX that can be backed by
// a real audio library (e.g. miniaudio) later. All operations are
// CI-safe no-ops when no audio backend is loaded.
class AudioManager {
public:
    enum class ChannelType {
        MUSIC,
        SFX
    };

    AudioManager();
    ~AudioManager();

    // Lifecycle
    bool initialize();
    void shutdown();

    // Sound registration — returns a sound ID, or -1 on failure.
    int registerSound(const std::string& name, const std::string& filepath, ChannelType type);

    // Playback control
    void playSound(int soundId);
    void stopSound(int soundId);
    void stopAllSounds();
    void stopMusic();
    void stopAllSFX();

    // Volume control (0.0 to 1.0)
    void setMasterVolume(float volume);
    void setMusicVolume(float volume);
    void setSFXVolume(float volume);
    float getMasterVolume() const { return m_masterVolume; }
    float getMusicVolume() const { return m_musicVolume; }
    float getSFXVolume() const { return m_sfxVolume; }

    // Mute controls
    void setMuted(bool muted);
    bool isMuted() const { return m_muted; }

    // Query
    bool isPlaying(int soundId) const;
    int getSoundId(const std::string& name) const;
    int getSoundCount() const { return static_cast<int>(m_sounds.size()); }
    bool isInitialized() const { return m_initialized; }

private:
    struct SoundEntry {
        int id;
        std::string name;
        std::string filepath;
        ChannelType type;
        bool playing;
    };

    bool m_initialized;
    float m_masterVolume;
    float m_musicVolume;
    float m_sfxVolume;
    bool m_muted;
    int m_nextId;
    std::vector<SoundEntry> m_sounds;
    std::unordered_map<std::string, int> m_nameToId;
};
