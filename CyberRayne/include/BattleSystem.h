#pragma once

#include <vector>
#include <memory>
#include <queue>

class Party;
class Enemy;
class Combatant;
class Spell;
class Item;

class BattleSystem {
public:
    enum class BattleAction {
        ATTACK,
        MAGIC,
        ITEM,
        DEFEND,
        FLEE
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

    explicit BattleSystem(Party* party);
    ~BattleSystem();

    void startBattle(const std::vector<std::unique_ptr<Enemy>>& enemies);

    // Advance battle simulation by deltaTime. Call every frame.
    void update(float deltaTime);

    // Submit an action for the currently waiting player character.
    // Returns false if no player character is currently awaiting input.
    bool submitPlayerAction(BattleAction action, int selectionIndex = 0);

    // Query the character whose turn is currently waiting for player input.
    Combatant* getCurrentActor() const { return m_currentActor; }

    State getState() const { return m_state; }
    BattleResult getResult() const { return m_battleResult; }

    // Accessors for UI rendering
    const std::vector<Combatant*>& getAllCombatants() const { return m_combatants; }
    const std::vector<Combatant*>& getEnemies() const { return m_enemyCombatants; }
    Party* getParty() const { return m_party; }

private:
    void buildCombatantList(const std::vector<std::unique_ptr<Enemy>>& enemies);
    void fillAtbGauges(float deltaTime);
    void processReadyCombatants();
    void executeEnemyTurn(Combatant* enemy);
    void executePlayerAction(Combatant* actor, BattleAction action, int selectionIndex);

    void executeAttack(Combatant* actor, Combatant* target);
    void executeMagic(Combatant* actor, int spellIndex, Combatant* target);
    void executeItem(Combatant* actor, int itemIndex, Combatant* target);
    void executeDefend(Combatant* actor);
    void executeFlee();

    Combatant* selectAutoTarget(Combatant* actor, Spell* spell) const;
    Combatant* getFirstLivingEnemy() const;
    Combatant* getFirstLivingPlayer() const;

    void checkBattleEnd();
    void removeDefeatedEnemies();

    Party* m_party;
    std::vector<Combatant*> m_combatants;      // All participants (party + enemies)
    std::vector<Combatant*> m_enemyCombatants; // Enemies only
    std::queue<Combatant*> m_readyQueue;
    Combatant* m_currentActor;

    State m_state;
    BattleResult m_battleResult;
};
