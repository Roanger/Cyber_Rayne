#pragma once

#include "InputAction.h"

#include <array>
#include <vector>

// Turns "which actions are held down right now" into what the game acts on:
// one event when something is first pressed, repeat events while a direction
// stays held (so a menu scrolls without tapping), and the direction to keep
// walking in. Keyboard and controller both feed the same held-down table, so
// the rest of the game never needs to know which one was used.
//
// Gameplay logic only — no platform dependencies.
class InputState {
public:
    static constexpr std::size_t ACTION_COUNT = 8;
    using Held = std::array<bool, ACTION_COUNT>;

    // How long a direction is held before it starts repeating, and how often
    // it repeats after that.
    static constexpr float REPEAT_DELAY = 0.40f;
    static constexpr float REPEAT_INTERVAL = 0.11f;

    static std::size_t indexOf(InputAction action) { return static_cast<std::size_t>(action); }

    // Call once per frame. Returns the actions that fire this frame.
    std::vector<InputAction> update(float deltaTime, const Held& held);

    bool isHeld(InputAction action) const;

    // The direction being held, as a one-tile step. With two held at once the
    // one pressed last wins, so rolling from one key or stick direction to
    // another turns straight away. False when no direction is held.
    bool heldDirection(int& dx, int& dy) const;

    void reset();

private:
    static bool isDirection(InputAction action);

    Held m_held{};
    std::array<float, ACTION_COUNT> m_heldFor{};
    std::array<float, ACTION_COUNT> m_nextRepeat{};
    // Order of pressing: higher was pressed more recently; 0 = not held.
    std::array<unsigned long, ACTION_COUNT> m_pressOrder{};
    unsigned long m_pressCounter = 0;
};
