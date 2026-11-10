#pragma once

#include "InputAction.h"
#include <unordered_map>
#include <vector>
#include <string>

// InputConfig — maps physical keys to InputAction values and supports rebinding.
// Gameplay logic only — no rendering or platform dependencies.
class InputConfig {
public:
    InputConfig();
    ~InputConfig() = default;

    // Initialize with default key bindings.
    void initializeDefaults();

    // Rebind a key for a given action. Returns true on success.
    bool rebindKey(InputAction action, int newKey);

    // Get the key bound to an action. Returns -1 if unbound.
    int getKeyForAction(InputAction action) const;

    // Get the action bound to a key. Returns a sentinel if unbound.
    InputAction getActionForKey(int key) const;

    // Check if a key is bound to any action.
    bool isKeyBound(int key) const;

    // Reset to default bindings.
    void resetToDefaults();

    // Get all bindings as a list of (key, action) pairs.
    const std::vector<std::pair<int, InputAction>>& getBindings() const { return m_bindingList; }

    // Serialize/deserialize for save/load.
    std::string serialize() const;
    bool deserialize(const std::string& data);

    // Load bindings from a file. A missing or unusable file leaves the
    // defaults in place and returns false.
    bool loadFromFile(const std::string& path);
    // Creates missing directories.
    bool saveToFile(const std::string& path) const;
    // $XDG_CONFIG_HOME/CyberRayne/input.cfg (normally ~/.config/CyberRayne/).
    static std::string defaultConfigPath();

    // True when every action has exactly one key and no key is shared.
    bool isComplete() const;

    // Human-readable names for the settings screen.
    static std::string keyName(int key);
    static const char* actionName(InputAction action);
    // Every rebindable action, in display order.
    static const std::vector<InputAction>& allActions();
    // The key that cancels a rebind in progress; it cannot be reassigned away.
    static int escapeKey();

    // Sentinel for "no action."
    static constexpr InputAction NO_ACTION = static_cast<InputAction>(-1);

private:
    void rebuildLookup();

    std::unordered_map<int, InputAction> m_keyToAction;
    std::unordered_map<int, int> m_actionToKey;  // keyed by static_cast<int>(action)
    std::vector<std::pair<int, InputAction>> m_bindingList;
};
