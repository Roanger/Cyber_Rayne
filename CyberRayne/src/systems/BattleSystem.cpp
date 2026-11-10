#include "../../include/BattleSystem.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include "../../include/Combatant.h"
#include "../../include/Ability.h"
#include "../../include/AbilityRegistry.h"
#include "../../include/Item.h"
#include "../../include/StatusEffect.h"
#include <iostream>
#include <algorithm>
#include <random>

namespace {

// Scale speed to a reasonable ATB fill rate.
// 100 speed ~= 5 seconds to fill the gauge (at 60 FPS ~ 300 frames).
constexpr float ATB_FILL_BASE = 0.005f;

// Percent chance an enemy with an affordable ability uses it instead of a
// basic attack.
constexpr int ENEMY_ABILITY_CHANCE = 40;

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
      m_battleResult(BattleResult::ONGOING),
      m_expEarned(0),
      m_goldEarned(0) {}

BattleSystem::~BattleSystem() = default;

void BattleSystem::startBattle(const std::vector<std::unique_ptr<Enemy>>& enemies) {
    m_battleResult = BattleResult::ONGOING;
    m_state = State::FILLING_GAUGES;
    m_currentActor = nullptr;
    m_expEarned = 0;
    m_goldEarned = 0;
    m_lastRejection.clear();
    // Stale pointers here would leak into the next battle's reward dedupe.
    m_rewardedEnemies.clear();
    m_events.clear();
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
            // Grant the kit for this enemy type. learnAbility is idempotent,
            // so re-entering battle with the same enemy is harmless.
            for (const Ability* ability : AbilityRegistry::forEnemy(enemy->getName())) {
                enemy->learnAbility(ability);
            }
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

    const HealthSnapshot before = snapshotHealth();

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

        bool wasFull = combatant->isAtbFull();
        if (!wasFull) {
            float fill = ATB_FILL_BASE * static_cast<float>(combatant->getSpeed()) * deltaTime;
            fill *= combatant->getAtbSpeedMultiplier();  // SLOW
            combatant->fillAtb(fill);

            // Enqueue only on the transition to full. Pushing every frame while
            // a combatant waits to act floods the queue with duplicates.
            if (combatant->isAtbFull()) {
                m_readyQueue.push(combatant);
            }
        }
    }

    // Remove defeated enemies from the active combatant list after status ticks.
    // Report poison and regen ticks before anyone acts, so a tick and a hit
    // on the same frame show as two numbers.
    emitHealthChanges(before);

    removeDefeatedEnemies();
    checkBattleEnd();
    if (m_battleResult != BattleResult::ONGOING) {
        m_state = State::ENDED;
        return;
    }

