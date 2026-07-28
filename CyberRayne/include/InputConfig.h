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

    // Sentinel for "no action."
    static constexpr InputAction NO_ACTION = static_cast<InputAction>(-1);

private:
    void rebuildLookup();

    std::unordered_map<int, InputAction> m_keyToAction;
    std::unordered_map<int, int> m_actionToKey;  // keyed by static_cast<int>(action)
    std::vector<std::pair<int, InputAction>> m_bindingList;
};
