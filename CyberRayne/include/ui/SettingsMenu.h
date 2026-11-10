#pragma once

#include <string>

#include "../InputAction.h"

class VulkanRenderer;
class TextRenderer;
class InputConfig;

// The settings screen: one row per rebindable action, then "Reset to
// defaults" and "Back". Choosing an action waits for the next key press and
// binds it. Logic is renderer-free; drawing is compiled only into the game.
class SettingsMenu {
public:
    enum class Command { NONE, CLOSE };

    void open();

    // Normal navigation. Ignored while waiting for a key.
    Command handleInput(InputAction action, InputConfig& config);

    // True while waiting for the player to press the key to bind. While this
    // is true the game must send raw keys here instead of actions.
    bool wantsRawKey() const { return m_capturing; }
    void handleRawKey(int key, InputConfig& config);

    // True once after bindings change, so the caller can write them to disk.
    bool consumeChanged();

    int getIndex() const { return m_index; }
    const std::string& getStatus() const { return m_status; }
    static int rowCount();

#ifndef NO_VULKAN
    void render(VulkanRenderer* renderer, TextRenderer* text, const InputConfig& config);
#endif

private:
    int m_index = 0;
    bool m_capturing = false;
    bool m_changed = false;
    std::string m_status;
};
