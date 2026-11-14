#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// What actually makes noise. The game uses the miniaudio one
// (AudioBackendMiniaudio.cpp); tests and machines without a sound device use
// none at all, and everything still runs silently.
class AudioBackend {
public:
    virtual ~AudioBackend() = default;
    // Start a music file, replacing whatever music is playing.
    virtual bool playMusic(const std::string& path, bool loop, float volume) = 0;
    virtual void stopMusic() = 0;
    virtual void setMusicVolume(float volume) = 0;
    // Fire a short sound; any number may overlap.
    virtual bool playSfx(const std::string& path, float volume) = 0;
};

// Music and sound effects by name. The game says playMusic("battle") or
// playSfx("gunshot"); this finds the file, remembers what is playing, and
// applies volume. A sound whose file does not exist yet is simply silent, so
// the game can ask for every sound from day one and each starts working the
// moment its file is dropped in.
//
//   music:  <assets>/audio/music/<name>.ogg   (also .mp3, .flac, .wav)
//   sounds: <assets>/audio/sfx/<name>.wav     (also .ogg, .mp3, .flac)
//
// Gameplay logic only: no audio library is included here.
class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // assetsPath is the game's assets folder. backend may be null (silent).
    bool initialize(const std::string& assetsPath = "assets", std::unique_ptr<AudioBackend> backend = nullptr);
    void shutdown();
    bool isInitialized() const { return m_initialized; }
    void setAssetsPath(const std::string& assetsPath) { m_assetsPath = assetsPath; }

    // Call once per frame.
    void update(float deltaTime);

    // Switch to a looping track. Asking for the track already playing does
    // nothing, so this can be called every frame. If the track has no file,
    // its stand-in is used (see setMusicFallback); with no file at all the
    // music stops.
    void playMusic(const std::string& name);
    // Play a track once, without looping (fanfares, game over).
    void playMusicOnce(const std::string& name);
    void stopMusic();
    // The track last asked for, or empty.
    const std::string& getCurrentMusic() const { return m_currentMusic; }

    // "If <name> has no file, play <standIn> instead."
    void setMusicFallback(const std::string& name, const std::string& standIn);

    // Play a sound effect. The same sound asked for again within a moment is
    // dropped, so three party members healed at once make one sound, not a
    // three-times-louder one.
    void playSfx(const std::string& name);

    // Volume control (0.0 to 1.0)
    void setMasterVolume(float volume);
    void setMusicVolume(float volume);
    void setSFXVolume(float volume);
    float getMasterVolume() const { return m_masterVolume; }
    float getMusicVolume() const { return m_musicVolume; }
    float getSFXVolume() const { return m_sfxVolume; }

    // Ducked: music drops to a murmur (the pause menu), sound effects carry on.
    void setMusicDucked(bool ducked);
    bool isMusicDucked() const { return m_ducked; }
    static constexpr float DUCKED_SCALE = 0.35f;

    // Muted: music is silenced and sound effects are not played.
    void setMuted(bool muted);
    bool isMuted() const { return m_muted; }

    // Where a sound's file is, or empty if there is none. Public for tests
    // and for reporting what is missing.
    std::string findMusicFile(const std::string& name) const;
    std::string findSfxFile(const std::string& name) const;

    // How soon the same sound effect may play again, in seconds.
    static constexpr float SFX_REPEAT_GUARD = 0.05f;

private:
    void startMusic(const std::string& name, bool loop);
    float effectiveMusicVolume() const;
    void noteMissing(const std::string& kind, const std::string& name);

    bool m_initialized;
    std::string m_assetsPath;
    std::unique_ptr<AudioBackend> m_backend;

    float m_masterVolume;
    float m_musicVolume;
    float m_sfxVolume;
    bool m_muted;
    bool m_ducked = false;

    std::string m_currentMusic;
    std::unordered_map<std::string, std::string> m_musicFallbacks;
    // Seconds since each sound effect last played.
    std::unordered_map<std::string, float> m_sinceSfx;
    // Missing files already reported, so the log says each once.
    std::unordered_set<std::string> m_reportedMissing;
};

// The real backend, when the game was built with one. Returns null if the
// build has no audio library or no sound device could be opened.
std::unique_ptr<AudioBackend> createDefaultAudioBackend();
