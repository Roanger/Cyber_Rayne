#include <iostream>
#include "../../include/EnemyTypes.h"

int main() {
    std::cout << "Enemy Types Test" << std::endl;
    
    // Test Forest Enemies
    std::cout << "\n--- Forest Enemies ---" << std::endl;
    Goblin goblin(1);
    std::cout << goblin.getName() << " Level 1 - HP: " << goblin.getHealth() << "/" << goblin.getMaxHealth() 
              << ", STR: " << goblin.getStrength() << ", SPD: " << goblin.getSpeed() << std::endl;
    
    Wolf wolf(2);
    std::cout << wolf.getName() << " Level 2 - HP: " << wolf.getHealth() << "/" << wolf.getMaxHealth() 
              << ", STR: " << wolf.getStrength() << ", SPD: " << wolf.getSpeed() << std::endl;
    
    Treant treant(3);
    std::cout << treant.getName() << " Level 3 - HP: " << treant.getHealth() << "/" << treant.getMaxHealth() 
              << ", STR: " << treant.getStrength() << ", SPD: " << treant.getSpeed() << std::endl;
    
    // Test Desert Enemies
    std::cout << "\n--- Desert Enemies ---" << std::endl;
    SandScorpion scorpion(1);
    std::cout << scorpion.getName() << " Level 1 - HP: " << scorpion.getHealth() << "/" << scorpion.getMaxHealth() 
              << ", STR: " << scorpion.getStrength() << ", SPD: " << scorpion.getSpeed() << std::endl;
    
    DesertBandit bandit(2);
    std::cout << bandit.getName() << " Level 2 - HP: " << bandit.getHealth() << "/" << bandit.getMaxHealth() 
              << ", STR: " << bandit.getStrength() << ", SPD: " << bandit.getSpeed() << std::endl;
    
    // Test Mountain Enemies
    std::cout << "\n--- Mountain Enemies ---" << std::endl;
    MountainLion lion(1);
    std::cout << lion.getName() << " Level 1 - HP: " << lion.getHealth() << "/" << lion.getMaxHealth() 
              << ", STR: " << lion.getStrength() << ", SPD: " << lion.getSpeed() << std::endl;
    
    StoneGolem golem(2);
    std::cout << golem.getName() << " Level 2 - HP: " << golem.getHealth() << "/" << golem.getMaxHealth() 
              << ", STR: " << golem.getStrength() << ", SPD: " << golem.getSpeed() << std::endl;
    
    // Test Swamp Enemies
    std::cout << "\n--- Swamp Enemies ---" << std::endl;
    Zombie zombie(1);
    std::cout << zombie.getName() << " Level 1 - HP: " << zombie.getHealth() << "/" << zombie.getMaxHealth() 
              << ", STR: " << zombie.getStrength() << ", SPD: " << zombie.getSpeed() << std::endl;
    
    PoisonToad toad(2);
    std::cout << toad.getName() << " Level 2 - HP: " << toad.getHealth() << "/" << toad.getMaxHealth() 
              << ", STR: " << toad.getStrength() << ", SPD: " << toad.getSpeed() << std::endl;
    
    std::cout << "\nEnemy types test completed successfully." << std::endl;
    return 0;
}
