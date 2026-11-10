#include "../../include/EnemyTypes.h"
#include <cmath>
#include <iostream>

// Forest Biome Enemies
Goblin::Goblin(int level) : Enemy("Goblin", level) {
    // Apply derived stats after base construction because constructor dispatch uses Enemy::initializeStats.
    initializeStats();
}

void Goblin::initializeStats() {
    // Goblins are weak but fast
    m_maxHealth = 40 + (m_level * 8);
    m_health = m_maxHealth;
    m_maxMana = 20 + (m_level * 3);
    m_mana = m_maxMana;
    m_strength = 12 + (m_level * 2);
    m_magic = 5 + m_level;
    m_speed = 18 + (m_level * 3);
    m_defense = 8 + static_cast<int>(m_level * 1.5);
    m_experienceReward = 15 + (m_level * 3);
    m_goldReward = 5 + (m_level * 2);
}

Wolf::Wolf(int level) : Enemy("Wolf", level) {
    // Apply derived stats after base construction because constructor dispatch uses Enemy::initializeStats.
    initializeStats();
}

void Wolf::initializeStats() {
    // Wolves are fast and have good attack
    m_maxHealth = 55 + (m_level * 10);
    m_health = m_maxHealth;
    m_maxMana = 15 + (m_level * 2);
    m_mana = m_maxMana;
    m_strength = 18 + static_cast<int>(m_level * 2.5);
    m_magic = 3 + static_cast<int>(m_level * 0.5);
    m_speed = 22 + static_cast<int>(m_level * 3.5);
    m_defense = 10 + static_cast<int>(m_level * 1.5);
    m_experienceReward = 25 + (m_level * 4);
    m_goldReward = 8 + (m_level * 2);
}

Treant::Treant(int level) : Enemy("Treant", level) {
    // Apply derived stats after base construction because constructor dispatch uses Enemy::initializeStats.
    initializeStats();
}

void Treant::initializeStats() {
    // Treants are slow but have high defense and health
    m_maxHealth = 120 + (m_level * 20);
    m_health = m_maxHealth;
    m_maxMana = 50 + (m_level * 8);
    m_mana = m_maxMana;
    m_strength = 20 + (m_level * 3);
    m_magic = 15 + (m_level * 2);
    m_speed = 5 + static_cast<int>(m_level * 0.5);
    m_defense = 30 + (m_level * 4);
    m_experienceReward = 50 + (m_level * 8);
    m_goldReward = 15 + (m_level * 3);
}

// Desert Biome Enemies
SandScorpion::SandScorpion(int level) : Enemy("Sand Scorpion", level) {
    // Apply derived stats after base construction because constructor dispatch uses Enemy::initializeStats.
    initializeStats();
}

void SandScorpion::initializeStats() {
    // Scorpions are fast and can poison
    m_maxHealth = 35 + (m_level * 7);
    m_health = m_maxHealth;
    m_maxMana = 30 + (m_level * 5);
    m_mana = m_maxMana;
    m_strength = 15 + (m_level * 2);
    m_magic = 8 + static_cast<int>(m_level * 1.5);
    m_speed = 25 + (m_level * 4);
    m_defense = 12 + (m_level * 2);
    m_experienceReward = 20 + (m_level * 3);
    m_goldReward = 7 + (m_level * 2);
}

DesertBandit::DesertBandit(int level) : Enemy("Desert Bandit", level) {
    // Apply derived stats after base construction because constructor dispatch uses Enemy::initializeStats.
    initializeStats();
}

void DesertBandit::initializeStats() {
    // Bandits are balanced enemies
    m_maxHealth = 70 + (m_level * 12);
    m_health = m_maxHealth;
    m_maxMana = 40 + (m_level * 6);
    m_mana = m_maxMana;
    m_strength = 16 + static_cast<int>(m_level * 2.5);
    m_magic = 10 + static_cast<int>(m_level * 1.5);
    m_speed = 15 + (m_level * 2);
    m_defense = 14 + (m_level * 2);
    m_experienceReward = 30 + (m_level * 5);
    m_goldReward = 12 + (m_level * 3);
}

// Mountain Biome Enemies
MountainLion::MountainLion(int level) : Enemy("Mountain Lion", level) {
    // Apply derived stats after base construction because constructor dispatch uses Enemy::initializeStats.
    initializeStats();
}

void MountainLion::initializeStats() {
    // Mountain Lions are fast and have high attack
    m_maxHealth = 65 + (m_level * 11);
    m_health = m_maxHealth;
    m_maxMana = 25 + (m_level * 4);
    m_mana = m_maxMana;
    m_strength = 22 + (m_level * 3);
    m_magic = 5 + (m_level * 1);
    m_speed = 28 + (m_level * 4);
    m_defense = 15 + (m_level * 2);
    m_experienceReward = 35 + (m_level * 6);
    m_goldReward = 10 + (m_level * 3);
}

StoneGolem::StoneGolem(int level) : Enemy("Stone Golem", level) {
    // Apply derived stats after base construction because constructor dispatch uses Enemy::initializeStats.
    initializeStats();
}

