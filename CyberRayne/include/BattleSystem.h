#pragma once

#include <vector>
#include <memory>

class Player;
class Enemy;
class UIManager;

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

    BattleSystem(Player* player, UIManager* uiManager);
    ~BattleSystem();

    void startBattle(const std::vector<std::unique_ptr<Enemy>>& enemies);
    void update();
    BattleResult getResult() const { return m_battleResult; }

    // Player actions
    void playerAttack();
    void playerUseMagic(int spellIndex);
    void playerUseItem(int itemIndex);
    void playerDefend();
    void playerFlee();

    // Enemy actions
    void enemyTurn();

private:
    void processTurn();
    void displayBattleOptions();
    void displayBattleStatus();

    Player* m_player;
    std::vector<Enemy*> m_enemies;  // References to enemies in the battle
    UIManager* m_uiManager;

    bool m_playerTurn;
    int m_currentEnemyIndex;
    BattleResult m_battleResult;
};
