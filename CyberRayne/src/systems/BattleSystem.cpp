#include "../../include/BattleSystem.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include "../../include/Combatant.h"
#include "../../include/Spell.h"
#include "../../include/Item.h"
#include "../../include/StatusEffect.h"
#include <iostream>
#include <algorithm>
#include <random>

namespace {

// Scale speed to a reasonable ATB fill rate.
// 100 speed ~= 5 seconds to fill the gauge (at 60 FPS ~ 300 frames).
constexpr float ATB_FILL_BASE = 0.005f;

int rollFleeChance() {
    static std::mt19937 rng(static_cast<unsigned>(std::random_device{}()));
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(rng);
}

} // namespace

BattleSystem::BattleSystem(Party* party)
    : m_party(party),
      m_currentActor(nullptr),
      m_state(State::FILLING_GAUGES),
      m_battleResult(BattleResult::ONGOING) {}

BattleSystem::~BattleSystem() = default;

void BattleSystem::startBattle(const std::vector<std::unique_ptr<Enemy>>& enemies) {
    m_battleResult = BattleResult::ONGOING;
    m_state = State::FILLING_GAUGES;
    m_currentActor = nullptr;
    while (!m_readyQueue.empty()) {
        m_readyQueue.pop();
    }

    buildCombatantList(enemies);

    std::cout << "A battle has started!" << std::endl;
}

void BattleSystem::buildCombatantList(const std::vector<std::unique_ptr<Enemy>>& enemies) {
    m_combatants.clear();
    m_enemyCombatants.clear();

    if (m_party) {
        for (const auto& member : m_party->getMembers()) {
            if (member && member->isAlive()) {
                member->resetAtb();
                member->setDefending(false);
                m_combatants.push_back(member.get());
            }
        }
    }

    for (const auto& enemy : enemies) {
        if (enemy && enemy->isAlive()) {
            enemy->resetAtb();
            enemy->setDefending(false);
            m_combatants.push_back(enemy.get());
            m_enemyCombatants.push_back(enemy.get());
        }
    }
}

void BattleSystem::update(float deltaTime) {
    if (m_battleResult != BattleResult::ONGOING) {
        m_state = State::ENDED;
        return;
    }

    // Tick status effects and fill gauges for all living combatants.
    for (auto* combatant : m_combatants) {
        if (!combatant || !combatant->isAlive()) {
            continue;
        }
        combatant->updateStatusEffects(deltaTime);

        // Don't fill ATB while someone is waiting for input or executing.
        if (m_state != State::FILLING_GAUGES) {
            continue;
        }

        if (!combatant->isAtbFull()) {
            float fill = ATB_FILL_BASE * static_cast<float>(combatant->getSpeed()) * deltaTime;
            // Apply slow/status modifiers here if needed.
            combatant->fillAtb(fill);
        }

        if (combatant->isAtbFull()) {
            m_readyQueue.push(combatant);
        }
    }

    // Remove defeated enemies from the active combatant list after status ticks.
    removeDefeatedEnemies();
    checkBattleEnd();
    if (m_battleResult != BattleResult::ONGOING) {
        m_state = State::ENDED;
        return;
    }

    if (m_state == State::FILLING_GAUGES) {
        processReadyCombatants();
    }
}

void BattleSystem::processReadyCombatants() {
    if (m_readyQueue.empty()) {
        return;
    }

    // Pop the next ready combatant. Skip combatants that died while waiting.
    Combatant* actor = nullptr;
    while (!m_readyQueue.empty() && actor == nullptr) {
        Combatant* candidate = m_readyQueue.front();
        m_readyQueue.pop();
        if (candidate && candidate->isAlive()) {
            actor = candidate;
        }
    }

    if (!actor) {
        return;
    }

    // Reset defending flag at the start of a new turn.
    actor->setDefending(false);

    if (actor->isPlayer()) {
        m_currentActor = actor;
        m_state = State::WAITING_PLAYER_ACTION;
        std::cout << "Waiting for " << actor->getName() << "'s command." << std::endl;
    } else {
        m_state = State::EXECUTING;
        executeEnemyTurn(actor);
        actor->resetAtb();
        m_state = State::FILLING_GAUGES;
    }
}

bool BattleSystem::submitPlayerAction(BattleAction action, int selectionIndex) {
    if (m_state != State::WAITING_PLAYER_ACTION || !m_currentActor || !m_currentActor->isPlayer()) {
        return false;
    }

    m_state = State::EXECUTING;
    executePlayerAction(m_currentActor, action, selectionIndex);

    m_currentActor->resetAtb();
    m_currentActor = nullptr;
    m_state = State::FILLING_GAUGES;

    removeDefeatedEnemies();
    checkBattleEnd();
    return true;
}

void BattleSystem::executeEnemyTurn(Combatant* enemy) {
    if (!enemy || !enemy->isAlive()) {
        return;
    }

    std::cout << "--- " << enemy->getName() << "'s turn ---" << std::endl;

    // Simple AI: attack the first living party member.
    // More advanced enemies could use decideAction()/spells later.
    Combatant* target = getFirstLivingPlayer();
    if (target) {
        executeAttack(enemy, target);
    }
}

