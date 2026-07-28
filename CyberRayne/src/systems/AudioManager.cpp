#include "../../include/AudioManager.h"
#include <algorithm>
#include <iostream>

AudioManager::AudioManager()
    : m_initialized(false), m_masterVolume(1.0f), m_musicVolume(0.8f),
      m_sfxVolume(1.0f), m_muted(false), m_nextId(0) {}

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::initialize() {
    std::cout << "AudioManager initialized (stub mode)." << std::endl;
    m_initialized = true;
    return true;
}

void AudioManager::shutdown() {
    stopAllSounds();
    m_sounds.clear();
    m_nameToId.clear();
    m_initialized = false;
}

int AudioManager::registerSound(const std::string& name, const std::string& filepath, ChannelType type) {
    if (!m_initialized) {
        std::cerr << "AudioManager not initialized." << std::endl;
        return -1;
    }

    int id = m_nextId++;
    m_sounds.push_back({id, name, filepath, type, false});
    m_nameToId[name] = id;
    return id;
}

void AudioManager::playSound(int soundId) {
    if (m_muted || !m_initialized) return;
    for (auto& sound : m_sounds) {
        if (sound.id == soundId) {
            sound.playing = true;
            return;
        }
    }
}

void AudioManager::stopSound(int soundId) {
    for (auto& sound : m_sounds) {
        if (sound.id == soundId) {
            sound.playing = false;
            return;
        }
    }
}

void AudioManager::stopAllSounds() {
    for (auto& sound : m_sounds) {
        sound.playing = false;
    }
}

void AudioManager::stopMusic() {
    for (auto& sound : m_sounds) {
        if (sound.type == ChannelType::MUSIC) {
            sound.playing = false;
        }
    }
}

void AudioManager::stopAllSFX() {
    for (auto& sound : m_sounds) {
        if (sound.type == ChannelType::SFX) {
            sound.playing = false;
        }
    }
}

void AudioManager::setMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::setMusicVolume(float volume) {
    m_musicVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::setSFXVolume(float volume) {
    m_sfxVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::setMuted(bool muted) {
    m_muted = muted;
    if (muted) {
        stopAllSounds();
    }
}

bool AudioManager::isPlaying(int soundId) const {
    for (const auto& sound : m_sounds) {
        if (sound.id == soundId) {
            return sound.playing;
        }
    }
    return false;
}

int AudioManager::getSoundId(const std::string& name) const {
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end()) {
        return it->second;
    }
    return -1;
}
