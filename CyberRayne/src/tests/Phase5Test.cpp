#include "../../include/AudioManager.h"
#include "../../include/InputConfig.h"
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
    AudioManager audio;
    expect(!audio.isInitialized(), "audio should not be initialized before init");
    expect(audio.initialize(), "audio should initialize successfully");
    expect(audio.isInitialized(), "audio should be initialized after init");

    // Volume controls
    audio.setMasterVolume(0.5f);
    expect(audio.getMasterVolume() == 0.5f, "master volume should be 0.5");
    audio.setMusicVolume(0.3f);
    expect(audio.getMusicVolume() == 0.3f, "music volume should be 0.3");
    audio.setSFXVolume(0.7f);
    expect(audio.getSFXVolume() == 0.7f, "sfx volume should be 0.7");

    // Clamping
    audio.setMasterVolume(2.0f);
    expect(audio.getMasterVolume() == 1.0f, "master volume should clamp to 1.0");
    audio.setMasterVolume(-0.5f);
    expect(audio.getMasterVolume() == 0.0f, "master volume should clamp to 0.0");

    // Sound registration
    int musicId = audio.registerSound("bgm_main", "assets/audio/bgm_main.ogg", AudioManager::ChannelType::MUSIC);
    expect(musicId >= 0, "music registration should return valid id");
    int sfxId = audio.registerSound("sfx_hit", "assets/audio/sfx_hit.wav", AudioManager::ChannelType::SFX);
    expect(sfxId >= 0, "sfx registration should return valid id");
    expect(audio.getSoundCount() == 2, "should have 2 sounds registered");

    // Name lookup
    expect(audio.getSoundId("bgm_main") == musicId, "name lookup should find music id");
    expect(audio.getSoundId("sfx_hit") == sfxId, "name lookup should find sfx id");
    expect(audio.getSoundId("nonexistent") == -1, "nonexistent sound should return -1");

    // Playback
    expect(!audio.isPlaying(musicId), "music should not be playing initially");
    audio.playSound(musicId);
    expect(audio.isPlaying(musicId), "music should be playing after play");
    audio.stopSound(musicId);
    expect(!audio.isPlaying(musicId), "music should stop after stop");

    // Mute
    audio.playSound(sfxId);
    expect(audio.isPlaying(sfxId), "sfx should be playing");
    audio.setMuted(true);
    expect(audio.isMuted(), "should be muted");
    expect(!audio.isPlaying(sfxId), "sfx should stop when muted");
    audio.setMuted(false);
    expect(!audio.isMuted(), "should be unmuted");

    // Stop all
    audio.playSound(musicId);
    audio.playSound(sfxId);
    audio.stopAllSounds();
    expect(!audio.isPlaying(musicId), "music should stop after stopAll");
    expect(!audio.isPlaying(sfxId), "sfx should stop after stopAll");

    // Stop by channel
    audio.playSound(musicId);
    audio.playSound(sfxId);
    audio.stopMusic();
    expect(!audio.isPlaying(musicId), "music should stop after stopMusic");
    expect(audio.isPlaying(sfxId), "sfx should still play after stopMusic");
    audio.stopAllSFX();
    expect(!audio.isPlaying(sfxId), "sfx should stop after stopAllSFX");

    // Shutdown
    audio.shutdown();
    expect(!audio.isInitialized(), "audio should not be initialized after shutdown");

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

    std::cout << "Phase5Test passed" << std::endl;
    return 0;
}
