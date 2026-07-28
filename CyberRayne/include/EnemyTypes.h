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

// Cyberpunk District Enemies
class StreetThug : public Enemy {
public:
    StreetThug(int level);
protected:
    void initializeStats() override;
};

class SecurityDrone : public Enemy {
public:
    SecurityDrone(int level);
protected:
    void initializeStats() override;
};

// Hostile intrusion countermeasures. Named to avoid colliding with the
// player's NETRUNNER class.
class BlackICE : public Enemy {
public:
    BlackICE(int level);
protected:
    void initializeStats() override;
};

class CombatCyborg : public Enemy {
public:
    CombatCyborg(int level);
protected:
    void initializeStats() override;
};

class CorporateEnforcer : public Enemy {
public:
    CorporateEnforcer(int level);
protected:
    void initializeStats() override;
};
