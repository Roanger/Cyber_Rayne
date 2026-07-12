#include "../../include/EnemyTypes.h"
#include <iostream>
#include <stdexcept>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
}

int main() {
    Goblin goblin(1);
    Wolf wolf(2);
    Treant treant(3);
    SandScorpion scorpion(1);
    DesertBandit bandit(2);
    MountainLion lion(1);
    StoneGolem golem(2);
    Zombie zombie(1);
    PoisonToad toad(2);

    expect(goblin.getMaxHealth() == 48, "goblin should use derived health stats");
    expect(goblin.getSpeed() == 21, "goblin should use derived speed stats");
    expect(wolf.getMaxHealth() == 75, "wolf should scale health by level");
    expect(treant.getExperienceReward() > goblin.getExperienceReward(), "treant should reward more experience than goblin");
    expect(scorpion.getHealth() == scorpion.getMaxHealth(), "enemy should start at full health");
    expect(bandit.getStrength() > 0, "bandit strength should be positive");
    expect(lion.getSpeed() > 0, "mountain lion speed should be positive");
    expect(golem.getDefense() > wolf.getDefense(), "golem defense should exceed wolf defense");
    expect(zombie.getExperienceReward() > 0, "zombie experience reward should be positive");
    expect(toad.getGoldReward() > 0, "poison toad gold reward should be positive");

    std::cout << "EnemyTypesTest passed" << std::endl;
    return 0;
}
