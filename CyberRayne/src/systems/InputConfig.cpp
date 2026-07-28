#include "../../include/InputConfig.h"
#include <sstream>
#include <iostream>

// Platform-specific key codes. We use the same integer values as
// GLFW on Linux and virtual key codes on Windows. The config stores
// raw ints so it works regardless of platform.
#ifdef _WIN32
static constexpr int KEY_UP    = 0x26; // VK_UP
static constexpr int KEY_DOWN  = 0x28; // VK_DOWN
static constexpr int KEY_LEFT  = 0x25; // VK_LEFT
static constexpr int KEY_RIGHT = 0x27; // VK_RIGHT
static constexpr int KEY_ENTER = 0x0D; // VK_RETURN
static constexpr int KEY_ESC   = 0x1B; // VK_ESCAPE
static constexpr int KEY_M     = 0x4D; // 'M'
static constexpr int KEY_P     = 0x50; // 'P'
#else
// GLFW key codes
static constexpr int KEY_UP    = 265; // GLFW_KEY_UP
static constexpr int KEY_DOWN  = 264; // GLFW_KEY_DOWN
static constexpr int KEY_LEFT  = 263; // GLFW_KEY_LEFT
static constexpr int KEY_RIGHT = 262; // GLFW_KEY_RIGHT
static constexpr int KEY_ENTER = 257; // GLFW_KEY_ENTER
static constexpr int KEY_ESC   = 256; // GLFW_KEY_ESCAPE
static constexpr int KEY_M     = 77;  // GLFW_KEY_M
static constexpr int KEY_P     = 80;  // GLFW_KEY_P
#endif

InputConfig::InputConfig() {
    initializeDefaults();
}

void InputConfig::initializeDefaults() {
    m_keyToAction.clear();
    m_actionToKey.clear();
    m_bindingList.clear();

    struct DefaultBinding { int key; InputAction action; };
    const DefaultBinding defaults[] = {
        {KEY_UP,     InputAction::MOVE_UP},
        {KEY_DOWN,   InputAction::MOVE_DOWN},
        {KEY_LEFT,   InputAction::MOVE_LEFT},
        {KEY_RIGHT,  InputAction::MOVE_RIGHT},
        {KEY_ENTER,  InputAction::CONFIRM},
        {KEY_ESC,    InputAction::CANCEL},
        {KEY_M,      InputAction::OPEN_MENU},
        {KEY_P,      InputAction::PAUSE},
    };

    for (const auto& b : defaults) {
        m_keyToAction[b.key] = b.action;
        m_actionToKey[static_cast<int>(b.action)] = b.key;
        m_bindingList.push_back({b.key, b.action});
    }
}

bool InputConfig::rebindKey(InputAction action, int newKey) {
    // Don't allow binding to a key already in use by another action.
    if (m_keyToAction.count(newKey) && m_keyToAction[newKey] != action) {
        std::cerr << "Key " << newKey << " is already bound to another action." << std::endl;
        return false;
    }

    // Remove old binding for this action.
    auto it = m_actionToKey.find(static_cast<int>(action));
    if (it != m_actionToKey.end()) {
        int oldKey = it->second;
        m_keyToAction.erase(oldKey);
    }

    m_keyToAction[newKey] = action;
    m_actionToKey[static_cast<int>(action)] = newKey;
    rebuildLookup();
    return true;
}

int InputConfig::getKeyForAction(InputAction action) const {
    auto it = m_actionToKey.find(static_cast<int>(action));
    if (it != m_actionToKey.end()) {
        return it->second;
    }
    return -1;
}

InputAction InputConfig::getActionForKey(int key) const {
    auto it = m_keyToAction.find(key);
    if (it != m_keyToAction.end()) {
        return it->second;
    }
    return NO_ACTION;
}

bool InputConfig::isKeyBound(int key) const {
    return m_keyToAction.count(key) > 0;
}

void InputConfig::resetToDefaults() {
    initializeDefaults();
}

void InputConfig::rebuildLookup() {
    m_bindingList.clear();
    for (const auto& pair : m_keyToAction) {
        m_bindingList.push_back({pair.first, pair.second});
    }
}

std::string InputConfig::serialize() const {
    std::ostringstream oss;
    for (const auto& b : m_bindingList) {
        oss << b.first << ":" << static_cast<int>(b.second) << ";";
    }
    return oss.str();
}

bool InputConfig::deserialize(const std::string& data) {
    m_keyToAction.clear();
    m_actionToKey.clear();
    m_bindingList.clear();

    std::istringstream iss(data);
    std::string token;
    while (std::getline(iss, token, ';')) {
        if (token.empty()) continue;
        size_t colon = token.find(':');
        if (colon == std::string::npos) continue;
        int key = std::stoi(token.substr(0, colon));
        int actionVal = std::stoi(token.substr(colon + 1));
        InputAction action = static_cast<InputAction>(actionVal);
        m_keyToAction[key] = action;
        m_actionToKey[static_cast<int>(action)] = key;
        m_bindingList.push_back({key, action});
    }
    return !m_bindingList.empty();
}
