#include "../../include/ui/BattleFx.h"
#include "../../include/Combatant.h"
#include "../../include/ui/Theme.h"

#include <algorithm>
#include <cmath>

void BattleFx::reset() {
    m_popups.clear();
    m_timers.clear();
    m_banner.clear();
    m_bannerLeft = 0.0f;
}

void BattleFx::addPopup(const Combatant* who, const std::string& text, uint32_t color) {
    // Queue behind any number on the same combatant that has not started
    // rising yet.
    float delay = 0.0f;
    for (const Popup& popup : m_popups) {
        if (popup.who == who && popup.age < POPUP_STAGGER) {
            delay = std::max(delay, POPUP_STAGGER - popup.age);
        }
    }
    m_popups.push_back({who, text, color, -delay});
}

void BattleFx::onEvents(const std::vector<BattleSystem::BattleEvent>& events) {
    using Kind = BattleSystem::BattleEvent::Kind;
    for (const auto& event : events) {
        const Combatant* who = event.subject;
        switch (event.kind) {
            case Kind::ACTION:
                if (who) {
                    Timers& timers = m_timers[who];
                    timers.lunge = LUNGE_TIME;
                    timers.action = ACTION_TIME;
                    if (event.text == "Attack") {
                        timers.pose = Pose::ATTACK;
                    } else if (event.text == "Defend") {
                        timers.pose = Pose::GUARD;
                    } else {
                        timers.pose = Pose::ABILITY;  // abilities and items
                    }
                    m_banner = who->getName() + ": " + event.text;
                } else {
                    m_banner = event.text;
                }
                m_bannerLeft = BANNER_TIME;
                break;
            case Kind::DAMAGE:
                if (!who) break;
                m_timers[who].flash = FLASH_TIME;
                addPopup(who, std::to_string(event.amount),
                         who->isPlayer() ? Theme::LOVE : Theme::TEXT);
                break;
            case Kind::HEAL:
                if (!who) break;
                // Healing someone who was down means they are back up.
                m_timers[who].defeatedFor = -1.0f;
                addPopup(who, "+" + std::to_string(event.amount), Theme::FOAM);
                break;
            case Kind::DEFEATED:
                if (!who) break;
                m_timers[who].defeatedFor = 0.0f;
                break;
            case Kind::MESSAGE:
                if (who) {
                    addPopup(who, event.text, Theme::GOLD);
                } else {
                    m_banner = event.text;
                    m_bannerLeft = BANNER_TIME;
                }
                break;
        }
    }
}

void BattleFx::update(float deltaTime) {
    for (Popup& popup : m_popups) {
        popup.age += deltaTime;
    }
    m_popups.erase(std::remove_if(m_popups.begin(), m_popups.end(),
                                  [](const Popup& p) { return p.age >= POPUP_LIFETIME; }),
                   m_popups.end());

    for (auto& [who, timers] : m_timers) {
        timers.flash = std::max(0.0f, timers.flash - deltaTime);
        timers.lunge = std::max(0.0f, timers.lunge - deltaTime);
        timers.action = std::max(0.0f, timers.action - deltaTime);
        if (timers.defeatedFor >= 0.0f) {
            timers.defeatedFor += deltaTime;
        }
    }

    if (m_bannerLeft > 0.0f) {
        m_bannerLeft -= deltaTime;
        if (m_bannerLeft <= 0.0f) {
            m_banner.clear();
        }
    }
}

float BattleFx::getHitFlash(const Combatant* who) const {
    auto it = m_timers.find(who);
    return it == m_timers.end() ? 0.0f : it->second.flash / FLASH_TIME;
}

float BattleFx::getLunge(const Combatant* who) const {
    auto it = m_timers.find(who);
    if (it == m_timers.end() || it->second.lunge <= 0.0f) {
        return 0.0f;
    }
    const float progress = 1.0f - it->second.lunge / LUNGE_TIME;
    return std::sin(progress * 3.14159265f);
}

BattleFx::Action BattleFx::getAction(const Combatant* who) const {
    auto it = m_timers.find(who);
    if (it == m_timers.end() || it->second.action <= 0.0f) {
        return {};
    }
    return {it->second.pose, 1.0f - it->second.action / ACTION_TIME};
}

float BattleFx::getVisibility(const Combatant* who) const {
    auto it = m_timers.find(who);
    if (it == m_timers.end() || it->second.defeatedFor < 0.0f) {
        return 1.0f;
    }
    return std::max(0.0f, 1.0f - it->second.defeatedFor / FADE_TIME);
}

bool BattleFx::isDefeated(const Combatant* who) const {
    auto it = m_timers.find(who);
    return it != m_timers.end() && it->second.defeatedFor >= 0.0f;
}

bool BattleFx::isBusy() const {
    if (!m_popups.empty()) {
        return true;
    }
    for (const auto& [who, timers] : m_timers) {
        if (timers.flash > 0.0f || timers.lunge > 0.0f || timers.action > 0.0f) return true;
        if (timers.defeatedFor >= 0.0f && timers.defeatedFor < FADE_TIME) return true;
    }
    return false;
}

StatusTag statusTagFor(StatusEffect::Type type) {
    switch (type) {
        case StatusEffect::Type::POISON:       return {"PSN", Theme::LOVE};
        case StatusEffect::Type::STUN:         return {"STN", Theme::GOLD};
        case StatusEffect::Type::DEFENSE_UP:   return {"DEF+", Theme::FOAM};
        case StatusEffect::Type::DEFENSE_DOWN: return {"DEF-", Theme::LOVE};
        case StatusEffect::Type::ATTACK_UP:    return {"ATK+", Theme::FOAM};
        case StatusEffect::Type::ATTACK_DOWN:  return {"ATK-", Theme::LOVE};
        case StatusEffect::Type::SLOW:         return {"SLOW", Theme::IRIS};
        case StatusEffect::Type::REGEN:        return {"RGN", Theme::FOAM};
    }
    return {"?", Theme::MUTED};
}