void StoneGolem::initializeStats() {
    // Stone Golems are slow but have very high defense and health
    m_maxHealth = 150 + (m_level * 25);
    m_health = m_maxHealth;
    m_maxMana = 80 + (m_level * 12);
    m_mana = m_maxMana;
    m_strength = 25 + static_cast<int>(m_level * 3.5);
    m_magic = 20 + static_cast<int>(m_level * 2.5);
    m_speed = 3 + static_cast<int>(m_level * 0.5);
    m_defense = 40 + (m_level * 5);
    m_experienceReward = 70 + (m_level * 10);
    m_goldReward = 20 + (m_level * 4);
}

// Swamp Biome Enemies
Zombie::Zombie(int level) : Enemy("Zombie", level) {
    // Apply derived stats after base construction because constructor dispatch uses Enemy::initializeStats.
    initializeStats();
}

void Zombie::initializeStats() {
    // Zombies are slow but hard to kill
    m_maxHealth = 80 + (m_level * 15);
    m_health = m_maxHealth;
    m_maxMana = 20 + (m_level * 3);
    m_mana = m_maxMana;
    m_strength = 14 + (m_level * 2);
    m_magic = 5 + (m_level * 1);
    m_speed = 4 + static_cast<int>(m_level * 0.5);
    m_defense = 18 + static_cast<int>(m_level * 2.5);
    m_experienceReward = 25 + (m_level * 4);
    m_goldReward = 3 + (m_level * 1);
}

PoisonToad::PoisonToad(int level) : Enemy("Poison Toad", level) {
    // Apply derived stats after base construction because constructor dispatch uses Enemy::initializeStats.
    initializeStats();
}

void PoisonToad::initializeStats() {
    // Poison Toads are weak but can poison the player
    m_maxHealth = 30 + (m_level * 5);
    m_health = m_maxHealth;
    m_maxMana = 40 + (m_level * 7);
    m_mana = m_maxMana;
    m_strength = 8 + static_cast<int>(m_level * 1.5);
    m_magic = 15 + (m_level * 2);
    m_speed = 12 + static_cast<int>(m_level * 1.5);
    m_defense = 6 + (m_level * 1);
    m_experienceReward = 18 + (m_level * 3);
    m_goldReward = 4 + (m_level * 1);
}

// Cyberpunk District Enemies
StreetThug::StreetThug(int level) : Enemy("Street Thug", level) {
    initializeStats();
}

void StreetThug::initializeStats() {
    m_maxHealth = 45 + (m_level * 9);
    m_health = m_maxHealth;
    m_maxMana = 15 + (m_level * 2);
    m_mana = m_maxMana;
    m_strength = 14 + (m_level * 2);
    m_magic = 3 + m_level;
    m_speed = 12 + (m_level * 2);
    m_defense = 6 + m_level;
    m_experienceReward = 18 + (m_level * 3);
    m_goldReward = 8 + (m_level * 2);
}

SecurityDrone::SecurityDrone(int level) : Enemy("Security Drone", level) {
    initializeStats();
}

void SecurityDrone::initializeStats() {
    m_maxHealth = 50 + (m_level * 9);
    m_health = m_maxHealth;
    m_maxMana = 30 + (m_level * 5);
    m_mana = m_maxMana;
    m_strength = 13 + (m_level * 2);
    m_magic = 10 + static_cast<int>(m_level * 1.5);
    m_speed = 14 + (m_level * 2);
    m_defense = 8 + static_cast<int>(m_level * 1.5);
    m_experienceReward = 25 + (m_level * 4);
    m_goldReward = 10 + (m_level * 2);
}

BlackICE::BlackICE(int level) : Enemy("Black ICE", level) {
    initializeStats();
}

void BlackICE::initializeStats() {
    m_maxHealth = 40 + (m_level * 7);
    m_health = m_maxHealth;
    m_maxMana = 80 + (m_level * 12);
    m_mana = m_maxMana;
    m_strength = 8 + static_cast<int>(m_level * 1.5);
    m_magic = 22 + (m_level * 3);
    m_speed = 13 + (m_level * 2);
    m_defense = 6 + (m_level * 1);
    m_experienceReward = 30 + (m_level * 5);
    m_goldReward = 15 + (m_level * 3);
}

CombatCyborg::CombatCyborg(int level) : Enemy("Combat Cyborg", level) {
    initializeStats();
}

void CombatCyborg::initializeStats() {
    m_maxHealth = 85 + (m_level * 14);
    m_health = m_maxHealth;
    m_maxMana = 40 + (m_level * 6);
    m_mana = m_maxMana;
    m_strength = 19 + static_cast<int>(m_level * 2.5);
    m_magic = 12 + (m_level * 2);
    m_speed = 10 + static_cast<int>(m_level * 1.5);
    m_defense = 13 + (m_level * 2);
    m_experienceReward = 45 + (m_level * 7);
    m_goldReward = 18 + (m_level * 3);
}

CorporateEnforcer::CorporateEnforcer(int level) : Enemy("Corporate Enforcer", level) {
    initializeStats();
}

void CorporateEnforcer::initializeStats() {
    m_maxHealth = 130 + (m_level * 16);
    m_health = m_maxHealth;
    m_maxMana = 50 + (m_level * 8);
    m_mana = m_maxMana;
    m_strength = 22 + (m_level * 3);
    m_magic = 15 + static_cast<int>(m_level * 2);
    m_speed = 9 + static_cast<int>(m_level * 1.5);
    m_defense = 15 + static_cast<int>(m_level * 2.5);
    m_experienceReward = 60 + (m_level * 9);
    m_goldReward = 25 + (m_level * 4);
}
