// The real sound backend: miniaudio (third_party/miniaudio.h), a single-file
// library that talks to PipeWire/PulseAudio/ALSA on Linux and WASAPI on
// Windows with nothing extra to install. Ogg files are decoded by stb_vorbis.
//
// This file is compiled with warnings off (see CMakeLists.txt): it pulls in
// two large third-party sources that were not written for -Wall -Wextra.

#define STB_VORBIS_HEADER_ONLY
#include "../../third_party/stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#include "../../third_party/miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "../../third_party/stb_vorbis.c"

#include "../../include/AudioManager.h"

#include <iostream>
#include <list>

namespace {

class MiniaudioBackend : public AudioBackend {
public:
    ~MiniaudioBackend() override {
        if (!m_ready) return;
        stopMusic();
        for (ma_sound& sound : m_voices) {
            ma_sound_uninit(&sound);
        }
        ma_engine_uninit(&m_engine);
    }

    bool open() {
        if (ma_engine_init(nullptr, &m_engine) != MA_SUCCESS) {
            return false;
        }
        m_ready = true;
        return true;
    }

    bool playMusic(const std::string& path, bool loop, float volume) override {
        stopMusic();
        // Streamed: a music track is too long to decode into memory up front.
        if (ma_sound_init_from_file(&m_engine, path.c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr,
                                    &m_music) != MA_SUCCESS) {
            return false;
        }
        m_hasMusic = true;
        ma_sound_set_looping(&m_music, loop ? MA_TRUE : MA_FALSE);
        ma_sound_set_volume(&m_music, volume);
        // A short fade in, so a change of track does not start with a jolt.
        ma_sound_set_fade_in_milliseconds(&m_music, 0.0f, volume, 250);
        return ma_sound_start(&m_music) == MA_SUCCESS;
    }

    void stopMusic() override {
        if (m_hasMusic) {
            ma_sound_stop(&m_music);
            ma_sound_uninit(&m_music);
            m_hasMusic = false;
        }
    }

    void setMusicVolume(float volume) override {
        if (m_hasMusic) {
            ma_sound_set_volume(&m_music, volume);
        }
    }

    bool playSfx(const std::string& path, float volume) override {
        // Drop voices that have finished before adding another.
        for (auto it = m_voices.begin(); it != m_voices.end();) {
            if (ma_sound_at_end(&*it)) {
                ma_sound_uninit(&*it);
                it = m_voices.erase(it);
            } else {
                ++it;
            }
        }
        if (m_voices.size() >= MAX_VOICES) {
            return true;  // plenty playing already; skipping one is inaudible
        }
        // std::list: a playing ma_sound must not move in memory.
        m_voices.emplace_back();
        ma_sound& voice = m_voices.back();
        // Decoded fully and cached by miniaudio, so repeats start instantly.
        if (ma_sound_init_from_file(&m_engine, path.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr,
                                    &voice) != MA_SUCCESS) {
            m_voices.pop_back();
            return false;
        }
        ma_sound_set_volume(&voice, volume);
        return ma_sound_start(&voice) == MA_SUCCESS;
    }

private:
    static constexpr std::size_t MAX_VOICES = 24;

    ma_engine m_engine{};
    bool m_ready = false;
    ma_sound m_music{};
    bool m_hasMusic = false;
    std::list<ma_sound> m_voices;
};

}  // namespace

std::unique_ptr<AudioBackend> createDefaultAudioBackend() {
    auto backend = std::make_unique<MiniaudioBackend>();
    if (!backend->open()) {
        std::cerr << "Audio: could not open a sound device." << std::endl;
        return nullptr;
    }
    return backend;
}
