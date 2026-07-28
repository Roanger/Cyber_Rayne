#pragma once

#include <vector>
#include <memory>
#include <string>

class Enemy;

// A weighted entry in an encounter table.
struct EncounterEntry {
    std::string enemyTypeName;  // Identifies which enemy type to spawn.
    int weight;                 // Relative probability weight.
    int level;                  // Level of the spawned enemy.
};

// A table of weighted enemy spawns for a zone.
// Gameplay logic only — no rendering dependencies.
class EncounterTable {
public:
    EncounterTable();
    explicit EncounterTable(const std::string& zoneName);
    ~EncounterTable() = default;

    void addEntry(const std::string& enemyTypeName, int weight, int level);
    const std::vector<EncounterEntry>& getEntries() const { return m_entries; }

    const std::string& getZoneName() const { return m_zoneName; }
    void setZoneName(const std::string& name) { m_zoneName = name; }

    // Chance per player step that a random encounter fires in this zone, 0..1.
    // Zero disables random encounters, leaving only visible on-map enemies.
    float getEncounterRate() const { return m_encounterRate; }
    void setEncounterRate(float rate) { m_encounterRate = rate < 0.0f ? 0.0f : (rate > 1.0f ? 1.0f : rate); }

    int getTotalWeight() const;

    // Roll a random encounter based on weights. Returns the selected entry,
    // or nullptr if the table is empty.
    const EncounterEntry* rollEncounter() const;

    // Spawn a single enemy from a rolled entry. Returns nullptr on unknown type.
    std::unique_ptr<Enemy> spawnEnemy(const EncounterEntry& entry) const;

    // Spawn a full encounter (1-3 enemies) for the zone.
    std::vector<std::unique_ptr<Enemy>> spawnEncounter(int minEnemies, int maxEnemies) const;

    bool isEmpty() const { return m_entries.empty(); }

private:
    std::string m_zoneName;
    std::vector<EncounterEntry> m_entries;
    float m_encounterRate = 0.06f;
};
