#pragma once

#include "Enemy.h"
#include <string>

// Forest Biome Enemies
class Goblin : public Enemy {
public:
    Goblin(int level);
protected:
    void initializeStats() override;
};

class Wolf : public Enemy {
public:
    Wolf(int level);
protected:
    void initializeStats() override;
};

class Treant : public Enemy {
public:
    Treant(int level);
protected:
    void initializeStats() override;
};

// Desert Biome Enemies
class SandScorpion : public Enemy {
public:
    SandScorpion(int level);
protected:
    void initializeStats() override;
};

class DesertBandit : public Enemy {
public:
    DesertBandit(int level);
protected:
    void initializeStats() override;
};

// Mountain Biome Enemies
class MountainLion : public Enemy {
public:
    MountainLion(int level);
protected:
    void initializeStats() override;
};

class StoneGolem : public Enemy {
public:
    StoneGolem(int level);
protected:
    void initializeStats() override;
};

// Swamp Biome Enemies
class Zombie : public Enemy {
public:
    Zombie(int level);
protected:
    void initializeStats() override;
};

class PoisonToad : public Enemy {
public:
    PoisonToad(int level);
protected:
    void initializeStats() override;
};
