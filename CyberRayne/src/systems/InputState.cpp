#include "../../include/InputState.h"

bool InputState::isDirection(InputAction action) {
    return action == InputAction::MOVE_UP || action == InputAction::MOVE_DOWN ||
           action == InputAction::MOVE_LEFT || action == InputAction::MOVE_RIGHT;
}

std::vector<InputAction> InputState::update(float deltaTime, const Held& held) {
    std::vector<InputAction> fired;
    for (std::size_t i = 0; i < ACTION_COUNT; ++i) {
        const InputAction action = static_cast<InputAction>(i);
        if (!held[i]) {
            m_heldFor[i] = 0.0f;
            m_pressOrder[i] = 0;
        } else if (!m_held[i]) {
            // Fresh press.
            m_heldFor[i] = 0.0f;
            m_nextRepeat[i] = REPEAT_DELAY;
            m_pressOrder[i] = ++m_pressCounter;
            fired.push_back(action);
        } else {
            m_heldFor[i] += deltaTime;
            // Only directions repeat; holding Confirm must not confirm twice.
            if (isDirection(action) && m_heldFor[i] >= m_nextRepeat[i]) {
                // At most one repeat per frame, even after a long frame.
                m_nextRepeat[i] = m_heldFor[i] + REPEAT_INTERVAL;
                fired.push_back(action);
            }
        }
    }
    m_held = held;
    return fired;
}

bool InputState::isHeld(InputAction action) const {
    const std::size_t i = indexOf(action);
    return i < ACTION_COUNT && m_held[i];
}

bool InputState::heldDirection(int& dx, int& dy) const {
    static constexpr struct { InputAction action; int dx; int dy; } STEPS[] = {
        {InputAction::MOVE_UP, 0, -1},
        {InputAction::MOVE_DOWN, 0, 1},
        {InputAction::MOVE_LEFT, -1, 0},
        {InputAction::MOVE_RIGHT, 1, 0},
    };
    unsigned long newest = 0;
    for (const auto& step : STEPS) {
        const std::size_t i = indexOf(step.action);
        if (m_held[i] && m_pressOrder[i] > newest) {
            newest = m_pressOrder[i];
            dx = step.dx;
            dy = step.dy;
        }
    }
    return newest != 0;
}

void InputState::reset() {
    m_held.fill(false);
    m_heldFor.fill(0.0f);
    m_nextRepeat.fill(0.0f);
    m_pressOrder.fill(0);
}
