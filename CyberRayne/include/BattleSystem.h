#pragma once

#include <vector>
#include <memory>
#include <queue>

#include <string>
#include <utility>

#include "Inventory.h"

class Party;
class Enemy;
class Combatant;
class Player;
class Ability;
class Item;

class BattleSystem {
public:
    enum class BattleAction {
        ATTACK,
        ABILITY,
        ITEM,
        DEFEND,
        FLEE
    };

    // A fully specified action. Replaces the old single overloaded
    // selectionIndex, which had to mean "spell index" or "item index"
    // depending on the action and could not carry a target at all.
    struct ActionRequest {
        BattleAction action = BattleAction::ATTACK;
        int abilityIndex = -1;  // Index into the actor's ability list
        int itemIndex = -1;     // Index into the party bag's consumable stacks
        // Index into the relevant side; -1 auto-targets. For items it indexes
        // every party member, living or not, since some items raise the fallen.
        int targetIndex = -1;

        static ActionRequest attack(int targetIndex = -1);
        static ActionRequest ability(int abilityIndex, int targetIndex = -1);
        static ActionRequest item(int itemIndex, int targetIndex = -1);
        static ActionRequest defend();
        static ActionRequest flee();
    };

    enum class BattleResult {
        ONGOING,
        PLAYER_WIN,
        PLAYER_LOSE,
        PLAYER_FLED
    };

    enum class State {
        FILLING_GAUGES,
        WAITING_PLAYER_ACTION,
        EXECUTING,
        ENDED
    };

    // Something that happened in the fight, for the presentation layer to
    // show. The logic resolves instantly; the screen plays these back.
    struct BattleEvent {
        enum class Kind {
            ACTION,    // subject acted; text names the action
            DAMAGE,    // subject lost `amount` HP
            HEAL,      // subject gained `amount` HP
            DEFEATED,  // subject went down
            MESSAGE    // text only (stunned, failed to flee)
        };
        Kind kind = Kind::MESSAGE;
        Combatant* subject = nullptr;
        int amount = 0;
        std::string text;
    };

    explicit BattleSystem(Party* party);
    ~BattleSystem();

    void startBattle(const std::vector<std::unique_ptr<Enemy>>& enemies);

    // Advance battle simulation by deltaTime. Call every frame.
    void update(float deltaTime);

    // Submit an action for the currently waiting player character. Returns
    // false if nobody is awaiting input, or if the action is invalid — an
    // unaffordable ability is rejected here rather than silently consuming the
    // turn, so the player can pick something else.
    bool submitPlayerAction(const ActionRequest& request);
    // Convenience for actions that need no selection.
    bool submitPlayerAction(BattleAction action);

    // Living combatants on each side, in stable order. These are the lists
    // ActionRequest::targetIndex refers to.
    std::vector<Combatant*> getLivingEnemies() const;
    std::vector<Combatant*> getLivingAllies() const;

    // Everything that happened since the last call, oldest first. Clears the
    // queue. Subjects stay valid for the length of the battle.
    std::vector<BattleEvent> takeEvents();

    // Why the last submitPlayerAction was rejected, for UI feedback.
    const std::string& getLastRejectionReason() const { return m_lastRejection; }

    // Query the character whose turn is currently waiting for player input.
    Combatant* getCurrentActor() const { return m_currentActor; }

    State getState() const { return m_state; }
    BattleResult getResult() const { return m_battleResult; }

    // Total XP and gold awarded from defeated enemies this battle.
    int getExperienceEarned() const { return m_expEarned; }
    int getGoldEarned() const { return m_goldEarned; }

    // Accessors for UI rendering
    const std::vector<Combatant*>& getAllCombatants() const { return m_combatants; }
    const std::vector<Combatant*>& getEnemies() const { return m_enemyCombatants; }
    Party* getParty() const { return m_party; }

private:
    void buildCombatantList(const std::vector<std::unique_ptr<Enemy>>& enemies);
    void fillAtbGauges(float deltaTime);
    void processReadyCombatants();
    void executeEnemyTurn(Combatant* enemy);
    void executePlayerAction(Combatant* actor, const ActionRequest& request);

    void executeAttack(Combatant* actor, Combatant* target);
    void executeItem(Combatant* actor, int itemIndex, Player* target);
    // The itemIndex-th consumable stack in the party bag, or nullptr.
    const Inventory::Stack* getBattleItem(int itemIndex) const;
    // targetIndex is a party member index; -1 means the actor themselves.
    Player* getItemTarget(Combatant* actor, int targetIndex) const;
    void executeDefend(Combatant* actor);
    void executeFlee();

    // Expand an ability's target mode into the concrete combatants it hits.
    // targetIndex selects among the living combatants on the relevant side for
    // single-target abilities; -1 falls back to the first living candidate.
    std::vector<Combatant*> resolveTargets(Combatant* actor, const Ability& ability,
                                           int targetIndex) const;

    Combatant* getFirstLivingEnemy() const;
    Combatant* getFirstLivingPlayer() const;
    // Index into getLivingAllies() of the party member an enemy goes for.
    int pickEnemyTargetIndex() const;

    void checkBattleEnd();

    // HP changes are detected by comparing before and after, so every source
    // (attacks, abilities, items, poison, regen) is reported without each one
    // having to remember to.
    using HealthSnapshot = std::vector<std::pair<Combatant*, int>>;
    HealthSnapshot snapshotHealth() const;
    void emitHealthChanges(const HealthSnapshot& before);
    void emitAction(Combatant* actor, const std::string& text);
    void removeDefeatedEnemies();

    Party* m_party;
    std::vector<Combatant*> m_combatants;      // All participants (party + enemies)
    std::vector<Combatant*> m_enemyCombatants; // Enemies only
    std::queue<Combatant*> m_readyQueue;
    Combatant* m_currentActor;

    State m_state;
    BattleResult m_battleResult;
    int m_expEarned;
    int m_goldEarned;
    std::vector<Combatant*> m_rewardedEnemies;
    std::string m_lastRejection;
    std::vector<BattleEvent> m_events;
};
