#include "../../include/EncounterTable.h"
#include "../../include/EnemyTypes.h"
#include "../../include/Enemy.h"
#include <random>
#include <algorithm>

namespace {
std::mt19937& getRng() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}
} // namespace

EncounterTable::EncounterTable() : m_zoneName("Unknown") {}

EncounterTable::EncounterTable(const std::string& zoneName) : m_zoneName(zoneName) {}

void EncounterTable::addEntry(const std::string& enemyTypeName, int weight, int level) {
    m_entries.push_back({enemyTypeName, weight, level});
}

int EncounterTable::getTotalWeight() const {
    int total = 0;
    for (const auto& entry : m_entries) {
        total += entry.weight;
    }
    return total;
}

const EncounterEntry* EncounterTable::rollEncounter() const {
    if (m_entries.empty()) {
        return nullptr;
    }

    int total = getTotalWeight();
    if (total <= 0) {
        return nullptr;
    }

    auto& rng = getRng();
    std::uniform_int_distribution<int> dist(1, total);
    int roll = dist(rng);

    int accumulated = 0;
    for (const auto& entry : m_entries) {
        accumulated += entry.weight;
        if (roll <= accumulated) {
            return &entry;
        }
    }

    return &m_entries.back();
}

std::unique_ptr<Enemy> EncounterTable::spawnEnemy(const EncounterEntry& entry) const {
    const std::string& name = entry.enemyTypeName;
    int level = entry.level;

    if (name == "StreetThug") {
        return std::make_unique<StreetThug>(level);
    } else if (name == "SecurityDrone") {
        return std::make_unique<SecurityDrone>(level);
    } else if (name == "BlackICE") {
        return std::make_unique<BlackICE>(level);
    } else if (name == "CombatCyborg") {
        return std::make_unique<CombatCyborg>(level);
    } else if (name == "CorporateEnforcer") {
        return std::make_unique<CorporateEnforcer>(level);
    } else if (name == "Goblin") {
        return std::make_unique<Goblin>(level);
    } else if (name == "Wolf") {
        return std::make_unique<Wolf>(level);
    } else if (name == "Treant") {
        return std::make_unique<Treant>(level);
    } else if (name == "SandScorpion") {
        return std::make_unique<SandScorpion>(level);
    } else if (name == "DesertBandit") {
        return std::make_unique<DesertBandit>(level);
    } else if (name == "MountainLion") {
        return std::make_unique<MountainLion>(level);
    } else if (name == "StoneGolem") {
        return std::make_unique<StoneGolem>(level);
    } else if (name == "Zombie") {
        return std::make_unique<Zombie>(level);
    } else if (name == "PoisonToad") {
        return std::make_unique<PoisonToad>(level);
    }

    return nullptr;
}

std::vector<std::unique_ptr<Enemy>> EncounterTable::spawnEncounter(int minEnemies, int maxEnemies) const {
    std::vector<std::unique_ptr<Enemy>> result;

    if (m_entries.empty()) {
        return result;
    }

    auto& rng = getRng();
    int count = minEnemies;
    if (maxEnemies > minEnemies) {
        std::uniform_int_distribution<int> dist(minEnemies, maxEnemies);
        count = dist(rng);
    }

    for (int i = 0; i < count; ++i) {
        const EncounterEntry* entry = rollEncounter();
        if (entry) {
            auto enemy = spawnEnemy(*entry);
            if (enemy) {
                result.push_back(std::move(enemy));
            }
        }
    }

    return result;
}