    if (m_state == State::FILLING_GAUGES) {
        const HealthSnapshot beforeTurn = snapshotHealth();
        processReadyCombatants();
        emitHealthChanges(beforeTurn);
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

    // A stunned combatant forfeits the turn it just earned.
    if (actor->isStunned()) {
        std::cout << actor->getName() << " is stunned and loses the turn!" << std::endl;
        m_events.push_back({BattleEvent::Kind::MESSAGE, actor, 0, "Stunned"});
        actor->resetAtb();
        return;
    }

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

BattleSystem::ActionRequest BattleSystem::ActionRequest::attack(int targetIndex) {
    ActionRequest r;
    r.action = BattleAction::ATTACK;
    r.targetIndex = targetIndex;
    return r;
}

BattleSystem::ActionRequest BattleSystem::ActionRequest::ability(int abilityIndex, int targetIndex) {
    ActionRequest r;
    r.action = BattleAction::ABILITY;
    r.abilityIndex = abilityIndex;
    r.targetIndex = targetIndex;
    return r;
}

BattleSystem::ActionRequest BattleSystem::ActionRequest::item(int itemIndex, int targetIndex) {
    ActionRequest r;
    r.action = BattleAction::ITEM;
    r.itemIndex = itemIndex;
    r.targetIndex = targetIndex;
    return r;
}

BattleSystem::ActionRequest BattleSystem::ActionRequest::defend() {
    ActionRequest r;
    r.action = BattleAction::DEFEND;
    return r;
}

BattleSystem::ActionRequest BattleSystem::ActionRequest::flee() {
    ActionRequest r;
    r.action = BattleAction::FLEE;
    return r;
}

std::vector<Combatant*> BattleSystem::getLivingEnemies() const {
    std::vector<Combatant*> living;
    for (Combatant* c : m_enemyCombatants) {
        if (c && c->isAlive()) living.push_back(c);
    }
    return living;
}

std::vector<Combatant*> BattleSystem::getLivingAllies() const {
    std::vector<Combatant*> living;
    for (Combatant* c : m_combatants) {
        if (c && c->isPlayer() && c->isAlive()) living.push_back(c);
    }
    return living;
}

bool BattleSystem::submitPlayerAction(BattleAction action) {
    ActionRequest request;
    request.action = action;
    return submitPlayerAction(request);
}

bool BattleSystem::submitPlayerAction(const ActionRequest& request) {
    m_lastRejection.clear();

    if (m_state != State::WAITING_PLAYER_ACTION || !m_currentActor || !m_currentActor->isPlayer()) {
        m_lastRejection = "No character is waiting for a command.";
        return false;
    }

    // Validate before committing the turn. The old code reset the ATB gauge
    // first and spent mana inside the cast, so an unaffordable ability burned
    // the turn for nothing.
    if (request.action == BattleAction::ABILITY) {
        const auto& abilities = m_currentActor->getAbilities();
        if (request.abilityIndex < 0 || request.abilityIndex >= static_cast<int>(abilities.size())) {
            m_lastRejection = "That ability does not exist.";
            return false;
        }
        const Ability* ability = abilities[request.abilityIndex];
        if (!ability) {
            m_lastRejection = "That ability does not exist.";
            return false;
        }
        if (!ability->canAfford(m_currentActor)) {
            m_lastRejection = "Not enough MP for " + ability->getName() + ".";
            std::cout << m_currentActor->getName() << " lacks the MP for "
                      << ability->getName() << "." << std::endl;
            return false;
        }
        if (resolveTargets(m_currentActor, *ability, request.targetIndex).empty()) {
            m_lastRejection = "No valid target.";
            return false;
        }
    } else if (request.action == BattleAction::ITEM) {
        const Inventory::Stack* stack = getBattleItem(request.itemIndex);
        if (!stack) {
            m_lastRejection = "That item does not exist.";
            return false;
        }
        // Items can target the fallen, so the index covers every member, not
        // just the living. -1 means the actor.
        const Player* target = getItemTarget(m_currentActor, request.targetIndex);
        if (!target || !stack->prototype->canUseOn(target)) {
            m_lastRejection = stack->prototype->getName() + " would have no effect.";
            return false;
        }
    }

    m_state = State::EXECUTING;
    const HealthSnapshot before = snapshotHealth();
    executePlayerAction(m_currentActor, request);
    emitHealthChanges(before);

    m_currentActor->resetAtb();
    m_currentActor = nullptr;
    m_state = State::FILLING_GAUGES;

    removeDefeatedEnemies();
    checkBattleEnd();
    return true;
}

std::vector<Combatant*> BattleSystem::resolveTargets(Combatant* actor, const Ability& ability,
                                                     int targetIndex) const {
    std::vector<Combatant*> targets;
    if (!actor) {
        return targets;
    }

    // "Enemy" is relative to the actor, so the same ability data works for
    // party members and for enemies using it against the party.
    const bool actorIsPlayer = actor->isPlayer();
    std::vector<Combatant*> foes;
    std::vector<Combatant*> friends;
    for (Combatant* c : m_combatants) {
        if (!c || !c->isAlive()) continue;
        if (c->isPlayer() == actorIsPlayer) {
            friends.push_back(c);
        } else {
            foes.push_back(c);
        }
    }

    auto pick = [targetIndex](const std::vector<Combatant*>& side) -> std::vector<Combatant*> {
        if (side.empty()) return {};
        if (targetIndex >= 0 && targetIndex < static_cast<int>(side.size())) {
            return {side[targetIndex]};
        }
        return {side.front()};
    };

    switch (ability.getTarget()) {
        case Ability::Target::SELF:
            targets.push_back(actor);
            break;
        case Ability::Target::SINGLE_ENEMY:
            targets = pick(foes);
            break;
        case Ability::Target::ALL_ENEMIES:
            targets = foes;
            break;
        case Ability::Target::SINGLE_ALLY:
            targets = pick(friends);
            break;
        case Ability::Target::ALL_ALLIES:
            targets = friends;
            break;
    }

    return targets;
}

void BattleSystem::executeEnemyTurn(Combatant* enemy) {
    if (!enemy || !enemy->isAlive()) {
        return;
    }

    std::cout << "--- " << enemy->getName() << "'s turn ---" << std::endl;

    // Use an affordable ability when one is available, otherwise attack.
    // Abilities run through the same path players use.
    std::vector<const Ability*> usable;
    for (const Ability* ability : enemy->getAbilities()) {
        if (ability && ability->canAfford(enemy)) {
            usable.push_back(ability);
        }
    }

    const bool useAbility = !usable.empty() && (std::rand() % 100) < ENEMY_ABILITY_CHANCE;
    if (useAbility) {
        const Ability* chosen = usable[std::rand() % usable.size()];
        // -1 would mean "the first one"; pick someone instead.
        auto targets = resolveTargets(enemy, *chosen, pickEnemyTargetIndex());
        if (!targets.empty()) {
            chosen->payCost(enemy);
            emitAction(enemy, chosen->getName());
            chosen->execute(enemy, targets);
            return;
        }
    }

    const auto players = getLivingAllies();
    if (!players.empty()) {
        executeAttack(enemy, players[static_cast<std::size_t>(pickEnemyTargetIndex())]);
    }
}

int BattleSystem::pickEnemyTargetIndex() const {
    // Enemies spread their attacks across the party at random. They used to
    // always hit the first living member, which was always Rayne.
    const auto players = getLivingAllies();
    if (players.empty()) {
        return -1;
    }
    return std::rand() % static_cast<int>(players.size());
}

void BattleSystem::executePlayerAction(Combatant* actor, const ActionRequest& request) {
    if (!actor || !actor->isAlive()) {
        return;
    }

    std::cout << "--- " << actor->getName() << "'s turn ---" << std::endl;

    switch (request.action) {
        case BattleAction::ATTACK: {
            auto foes = getLivingEnemies();
            Combatant* target = nullptr;
            if (request.targetIndex >= 0 && request.targetIndex < static_cast<int>(foes.size())) {
                target = foes[request.targetIndex];
            } else if (!foes.empty()) {
                target = foes.front();
            }
            if (target) {
                executeAttack(actor, target);
            }
            break;
        }
        case BattleAction::ABILITY: {
            // Already validated in submitPlayerAction, including affordability.
            const Ability* ability = actor->getAbilities()[request.abilityIndex];
            auto targets = resolveTargets(actor, *ability, request.targetIndex);
            ability->payCost(actor);
            emitAction(actor, ability->getName());
            ability->execute(actor, targets);
            break;
        }
        case BattleAction::ITEM: {
            executeItem(actor, request.itemIndex, getItemTarget(actor, request.targetIndex));
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

    // Effective strength folds in the equipped weapon and any attack buff or
    // debuff. Defense is applied by the target inside takeDamage().
    int damage = actor->getEffectiveStrength();
    if (target->isDefending()) {
        damage = damage / 2;
    }

    std::cout << actor->getName() << " attacks " << target->getName()
              << " for " << damage << " damage!" << std::endl;
    emitAction(actor, "Attack");
    target->takeDamage(damage);
}

const Inventory::Stack* BattleSystem::getBattleItem(int itemIndex) const {
    if (!m_party || itemIndex < 0) {
        return nullptr;
    }
    const auto consumables = m_party->getBag().getStacksOfType(Item::ItemType::CONSUMABLE);
    if (itemIndex >= static_cast<int>(consumables.size())) {
        return nullptr;
    }
    return consumables[static_cast<std::size_t>(itemIndex)];
}

Player* BattleSystem::getItemTarget(Combatant* actor, int targetIndex) const {
    if (!m_party) {
        return nullptr;
    }
    if (targetIndex >= 0) {
        return m_party->getMember(static_cast<std::size_t>(targetIndex));
    }
    for (const auto& member : m_party->getMembers()) {
        if (member.get() == actor) {
            return member.get();
        }
    }
    return nullptr;
}

void BattleSystem::executeItem(Combatant* actor, int itemIndex, Player* target) {
    const Inventory::Stack* stack = getBattleItem(itemIndex);
    if (!actor || !stack || !target) {
        return;
    }
    // Copy the ID: consuming the last one frees the stack.
    const std::string itemId = stack->id;
    std::cout << actor->getName() << " uses " << stack->prototype->getName() << " on "
              << target->getName() << std::endl;
    emitAction(actor, stack->prototype->getName());
    stack->prototype->use(target);
    // Fallen members are dropped from the turn order, so someone brought back
    // has to be put into it again or they would stand there for the rest of
    // the fight.
    if (target->isAlive() &&
        std::find(m_combatants.begin(), m_combatants.end(), target) == m_combatants.end()) {
        target->resetAtb();
        target->setDefending(false);
        m_combatants.push_back(target);
    }
    m_party->getBag().remove(itemId, 1);
}

void BattleSystem::executeDefend(Combatant* actor) {
    if (!actor) return;
    actor->setDefending(true);
    emitAction(actor, "Defend");
    std::cout << actor->getName() << " takes a defensive stance." << std::endl;
}

void BattleSystem::executeFlee() {
    if (rollFleeChance() == 0) {
        std::cout << "The party successfully fled!" << std::endl;
        m_battleResult = BattleResult::PLAYER_FLED;
        m_state = State::ENDED;
    } else {
        std::cout << "The party failed to flee!" << std::endl;
        m_events.push_back({BattleEvent::Kind::MESSAGE, m_currentActor, 0, "Couldn't escape"});
    }
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
    // Collect rewards from enemies that are about to be removed.
    for (auto* combatant : m_enemyCombatants) {
        if (combatant && !combatant->isAlive()) {
            // Only count each enemy once.
            if (std::find(m_rewardedEnemies.begin(), m_rewardedEnemies.end(), combatant) == m_rewardedEnemies.end()) {
                auto* enemy = static_cast<Enemy*>(combatant);
                m_expEarned += enemy->getExperienceReward();
                m_goldEarned += enemy->getGoldReward();
                m_rewardedEnemies.push_back(combatant);
            }
        }
    }

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

        // Award XP and gold to all living party members.
        if (m_party && m_expEarned > 0) {
            int share = m_expEarned / m_party->countAlive();
            int remainder = m_expEarned % m_party->countAlive();
            int memberIndex = 0;
            for (const auto& member : m_party->getMembers()) {
                if (member && member->isAlive()) {
                    int award = share + (memberIndex == 0 ? remainder : 0);
                    member->addExperience(award);
                    member->addGold(m_goldEarned / m_party->countAlive());
                    ++memberIndex;
                }
            }
            std::cout << "Party earned " << m_expEarned << " XP and " << m_goldEarned << " gold!" << std::endl;
        }
    }
}

BattleSystem::HealthSnapshot BattleSystem::snapshotHealth() const {
    HealthSnapshot snapshot;
    // Party members are read from the party, not m_combatants, so that someone
    // revived by an item mid-battle is still reported.
    if (m_party) {
        for (const auto& member : m_party->getMembers()) {
            if (member) snapshot.emplace_back(member.get(), member->getHealth());
        }
    }
    for (Combatant* enemy : m_enemyCombatants) {
        if (enemy) snapshot.emplace_back(enemy, enemy->getHealth());
    }
    return snapshot;
}

void BattleSystem::emitHealthChanges(const HealthSnapshot& before) {
    for (const auto& [combatant, oldHealth] : before) {
        const int now = combatant->getHealth();
        if (now < oldHealth) {
            m_events.push_back({BattleEvent::Kind::DAMAGE, combatant, oldHealth - now, {}});
            if (now <= 0) {
                m_events.push_back({BattleEvent::Kind::DEFEATED, combatant, 0, {}});
            }
        } else if (now > oldHealth) {
            m_events.push_back({BattleEvent::Kind::HEAL, combatant, now - oldHealth, {}});
        }
    }
}

void BattleSystem::emitAction(Combatant* actor, const std::string& text) {
    m_events.push_back({BattleEvent::Kind::ACTION, actor, 0, text});
}

std::vector<BattleSystem::BattleEvent> BattleSystem::takeEvents() {
    std::vector<BattleEvent> events;
    events.swap(m_events);
    return events;
}
