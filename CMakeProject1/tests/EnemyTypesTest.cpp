#include "../include/EnemyTypes.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Testing Enemy Prototypes..." << std::endl;
    
    // Test Forest Biome Enemies
    std::cout << "\n--- Forest Biome Enemies ---" << std::endl;
    auto goblin = std::make_unique<Goblin>(5);
    std::cout << "Goblin - Health: " << goblin->getHealth() << "/" << goblin->getMaxHealth() 
              << ", Strength: " << goblin->getStrength() 
              << ", Speed: " << goblin->getSpeed()
              << ", XP Reward: " << goblin->getExperienceReward()
              << ", Gold Reward: " << goblin->getGoldReward() << std::endl;
    
    auto wolf = std::make_unique<Wolf>(5);
    std::cout << "Wolf - Health: " << wolf->getHealth() << "/" << wolf->getMaxHealth() 
              << ", Strength: " << wolf->getStrength() 
              << ", Speed: " << wolf->getSpeed()
              << ", XP Reward: " << wolf->getExperienceReward()
              << ", Gold Reward: " << wolf->getGoldReward() << std::endl;
    
    auto treant = std::make_unique<Treant>(5);
    std::cout << "Treant - Health: " << treant->getHealth() << "/" << treant->getMaxHealth() 
              << ", Strength: " << treant->getStrength() 
              << ", Speed: " << treant->getSpeed()
              << ", XP Reward: " << treant->getExperienceReward()
              << ", Gold Reward: " << treant->getGoldReward() << std::endl;
    
    // Test Desert Biome Enemies
    std::cout << "\n--- Desert Biome Enemies ---" << std::endl;
    auto scorpion = std::make_unique<SandScorpion>(5);
    std::cout << "Sand Scorpion - Health: " << scorpion->getHealth() << "/" << scorpion->getMaxHealth() 
              << ", Strength: " << scorpion->getStrength() 
              << ", Speed: " << scorpion->getSpeed()
              << ", XP Reward: " << scorpion->getExperienceReward()
              << ", Gold Reward: " << scorpion->getGoldReward() << std::endl;
    
    auto bandit = std::make_unique<DesertBandit>(5);
    std::cout << "Desert Bandit - Health: " << bandit->getHealth() << "/" << bandit->getMaxHealth() 
              << ", Strength: " << bandit->getStrength() 
              << ", Speed: " << bandit->getSpeed()
              << ", XP Reward: " << bandit->getExperienceReward()
              << ", Gold Reward: " << bandit->getGoldReward() << std::endl;
    
    // Test Mountain Biome Enemies
    std::cout << "\n--- Mountain Biome Enemies ---" << std::endl;
    auto lion = std::make_unique<MountainLion>(5);
    std::cout << "Mountain Lion - Health: " << lion->getHealth() << "/" << lion->getMaxHealth() 
              << ", Strength: " << lion->getStrength() 
              << ", Speed: " << lion->getSpeed()
              << ", XP Reward: " << lion->getExperienceReward()
              << ", Gold Reward: " << lion->getGoldReward() << std::endl;
    
    auto golem = std::make_unique<StoneGolem>(5);
    std::cout << "Stone Golem - Health: " << golem->getHealth() << "/" << golem->getMaxHealth() 
              << ", Strength: " << golem->getStrength() 
              << ", Speed: " << golem->getSpeed()
              << ", XP Reward: " << golem->getExperienceReward()
              << ", Gold Reward: " << golem->getGoldReward() << std::endl;
    
    // Test Swamp Biome Enemies
    std::cout << "\n--- Swamp Biome Enemies ---" << std::endl;
    auto zombie = std::make_unique<Zombie>(5);
    std::cout << "Zombie - Health: " << zombie->getHealth() << "/" << zombie->getMaxHealth() 
              << ", Strength: " << zombie->getStrength() 
              << ", Speed: " << zombie->getSpeed()
              << ", XP Reward: " << zombie->getExperienceReward()
              << ", Gold Reward: " << zombie->getGoldReward() << std::endl;
    
    auto toad = std::make_unique<PoisonToad>(5);
    std::cout << "Poison Toad - Health: " << toad->getHealth() << "/" << toad->getMaxHealth() 
              << ", Strength: " << toad->getStrength() 
              << ", Speed: " << toad->getSpeed()
              << ", XP Reward: " << toad->getExperienceReward()
              << ", Gold Reward: " << toad->getGoldReward() << std::endl;
    
    std::cout << "\nEnemy Prototypes Test Completed Successfully!" << std::endl;
    
    return 0;
}
