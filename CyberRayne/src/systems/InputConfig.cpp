#include "../../include/InputConfig.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
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
    std::unordered_map<int, InputAction> keyToAction;
    std::unordered_map<int, int> actionToKey;
    std::vector<std::pair<int, InputAction>> bindingList;

    std::istringstream iss(data);
    std::string token;
    while (std::getline(iss, token, ';')) {
        if (token.empty()) continue;
        size_t colon = token.find(':');
        if (colon == std::string::npos) continue;
        int key = 0;
        int actionVal = 0;
        try {
            key = std::stoi(token.substr(0, colon));
            actionVal = std::stoi(token.substr(colon + 1));
        } catch (const std::exception&) {
            return false;
        }
        if (actionVal < 0 || actionVal >= static_cast<int>(allActions().size())) {
            return false;
        }
        InputAction action = static_cast<InputAction>(actionVal);
        keyToAction[key] = action;
        actionToKey[actionVal] = key;
        bindingList.push_back({key, action});
    }
    if (bindingList.empty()) {
        return false;
    }

    // Commit only once the whole string has parsed, so bad data never leaves
    // the config half-replaced.
    m_keyToAction = std::move(keyToAction);
    m_actionToKey = std::move(actionToKey);
    m_bindingList = std::move(bindingList);
    return true;
}

bool InputConfig::isComplete() const {
    if (m_keyToAction.size() != allActions().size()) {
        return false;
    }
    for (InputAction action : allActions()) {
        auto it = m_actionToKey.find(static_cast<int>(action));
        if (it == m_actionToKey.end()) return false;
        auto back = m_keyToAction.find(it->second);
        if (back == m_keyToAction.end() || back->second != action) return false;
    }
    return true;
}

std::string InputConfig::defaultConfigPath() {
    namespace fs = std::filesystem;
    fs::path dir;
#ifdef _WIN32
    if (const char* appData = std::getenv("APPDATA"); appData && *appData) {
        dir = fs::path(appData) / "CyberRayne";
    }
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        dir = fs::path(xdg) / "CyberRayne";
    } else if (const char* home = std::getenv("HOME"); home && *home) {
        dir = fs::path(home) / ".config" / "CyberRayne";
    }
#endif
    return (dir / "input.cfg").string();
}

bool InputConfig::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    std::string data;
    std::getline(file, data);

    // Try it on a copy: a file that parses but leaves an action unbound or a
    // key shared would lock the player out of that action.
    InputConfig candidate;
    if (!candidate.deserialize(data) || !candidate.isComplete()) {
        std::cerr << "Ignoring unusable key bindings in " << path << "; using defaults." << std::endl;
        return false;
    }
    *this = candidate;
    return true;
}

bool InputConfig::saveToFile(const std::string& path) const {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path target(path);
    if (target.has_parent_path()) {
        fs::create_directories(target.parent_path(), ec);
    }
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to write key bindings to " << path << std::endl;
        return false;
    }
    file << serialize() << "\n";
    return file.good();
}

const std::vector<InputAction>& InputConfig::allActions() {
    static const std::vector<InputAction> kActions = {
        InputAction::MOVE_UP, InputAction::MOVE_DOWN, InputAction::MOVE_LEFT, InputAction::MOVE_RIGHT,
        InputAction::CONFIRM, InputAction::CANCEL, InputAction::OPEN_MENU, InputAction::PAUSE};
    return kActions;
}

const char* InputConfig::actionName(InputAction action) {
    switch (action) {
        case InputAction::MOVE_UP:    return "Move Up";
        case InputAction::MOVE_DOWN:  return "Move Down";
        case InputAction::MOVE_LEFT:  return "Move Left";
        case InputAction::MOVE_RIGHT: return "Move Right";
        case InputAction::CONFIRM:    return "Confirm / Talk";
        case InputAction::CANCEL:     return "Cancel";
        case InputAction::OPEN_MENU:  return "Menu";
        case InputAction::PAUSE:      return "Pause";
    }
    return "?";
}

int InputConfig::escapeKey() {
    return KEY_ESC;
}

std::string InputConfig::keyName(int key) {
    if (key == KEY_UP) return "Up";
    if (key == KEY_DOWN) return "Down";
    if (key == KEY_LEFT) return "Left";
    if (key == KEY_RIGHT) return "Right";
    if (key == KEY_ENTER) return "Enter";
    if (key == KEY_ESC) return "Esc";
    if (key == 32) return "Space";
#ifndef _WIN32
    // GLFW codes for the common non-printing keys.
    switch (key) {
        case 258: return "Tab";
        case 259: return "Backspace";
        case 260: return "Insert";
        case 261: return "Delete";
        case 340: return "Left Shift";
        case 341: return "Left Ctrl";
        case 342: return "Left Alt";
        case 344: return "Right Shift";
        case 345: return "Right Ctrl";
        case 346: return "Right Alt";
        default: break;
    }
    if (key >= 290 && key <= 301) return "F" + std::to_string(key - 289);
    if (key >= 320 && key <= 329) return "Numpad " + std::to_string(key - 320);
#endif
    // Letters and digits share their ASCII codes on both platforms.
    if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) {
        return std::string(1, static_cast<char>(key));
    }
    if (key > 32 && key < 127) {
        return std::string(1, static_cast<char>(key));
    }
    return "Key " + std::to_string(key);
}