void BattleSystem::executePlayerAction(Combatant* actor, BattleAction action, int selectionIndex) {
    if (!actor || !actor->isAlive()) {
        return;
    }

    std::cout << "--- " << actor->getName() << "'s turn ---" << std::endl;

    switch (action) {
        case BattleAction::ATTACK: {
            Combatant* target = getFirstLivingEnemy();
            if (target) {
                executeAttack(actor, target);
            }
            break;
        }
        case BattleAction::MAGIC: {
            Player* player = static_cast<Player*>(actor);
            const auto& spells = player->getSpells();
            if (selectionIndex < 0 || selectionIndex >= static_cast<int>(spells.size())) {
                std::cout << "Invalid spell selection." << std::endl;
                return;
            }
            Spell* spell = spells[selectionIndex].get();
            Combatant* target = selectAutoTarget(actor, spell);
            if (target) {
                executeMagic(actor, selectionIndex, target);
            }
            break;
        }
        case BattleAction::ITEM: {
            Player* player = static_cast<Player*>(actor);
            const auto& items = player->getInventory();
            if (selectionIndex < 0 || selectionIndex >= static_cast<int>(items.size())) {
                std::cout << "Invalid item selection." << std::endl;
                return;
            }
            executeItem(actor, selectionIndex, actor);
            break;
        }
        case BattleAction::DEFEND:
            executeDefend(actor);
            break;
        case BattleAction::FLEE:
            executeFlee();
            break;
    }
}

void BattleSystem::executeAttack(Combatant* actor, Combatant* target) {
    if (!actor || !target || !target->isAlive()) {
        return;
    }

    int baseDamage = actor->getStrength();
    if (actor->isPlayer()) {
        // TODO: weapon damage can be added here.
    }

    int damage = baseDamage;
    if (target->isDefending()) {
        damage = damage / 2;
    }

    std::cout << actor->getName() << " attacks " << target->getName()
              << " for " << damage << " damage!" << std::endl;
    target->takeDamage(damage);
}

void BattleSystem::executeMagic(Combatant* actor, int spellIndex, Combatant* target) {
    Player* player = static_cast<Player*>(actor);
    Spell* spell = player->getSpells()[spellIndex].get();
    if (target->isEnemy()) {
        spell->cast(player, static_cast<Enemy*>(target));
    } else {
        spell->cast(player, static_cast<Player*>(target));
    }
}

void BattleSystem::executeItem(Combatant* actor, int itemIndex, Combatant* target) {
    Player* player = static_cast<Player*>(actor);
    Item* item = player->getInventory()[itemIndex].get();
    std::cout << player->getName() << " uses " << item->getName() << " on " << target->getName() << std::endl;
    item->use(static_cast<Player*>(target));
}

void BattleSystem::executeDefend(Combatant* actor) {
    if (!actor) return;
    actor->setDefending(true);
    std::cout << actor->getName() << " takes a defensive stance." << std::endl;
}

void BattleSystem::executeFlee() {
    if (rollFleeChance() == 0) {
        std::cout << "The party successfully fled!" << std::endl;
        m_battleResult = BattleResult::PLAYER_FLED;
        m_state = State::ENDED;
    } else {
        std::cout << "The party failed to flee!" << std::endl;
    }
}

Combatant* BattleSystem::selectAutoTarget(Combatant* actor, Spell* spell) const {
    if (!spell) {
        return nullptr;
    }
    Spell::SpellType type = spell->getType();
    if (type == Spell::SpellType::HEAL || type == Spell::SpellType::BUFF) {
        return actor;
    }
    return getFirstLivingEnemy();
}

Combatant* BattleSystem::getFirstLivingEnemy() const {
    for (auto* enemy : m_enemyCombatants) {
        if (enemy && enemy->isAlive()) {
            return enemy;
        }
    }
    return nullptr;
}

Combatant* BattleSystem::getFirstLivingPlayer() const {
    if (!m_party) {
        return nullptr;
    }
    for (const auto& member : m_party->getMembers()) {
        if (member && member->isAlive()) {
            return member.get();
        }
    }
    return nullptr;
}

void BattleSystem::removeDefeatedEnemies() {
    auto removeIfDead = [](Combatant* c) { return !c || !c->isAlive(); };
    m_enemyCombatants.erase(
        std::remove_if(m_enemyCombatants.begin(), m_enemyCombatants.end(), removeIfDead),
        m_enemyCombatants.end());
    m_combatants.erase(
        std::remove_if(m_combatants.begin(), m_combatants.end(), removeIfDead),
        m_combatants.end());
}

void BattleSystem::checkBattleEnd() {
    if (m_battleResult != BattleResult::ONGOING) {
        return;
    }

    bool enemiesAlive = !m_enemyCombatants.empty();
    bool playersAlive = false;
    if (m_party) {
        playersAlive = m_party->countAlive() > 0;
    }

    if (!playersAlive) {
        m_battleResult = BattleResult::PLAYER_LOSE;
        m_state = State::ENDED;
        std::cout << "The party has been defeated!" << std::endl;
    } else if (!enemiesAlive) {
        m_battleResult = BattleResult::PLAYER_WIN;
        m_state = State::ENDED;
        std::cout << "Victory!" << std::endl;
    }
}
