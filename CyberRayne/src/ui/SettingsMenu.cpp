#include "../../include/ui/SettingsMenu.h"
#include "../../include/InputConfig.h"

#ifndef NO_VULKAN
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/TextRenderer.h"
#include "../../include/ui/Theme.h"
#endif

namespace {
int actionCount() {
    return static_cast<int>(InputConfig::allActions().size());
}
}  // namespace

int SettingsMenu::rowCount() {
    return actionCount() + 2;  // + Reset to defaults, Back
}

void SettingsMenu::open() {
    m_index = 0;
    m_capturing = false;
    m_status.clear();
}

bool SettingsMenu::consumeChanged() {
    const bool changed = m_changed;
    m_changed = false;
    return changed;
}

SettingsMenu::Command SettingsMenu::handleInput(InputAction action, InputConfig& config) {
    if (m_capturing) {
        return Command::NONE;
    }

    const int rows = rowCount();
    switch (action) {
        case InputAction::MOVE_UP:
            m_index = (m_index + rows - 1) % rows;
            m_status.clear();
            break;
        case InputAction::MOVE_DOWN:
            m_index = (m_index + 1) % rows;
            m_status.clear();
            break;
        case InputAction::CANCEL:
            return Command::CLOSE;
        case InputAction::CONFIRM:
            if (m_index < actionCount()) {
                m_capturing = true;
                m_status = "Press the new key. Esc cancels.";
            } else if (m_index == actionCount()) {
                config.resetToDefaults();
                m_changed = true;
                m_status = "Key bindings reset to defaults.";
            } else {
                return Command::CLOSE;
            }
            break;
        default:
            break;
    }
    return Command::NONE;
}

void SettingsMenu::handleRawKey(int key, InputConfig& config) {
    if (!m_capturing) {
        return;
    }
    m_capturing = false;

    const InputAction action = InputConfig::allActions()[static_cast<std::size_t>(m_index)];
    // Esc always backs out of a rebind, so it can never be given to another
    // action by accident. Cancel itself may keep it.
    if (key == InputConfig::escapeKey() && action != InputAction::CANCEL) {
        m_status = "Cancelled.";
        return;
    }

    const InputAction owner = config.getActionForKey(key);
    if (owner != InputConfig::NO_ACTION && owner != action) {
        m_status = InputConfig::keyName(key) + " is already used for " + InputConfig::actionName(owner) + ".";
        return;
    }

    if (config.rebindKey(action, key)) {
        m_changed = true;
        m_status = std::string(InputConfig::actionName(action)) + " is now " + InputConfig::keyName(key) + ".";
    } else {
        m_status = "That key can't be used.";
    }
}

#ifndef NO_VULKAN
void SettingsMenu::render(VulkanRenderer* renderer, TextRenderer* text, const InputConfig& config) {
    if (!renderer || !text) return;

    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    Theme::dimScreen(renderer, 1.0f);

    const int scale = 3;
    const int line = Theme::GLYPH * scale + 20;
    const int pad = 36;
    const int w = 1000;
    const int h = pad * 2 + line * (rowCount() + 3);
    const int x = (screenW - w) / 2;
    const int y0 = (screenH - h) / 2 - 30;
    Theme::drawPanel(renderer, x, y0, w, h);

    int y = y0 + pad;
    text->renderText(renderer, "SETTINGS", x + pad, y, scale, Theme::IRIS);
    text->renderText(renderer, "Key Bindings", x + w - pad - 12 * Theme::GLYPH * scale, y, scale, Theme::MUTED);
    y += line * 2;

    const auto& actions = InputConfig::allActions();
    for (int i = 0; i < rowCount(); ++i) {
        const bool selected = i == m_index;
        const uint32_t color = selected ? Theme::GOLD : Theme::TEXT;
        std::string label;
        std::string value;
        if (i < actionCount()) {
            const InputAction action = actions[static_cast<std::size_t>(i)];
            label = InputConfig::actionName(action);
            value = (selected && m_capturing) ? "..." : InputConfig::keyName(config.getKeyForAction(action));
        } else if (i == actionCount()) {
            label = "Reset to defaults";
            y += line / 2;
        } else {
            label = "Back";
        }
        text->renderText(renderer, (selected ? "> " : "  ") + label, x + pad, y, scale, color);
        if (!value.empty()) {
            text->renderText(renderer, value, x + w / 2 + 80, y, scale,
                             (selected && m_capturing) ? Theme::LOVE : Theme::FOAM);
        }
        y += line;
    }

    const int small = 2;
    if (!m_status.empty()) {
        text->renderText(renderer, m_status, x, y0 + h + 20, small, m_capturing ? Theme::LOVE : Theme::FOAM);
    }
    text->renderText(renderer, "Up/Down: select   Enter: change   Esc: back", x, y0 + h + 52, small, Theme::MUTED);
}
#endif
