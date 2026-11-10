#include "../../include/InputPrompts.h"
#include "../../include/InputAction.h"
#include "../../include/InputConfig.h"

#include <algorithm>
#include <cctype>

namespace {

InputPrompts::Device g_device = InputPrompts::Device::KEYBOARD;
const InputConfig* g_bindings = nullptr;

// The name of the key bound to an action, or a sensible default when there
// are no bindings to ask (tests, or before the game has loaded them).
std::string keyFor(InputAction action, const char* fallback) {
    if (g_bindings) {
        const int key = g_bindings->getKeyForAction(action);
        if (key >= 0) {
            return InputConfig::keyName(key);
        }
    }
    return fallback;
}

}  // namespace

namespace InputPrompts {

void setDevice(Device device) { g_device = device; }
Device device() { return g_device; }
void setBindings(const InputConfig* bindings) { g_bindings = bindings; }

Device deviceForGamepadName(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const char* hint : {"sony", "playstation", "dualsense", "dualshock", "ps3", "ps4", "ps5"}) {
        if (lower.find(hint) != std::string::npos) {
            return Device::PLAYSTATION;
        }
    }
    return Device::XBOX;
}

std::string confirm() {
    switch (g_device) {
        case Device::PLAYSTATION: return "Cross";
        case Device::XBOX:        return "A";
        case Device::KEYBOARD:    break;
    }
    return keyFor(InputAction::CONFIRM, "Enter");
}

std::string cancel() {
    switch (g_device) {
        case Device::PLAYSTATION: return "Circle";
        case Device::XBOX:        return "B";
        case Device::KEYBOARD:    break;
    }
    return keyFor(InputAction::CANCEL, "Esc");
}

std::string menu() {
    switch (g_device) {
        case Device::PLAYSTATION: return "Options";
        case Device::XBOX:        return "Start";
        case Device::KEYBOARD:    break;
    }
    return keyFor(InputAction::OPEN_MENU, "M");
}

std::string upDown() {
    if (g_device != Device::KEYBOARD) {
        return "D-Pad";
    }
    return keyFor(InputAction::MOVE_UP, "Up") + "/" + keyFor(InputAction::MOVE_DOWN, "Down");
}

std::string leftRight() {
    if (g_device != Device::KEYBOARD) {
        return "D-Pad";
    }
    return keyFor(InputAction::MOVE_LEFT, "Left") + "/" + keyFor(InputAction::MOVE_RIGHT, "Right");
}

}  // namespace InputPrompts
