#include "../../include/BattleSystem.h"
#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include "../../include/UIManager.h"
#include <iostream>
#include <algorithm>

BattleSystem::BattleSystem(Player* player, UIManager* uiManager)
    : m_player(player),
      m_uiManager(uiManager),
      m_playerTurn(true),
      m_currentEnemyIndex(0),
      m_battleResult(BattleResult::ONGOING) {}

BattleSystem::~BattleSystem() {}

void BattleSystem::startBattle(const std::vector<std::unique_ptr<Enemy>>& enemies) {
    m_battleResult = BattleResult::ONGOING;
    m_playerTurn = true;
    m_currentEnemyIndex = 0;
    
    // Copy enemy references
    m_enemies.clear();
    for (const auto& enemy : enemies) {
        m_enemies.push_back(enemy.get());
    }
    
    std::cout << "A battle has started!" << std::endl;
    displayBattleStatus();
}

void BattleSystem::update() {
    if (m_battleResult != BattleResult::ONGOING) {
        return;
    }
    
    if (m_playerTurn) {
        // Poll UIManager for action
        if (m_uiManager->hasActionSelected()) {
            UIManager::BattleAction action = m_uiManager->getSelectedAction();
            
            switch (action) {
                case UIManager::BattleAction::ATTACK:
                    playerAttack();
                    m_playerTurn = false;
                    break;
                case UIManager::BattleAction::MAGIC:
                    playerUseMagic(m_uiManager->getSelectedSpellIndex());
                    m_playerTurn = false;
                    break;
                case UIManager::BattleAction::ITEM:
                    playerUseItem(0); // Placeholder
                    m_playerTurn = false;
                    break;
                case UIManager::BattleAction::DEFEND:
                    playerDefend();
                    m_playerTurn = false;
                    break;
                case UIManager::BattleAction::FLEE:
                    playerFlee();
                    // Flee might not always succeed, so check result
                    if (m_battleResult == BattleResult::ONGOING) {
                        m_playerTurn = false;
                    }
                    break;
                default:
                    break;
            }
            
            // Reset UI action after processing
            m_uiManager->resetAction();
        }
    } else {
        enemyTurn();
        m_playerTurn = true;
    }
}

void BattleSystem::playerAttack() {
    if (m_enemies.empty()) return;
    
    // Attack the first enemy for now
    Enemy* target = m_enemies[0];
    int damage = m_player->getStrength();
    
    std::cout << m_player->getName() << " attacks " << target->getName() 
              << " for " << damage << " damage!" << std::endl;
    
    target->takeDamage(damage);
    
    // Check if enemy is defeated
    if (target->getHealth() <= 0) {
        std::cout << target->getName() << " is defeated!" << std::endl;
        // Remove defeated enemy
        m_enemies.erase(std::remove(m_enemies.begin(), m_enemies.end(), target), m_enemies.end());
        
        // Check for victory
        if (m_enemies.empty()) {
            std::cout << "You win the battle!" << std::endl;
            m_battleResult = BattleResult::PLAYER_WIN;
        }
    }
}

void BattleSystem::playerUseMagic(int spellIndex) {
    // Implementation would go here
    std::cout << "Using magic is not yet implemented." << std::endl;
}

void BattleSystem::playerUseItem(int itemIndex) {
    // Implementation would go here
    std::cout << "Using items is not yet implemented." << std::endl;
}

void BattleSystem::playerDefend() {
    // Implementation would go here
    std::cout << m_player->getName() << " takes a defensive stance." << std::endl;
}

void BattleSystem::playerFlee() {
    // Simple flee chance
    if (rand() % 2 == 0) {  // 50% chance to flee
        std::cout << m_player->getName() << " successfully fled from battle!" << std::endl;
        m_battleResult = BattleResult::PLAYER_FLED;
    } else {
        std::cout << m_player->getName() << " failed to flee!" << std::endl;
    }
}

void BattleSystem::enemyTurn() {
    if (m_enemies.empty() || m_battleResult != BattleResult::ONGOING) {
        return;
    }
    
    // For now, just have the first enemy attack the player
    Enemy* attacker = m_enemies[0];
    int damage = attacker->getStrength();
    
    std::cout << attacker->getName() << " attacks " << m_player->getName() 
              << " for " << damage << " damage!" << std::endl;
    
    m_player->takeDamage(damage);
    
    // Check if player is defeated
    if (m_player->getHealth() <= 0) {
        std::cout << m_player->getName() << " has been defeated!" << std::endl;
        m_battleResult = BattleResult::PLAYER_LOSE;
    }
}

void BattleSystem::displayBattleOptions() {
    std::cout << "\n--- Battle Options ---" << std::endl;
    std::cout << "1. Attack" << std::endl;
    std::cout << "2. Magic" << std::endl;
    std::cout << "3. Item" << std::endl;
    std::cout << "4. Defend" << std::endl;
    std::cout << "5. Flee" << std::endl;
    std::cout << "Choose an action: ";
}

void BattleSystem::displayBattleStatus() {
    std::cout << "\n--- Battle Status ---" << std::endl;
    std::cout << m_player->getName() << " - HP: " << m_player->getHealth() 
              << "/" << m_player->getMaxHealth() << std::endl;
    
    for (size_t i = 0; i < m_enemies.size(); ++i) {
        std::cout << i + 1 << ". " << m_enemies[i]->getName() 
                  << " - HP: " << m_enemies[i]->getHealth() 
                  << "/" << m_enemies[i]->getHealth() << std::endl;
    }
}
