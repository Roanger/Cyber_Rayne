#include "../../include/AudioManager.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace {

float clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

// First existing <folder>/<name><ext>, trying extensions in order.
std::string findFile(const std::string& folder, const std::string& name,
                     std::initializer_list<const char*> extensions) {
    for (const char* ext : extensions) {
        const std::string path = folder + "/" + name + ext;
        std::error_code ignored;
        if (std::filesystem::is_regular_file(path, ignored)) {
            return path;
        }
    }
    return {};
}

}  // namespace

AudioManager::AudioManager()
    : m_initialized(false), m_masterVolume(1.0f), m_musicVolume(0.7f),
      m_sfxVolume(1.0f), m_muted(false) {}

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::initialize(const std::string& assetsPath, std::unique_ptr<AudioBackend> backend) {
    m_assetsPath = assetsPath;
    m_backend = std::move(backend);
    m_initialized = true;
    std::cout << "Audio: " << (m_backend ? "sound device ready." : "no sound device; running silent.")
              << std::endl;
    return true;
}

void AudioManager::shutdown() {
    if (m_backend) {
        m_backend->stopMusic();
    }
    m_backend.reset();
    m_currentMusic.clear();
    m_sinceSfx.clear();
    m_initialized = false;
}

void AudioManager::update(float deltaTime) {
    for (auto& [name, since] : m_sinceSfx) {
        since += deltaTime;
    }
}

std::string AudioManager::findMusicFile(const std::string& name) const {
    return findFile(m_assetsPath + "/audio/music", name, {".ogg", ".mp3", ".flac", ".wav"});
}

std::string AudioManager::findSfxFile(const std::string& name) const {
    return findFile(m_assetsPath + "/audio/sfx", name, {".wav", ".ogg", ".mp3", ".flac"});
}

void AudioManager::noteMissing(const std::string& kind, const std::string& name) {
    if (m_reportedMissing.insert(kind + "/" + name).second) {
        std::cout << "Audio: no file yet for " << kind << " '" << name << "'." << std::endl;
    }
}

void AudioManager::setMusicFallback(const std::string& name, const std::string& standIn) {
    m_musicFallbacks[name] = standIn;
}

float AudioManager::effectiveMusicVolume() const {
    return m_muted ? 0.0f : m_masterVolume * m_musicVolume * (m_ducked ? DUCKED_SCALE : 1.0f);
}

void AudioManager::playMusic(const std::string& name) {
    startMusic(name, true);
}

void AudioManager::playMusicOnce(const std::string& name) {
    startMusic(name, false);
}

void AudioManager::startMusic(const std::string& name, bool loop) {
    if (!m_initialized || name == m_currentMusic) {
        return;
    }
    // Two names that resolve to the same file (a district borrowing another's
    // track) should not restart it. Compare files, not names.
    auto resolve = [this](const std::string& track) {
        std::string path = findMusicFile(track);
        if (path.empty()) {
            auto standIn = m_musicFallbacks.find(track);
            if (standIn != m_musicFallbacks.end()) {
                path = findMusicFile(standIn->second);
            }
        }
        return path;
    };
    const std::string previous = m_currentMusic.empty() ? std::string() : resolve(m_currentMusic);
    const std::string path = resolve(name);
    m_currentMusic = name;

    if (path.empty()) {
        noteMissing("music", name);
        if (m_backend) m_backend->stopMusic();
        return;
    }
    if (path == previous && loop) {
        return;  // already playing this file
    }
    if (m_backend && !m_backend->playMusic(path, loop, effectiveMusicVolume())) {
        std::cerr << "Audio: could not play " << path << std::endl;
    }
}

void AudioManager::stopMusic() {
    m_currentMusic.clear();
    if (m_backend) {
        m_backend->stopMusic();
    }
}

void AudioManager::playSfx(const std::string& name) {
    if (!m_initialized || m_muted) {
        return;
    }
    auto since = m_sinceSfx.find(name);
    if (since != m_sinceSfx.end() && since->second < SFX_REPEAT_GUARD) {
        return;
    }
    m_sinceSfx[name] = 0.0f;

    const std::string path = findSfxFile(name);
    if (path.empty()) {
        noteMissing("sound", name);
        return;
    }
    if (m_backend && !m_backend->playSfx(path, m_masterVolume * m_sfxVolume)) {
        std::cerr << "Audio: could not play " << path << std::endl;
    }
}

void AudioManager::setMasterVolume(float volume) {
    m_masterVolume = clamp01(volume);
    if (m_backend) m_backend->setMusicVolume(effectiveMusicVolume());
}

void AudioManager::setMusicVolume(float volume) {
    m_musicVolume = clamp01(volume);
    if (m_backend) m_backend->setMusicVolume(effectiveMusicVolume());
}

void AudioManager::setSFXVolume(float volume) {
    m_sfxVolume = clamp01(volume);
}

void AudioManager::setMusicDucked(bool ducked) {
    m_ducked = ducked;
    if (m_backend) m_backend->setMusicVolume(effectiveMusicVolume());
}

void AudioManager::setMuted(bool muted) {
    m_muted = muted;
    if (m_backend) m_backend->setMusicVolume(effectiveMusicVolume());
}
