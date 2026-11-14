#include "../../include/InputPrompts.h"
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

// Row layout: volumes, then actions, then Reset, then Back.
constexpr int FIRST_ACTION = SettingsMenu::VOLUME_ROWS;

int SettingsMenu::rowCount() {
    return VOLUME_ROWS + actionCount() + 2;  // + Reset to defaults, Back
}

bool SettingsMenu::consumeVolumeChanged() {
    const bool changed = m_volumeChanged;
    m_volumeChanged = false;
    return changed;
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
        case InputAction::MOVE_LEFT:
        case InputAction::MOVE_RIGHT:
            if (m_index < VOLUME_ROWS) {
                int* levels[VOLUME_ROWS] = {&m_volumes.master, &m_volumes.music, &m_volumes.effects};
                int& level = *levels[m_index];
                const int wanted = level + (action == InputAction::MOVE_RIGHT ? 1 : -1);
                if (wanted >= 0 && wanted <= AudioSettings::MAX_STEP) {
                    level = wanted;
                    m_volumeChanged = true;
                }
            }
            break;
        case InputAction::CANCEL:
            return Command::CLOSE;
        case InputAction::CONFIRM:
            if (m_index < VOLUME_ROWS) {
                m_status = InputPrompts::leftRight() + " changes the volume.";
            } else if (m_index < FIRST_ACTION + actionCount()) {
                m_capturing = true;
                m_status = "Press the new key. " + InputPrompts::cancel() + " cancels.";
            } else if (m_index == FIRST_ACTION + actionCount()) {
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

    const InputAction action = InputConfig::allActions()[static_cast<std::size_t>(m_index - FIRST_ACTION)];
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
    const int h = pad * 2 + line * (rowCount() + 3) + line / 2;
    const int x = (screenW - w) / 2;
    const int y0 = (screenH - h) / 2 - 30;
    Theme::drawPanel(renderer, x, y0, w, h);

    int y = y0 + pad;
    text->renderText(renderer, "SETTINGS", x + pad, y, scale, Theme::IRIS);
    y += line * 2;

    const auto& actions = InputConfig::allActions();
    const int valueX = x + w / 2 + 80;
    static const char* const kVolumeLabels[VOLUME_ROWS] = {"Master volume", "Music volume", "Effects volume"};
    const int levels[VOLUME_ROWS] = {m_volumes.master, m_volumes.music, m_volumes.effects};
    for (int i = 0; i < rowCount(); ++i) {
        const bool selected = i == m_index;
        const uint32_t color = selected ? Theme::GOLD : Theme::TEXT;
        std::string label;
        std::string value;
        if (i < VOLUME_ROWS) {
            label = kVolumeLabels[i];
            // A ten-segment bar, with the number beside it.
            const int segW = 22;
            const int segH = Theme::GLYPH * scale;
            for (int s = 0; s < AudioSettings::MAX_STEP; ++s) {
                const bool lit = s < levels[i];
                Theme::fillRect(renderer, valueX + s * (segW + 4), y, segW, segH,
                                lit ? (selected ? Theme::GOLD : Theme::FOAM) : Theme::MUTED, lit ? 1.0f : 0.3f);
            }
            text->renderText(renderer, std::to_string(levels[i] * 10) + "%",
                             valueX + AudioSettings::MAX_STEP * (segW + 4) + 14, y, scale, color);
        } else if (i < FIRST_ACTION + actionCount()) {
            if (i == FIRST_ACTION) {
                y += line / 2;  // gap between volume and key bindings
            }
            const InputAction action = actions[static_cast<std::size_t>(i - FIRST_ACTION)];
            label = InputConfig::actionName(action);
            value = (selected && m_capturing) ? "..." : InputConfig::keyName(config.getKeyForAction(action));
        } else if (i == FIRST_ACTION + actionCount()) {
            label = "Reset keys to defaults";
            y += line / 2;
        } else {
            label = "Back";
        }
        text->renderText(renderer, (selected ? "> " : "  ") + label, x + pad, y, scale, color);
        if (!value.empty()) {
            text->renderText(renderer, value, valueX, y, scale,
                             (selected && m_capturing) ? Theme::LOVE : Theme::FOAM);
        }
        y += line;
    }

    const int small = 2;
    if (!m_status.empty()) {
        text->renderText(renderer, m_status, x, y0 + h + 20, small, m_capturing ? Theme::LOVE : Theme::FOAM);
    }
    text->renderText(renderer,
                     InputPrompts::upDown() + ": select   " + InputPrompts::leftRight() + ": volume   " +
                         InputPrompts::confirm() + ": change key   " +
                         InputPrompts::cancel() + ": back",
                     x, y0 + h + 52, small, Theme::MUTED);
}
#endif
