#include <string>
#include <memory>
#include <fstream>
#include <filesystem>
#include <cmath>
#include "../../include/AudioManager.h"
#include "../../include/InputConfig.h"
#include "../../include/InputPrompts.h"
#include "../../include/InputState.h"
#include <vector>
#include "../../include/InputAction.h"
#include <iostream>
#include <stdexcept>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
} // namespace

int main() {
    // === AudioManager tests ===
    {
        // A scratch assets folder with a few empty "sound files": the manager
        // only needs them to exist.
        const std::filesystem::path assets =
            std::filesystem::temp_directory_path() / "cyberrayne_audio_test";
        std::filesystem::remove_all(assets);
        std::filesystem::create_directories(assets / "audio" / "music");
        std::filesystem::create_directories(assets / "audio" / "sfx");
        auto touch = [&](const char* relative) { std::ofstream(assets / relative) << "x"; };
        touch("audio/music/central.ogg");
        touch("audio/music/battle.mp3");
        touch("audio/sfx/heal.wav");

        // Records what the manager asks the sound device to do.
        struct Log {
            std::vector<std::string> music;
            std::vector<std::string> sfx;
            std::vector<bool> loops;
            int stops = 0;
            float musicVolume = -1.0f;
            float lastSfxVolume = -1.0f;
        };
        struct FakeBackend : AudioBackend {
            explicit FakeBackend(Log* log) : log(log) {}
            bool playMusic(const std::string& path, bool loop, float volume) override {
                log->music.push_back(std::filesystem::path(path).filename().string());
                log->loops.push_back(loop);
                log->musicVolume = volume;
                return true;
            }
            void stopMusic() override { ++log->stops; }
            void setMusicVolume(float volume) override { log->musicVolume = volume; }
            bool playSfx(const std::string& path, float volume) override {
                log->sfx.push_back(std::filesystem::path(path).filename().string());
                log->lastSfxVolume = volume;
                return true;
            }
            Log* log;
        };

        Log log;
        AudioManager audio;
        expect(!audio.isInitialized(), "audio should not be initialized before init");
        expect(audio.initialize(assets.string(), std::make_unique<FakeBackend>(&log)), "audio should initialize");
        expect(audio.isInitialized(), "audio should be initialized after init");

        // Music: found by name, whatever the file type; asking again is free.
        audio.playMusic("central");
        expect(log.music.size() == 1 && log.music[0] == "central.ogg" && log.loops[0], "a track plays, looping");
        audio.playMusic("central");
        expect(log.music.size() == 1, "asking for the current track does not restart it");
        audio.playMusic("battle");
        expect(log.music.size() == 2 && log.music[1] == "battle.mp3", "other file types are found too");
        expect(audio.getCurrentMusic() == "battle", "the current track is remembered");

        // A track with no file borrows its stand-in; one borrowing the file
        // already playing carries on without a restart.
        audio.setMusicFallback("boss", "battle");
        audio.setMusicFallback("alley", "central");
        audio.playMusic("boss");
        expect(log.music.size() == 2, "a stand-in that is already playing is not restarted");
        audio.playMusic("alley");
        expect(log.music.size() == 3 && log.music[2] == "central.ogg", "a missing track plays its stand-in");

        // No file and no stand-in: silence, not an error.
        const int stopsBefore = log.stops;
        audio.playMusic("ending");
        expect(log.music.size() == 3 && log.stops == stopsBefore + 1, "a track with no file stops the music");
        audio.playMusicOnce("battle");
        expect(log.music.size() == 4 && !log.loops[3], "a one-shot does not loop");

        // Sound effects.
        audio.playSfx("heal");
        expect(log.sfx.size() == 1 && log.sfx[0] == "heal.wav", "a sound effect plays");
        audio.playSfx("heal");
        expect(log.sfx.size() == 1, "the same sound straight away is dropped");
        audio.update(AudioManager::SFX_REPEAT_GUARD + 0.01f);
        audio.playSfx("heal");
        expect(log.sfx.size() == 2, "and plays again a moment later");
        audio.playSfx("gunshot");
        expect(log.sfx.size() == 2, "a sound with no file is silent");

        // Volume: master times channel; clamped to 0..1.
        audio.setMasterVolume(0.5f);
        audio.setMusicVolume(0.4f);
        expect(std::fabs(log.musicVolume - 0.2f) < 0.001f, "music volume is master times music");
        audio.setSFXVolume(0.5f);
        audio.update(1.0f);
        audio.playSfx("heal");
        expect(std::fabs(log.lastSfxVolume - 0.25f) < 0.001f, "sfx volume is master times sfx");
        audio.setMasterVolume(2.0f);
        expect(audio.getMasterVolume() == 1.0f, "master volume should clamp to 1.0");
        audio.setMasterVolume(-0.5f);
        expect(audio.getMasterVolume() == 0.0f, "master volume should clamp to 0.0");
        audio.setMasterVolume(1.0f);
        audio.setMusicVolume(1.0f);

        // Ducking quietens the music but lets sound effects through; muting
        // silences both.
        audio.setMusicDucked(true);
        expect(std::fabs(log.musicVolume - AudioManager::DUCKED_SCALE) < 0.001f, "ducked music is quiet");
        audio.update(1.0f);
        const std::size_t before = log.sfx.size();
        audio.playSfx("heal");
        expect(log.sfx.size() == before + 1, "sound effects still play while ducked");
        audio.setMusicDucked(false);
        audio.setMuted(true);
        expect(log.musicVolume == 0.0f, "muted music is silent");
        audio.update(1.0f);
        audio.playSfx("heal");
        expect(log.sfx.size() == before + 1, "muted sound effects do not play");
        audio.setMuted(false);
        expect(log.musicVolume == 1.0f, "unmuting restores the music");

        // With no sound device at all, everything is a quiet no-op.
        AudioManager silent;
        expect(silent.initialize(assets.string(), nullptr), "audio initializes without a device");
        silent.playMusic("central");
        silent.playSfx("heal");
        expect(silent.getCurrentMusic() == "central", "and still tracks what should be playing");

        audio.shutdown();
        std::filesystem::remove_all(assets);
    }

    // === InputConfig tests ===
    InputConfig config;
    expect(config.getBindings().size() == 8, "should have 8 default bindings");

    // Default key lookups
    InputAction upAction = config.getActionForKey(265); // GLFW_KEY_UP on Linux
    expect(upAction == InputAction::MOVE_UP, "key 265 should map to MOVE_UP on Linux");

    // Reverse lookup
    int upKey = config.getKeyForAction(InputAction::MOVE_UP);
    expect(upKey > 0, "MOVE_UP should have a positive key code");

    // Rebinding
    int originalKey = config.getKeyForAction(InputAction::CONFIRM);
    expect(config.rebindKey(InputAction::CONFIRM, 999), "rebinding CONFIRM to 999 should succeed");
    expect(config.getKeyForAction(InputAction::CONFIRM) == 999, "CONFIRM should now be on key 999");
    expect(config.getActionForKey(999) == InputAction::CONFIRM, "key 999 should map to CONFIRM");
    expect(config.getActionForKey(originalKey) == InputConfig::NO_ACTION, "old key should be unbound");

    // Can't rebind to a key already in use by another action
    int downKey = config.getKeyForAction(InputAction::MOVE_DOWN);
    expect(!config.rebindKey(InputAction::CONFIRM, downKey), "rebinding to an occupied key should fail");
    expect(config.getKeyForAction(InputAction::CONFIRM) == 999, "CONFIRM should still be on 999 after failed rebind");

    // isKeyBound
    expect(config.isKeyBound(999), "key 999 should be bound");
    expect(!config.isKeyBound(12345), "key 12345 should not be bound");

    // Reset to defaults
    config.resetToDefaults();
    expect(config.getBindings().size() == 8, "should have 8 bindings after reset");
    expect(config.getKeyForAction(InputAction::CONFIRM) == originalKey, "CONFIRM should be back to original key after reset");

    // Serialize/deserialize
    std::string serialized = config.serialize();
    expect(!serialized.empty(), "serialized config should not be empty");

    config.rebindKey(InputAction::MOVE_UP, 777);
    expect(config.getKeyForAction(InputAction::MOVE_UP) == 777, "MOVE_UP should be rebound to 777");

    bool deserialized = config.deserialize(serialized);
    expect(deserialized, "deserialization should succeed");
    expect(config.getKeyForAction(InputAction::MOVE_UP) != 777, "MOVE_UP should not be 777 after deserialize");
    expect(config.getBindings().size() == 8, "should have 8 bindings after deserialize");

    // --- InputState: presses, repeats and the held direction ---------------
    {
        InputState input;
        InputState::Held held{};
        const float frame = 1.0f / 60.0f;
        auto count = [](const std::vector<InputAction>& fired, InputAction action) {
            int n = 0;
            for (InputAction a : fired) n += (a == action) ? 1 : 0;
            return n;
        };
        int dx = 9;
        int dy = 9;

        expect(input.update(frame, held).empty(), "nothing held, nothing fires");
        expect(!input.heldDirection(dx, dy), "no direction is held at rest");

        held[InputState::indexOf(InputAction::CONFIRM)] = true;
        expect(count(input.update(frame, held), InputAction::CONFIRM) == 1, "a press fires once");
        int confirms = 0;
        for (int i = 0; i < 120; ++i) confirms += count(input.update(frame, held), InputAction::CONFIRM);
        expect(confirms == 0, "holding Confirm does not confirm again");
        held.fill(false);
        input.update(frame, held);

        held[InputState::indexOf(InputAction::MOVE_RIGHT)] = true;
        expect(count(input.update(frame, held), InputAction::MOVE_RIGHT) == 1, "a direction fires on press");
        expect(input.heldDirection(dx, dy) && dx == 1 && dy == 0, "and is reported as held");
        int early = 0;
        for (int i = 0; i < 18; ++i) early += count(input.update(frame, held), InputAction::MOVE_RIGHT);
        expect(early == 0, "a held direction waits before repeating");
        int repeats = 0;
        for (int i = 0; i < 60; ++i) repeats += count(input.update(frame, held), InputAction::MOVE_RIGHT);
        expect(repeats >= 6 && repeats <= 10, "then repeats steadily");
        expect(count(input.update(5.0f, held), InputAction::MOVE_RIGHT) <= 1, "a long frame does not flood repeats");

        // Rolling onto a second direction turns at once; letting it go
        // returns to the first.
        held[InputState::indexOf(InputAction::MOVE_UP)] = true;
        input.update(frame, held);
        expect(input.heldDirection(dx, dy) && dx == 0 && dy == -1, "the newest direction wins");
        held[InputState::indexOf(InputAction::MOVE_UP)] = false;
        input.update(frame, held);
        expect(input.heldDirection(dx, dy) && dx == 1 && dy == 0, "releasing it falls back to the older one");

        input.reset();
        expect(!input.heldDirection(dx, dy) && !input.isHeld(InputAction::MOVE_RIGHT), "reset forgets everything");
    }

    // --- InputPrompts: hints name the device in use -------------------------
    {
        using InputPrompts::Device;
        expect(InputPrompts::deviceForGamepadName("Sony DualSense") == Device::PLAYSTATION, "a DualSense is a PlayStation pad");
        expect(InputPrompts::deviceForGamepadName("PS4 Controller") == Device::PLAYSTATION, "so is a PS4 pad");
        expect(InputPrompts::deviceForGamepadName("Xbox Series X Controller") == Device::XBOX, "an Xbox pad is an Xbox pad");
        expect(InputPrompts::deviceForGamepadName("Steam Virtual Gamepad") == Device::XBOX, "Steam pads use Xbox names");
        expect(InputPrompts::deviceForGamepadName("") == Device::XBOX, "an unnamed pad uses Xbox names");

        InputPrompts::setBindings(nullptr);
        InputPrompts::setDevice(Device::KEYBOARD);
        expect(InputPrompts::confirm() == "Enter" && InputPrompts::cancel() == "Esc", "keyboard defaults");
        expect(InputPrompts::upDown() == "Up/Down" && InputPrompts::leftRight() == "Left/Right", "keyboard directions");

        InputPrompts::setDevice(Device::PLAYSTATION);
        expect(InputPrompts::confirm() == "Cross" && InputPrompts::cancel() == "Circle" &&
               InputPrompts::menu() == "Options", "PlayStation names");
        InputPrompts::setDevice(Device::XBOX);
        expect(InputPrompts::confirm() == "A" && InputPrompts::cancel() == "B" &&
               InputPrompts::menu() == "Start", "Xbox names");
        expect(InputPrompts::upDown() == "D-Pad", "controllers point with the D-pad");

        // A rebound key shows under its new name.
        InputConfig rebound;
        expect(rebound.rebindKey(InputAction::CONFIRM, 32), "rebinding Confirm to Space should work");
        InputPrompts::setBindings(&rebound);
        InputPrompts::setDevice(Device::KEYBOARD);
        expect(InputPrompts::confirm() == "Space", "keyboard hints follow the bindings");
        InputPrompts::setBindings(nullptr);
    }

    std::cout << "Phase5Test passed" << std::endl;
    return 0;
}
