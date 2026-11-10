#include "../../include/World.h"
#include "../../include/Map.h"
#include "../../include/Tile.h"
#include "../../include/NPC.h"
#include "../../include/EncounterTable.h"
#include "../../include/EnemyTypes.h"
#include "../../include/Enemy.h"
#include "../../include/QuestLog.h"
#include "../../include/Quest.h"
#include "../../include/ContentFactory.h"
#include "../../include/DialogueTree.h"
#include "../../include/DialogueNode.h"
#include "../../include/DialogueSystem.h"
#include "../../include/InputAction.h"
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
} // namespace

int main() {
    // === Test World initialization with cyberpunk districts ===
    World world;
    expect(world.initialize(), "world should initialize");
    expect(world.getMaps().size() == 3, "should have 3 district maps");

    // Verify map names
    expect(world.getMaps()[0]->getName() == "Neo-Kyoto Central", "first map should be Neo-Kyoto Central");
    expect(world.getMaps()[1]->getName() == "Neon Alley", "second map should be Neon Alley");
    expect(world.getMaps()[2]->getName() == "Corporate Plaza", "third map should be Corporate Plaza");

    // Verify starting biome
    expect(world.getCurrentBiome() == World::BiomeType::NEO_KYOTO_CENTRAL, "starting biome should be NEO_KYOTO_CENTRAL");

    // === Test cyberpunk tile types ===
    Map* central = world.getMaps()[0].get();
    expect(central->getWidth() == 30, "central map width should be 30");
    expect(central->getHeight() == 25, "central map height should be 25");

    // Check that concrete tiles are walkable
    Tile* concreteTile = central->getTile(10, 10);
    expect(concreteTile != nullptr, "tile at (10,10) should exist");
    expect(concreteTile->getType() == Tile::TileType::CONCRETE, "tile should be CONCRETE");
    expect(concreteTile->isWalkable(), "concrete should be walkable");

    // Check that metal building tiles are not walkable
    Tile* metalTile = central->getTile(3, 4);
    expect(metalTile != nullptr, "tile at (3,4) should exist");
    expect(metalTile->getType() == Tile::TileType::METAL, "tile should be METAL");
    expect(!metalTile->isWalkable(), "metal building should not be walkable");

    // Check road tiles
    Tile* roadTile = central->getTile(15, 12);
    expect(roadTile != nullptr, "tile at (15,12) should exist");
    expect(roadTile->getType() == Tile::TileType::ROAD, "tile should be ROAD");

    // Check neon tiles
    Tile* neonTile = central->getTile(7, 4);
    expect(neonTile != nullptr, "tile at (7,4) should exist");
    expect(neonTile->getType() == Tile::TileType::NEON, "tile should be NEON");
    expect(!neonTile->isWalkable(), "neon should not be walkable");

    // Check hologram tiles
    Tile* hologramTile = central->getTile(10, 15);
    expect(hologramTile != nullptr, "tile at (10,15) should exist");
    expect(hologramTile->getType() == Tile::TileType::HOLOGRAM, "tile should be HOLOGRAM");
    expect(!hologramTile->isWalkable(), "hologram should not be walkable");

    // Check vending machine tiles
    Tile* vendingTile = central->getTile(14, 10);
    expect(vendingTile != nullptr, "tile at (14,10) should exist");
    expect(vendingTile->getType() == Tile::TileType::VENDING, "tile should be VENDING");
    expect(!vendingTile->isWalkable(), "vending machine should not be walkable");

    // === Test NPCs ===
    expect(central->getNPCs().size() == 3, "central should have 3 NPCs");
    expect(central->getNPCs()[0]->getName() == "Fixer Kaga", "first NPC should be Fixer Kaga");
    expect(central->getNPCs()[1]->getName() == "Street Vendor", "second NPC should be Street Vendor");
    expect(central->getNPCs()[2]->getName() == "Info Broker", "third NPC should be Info Broker");

    Map* alley = world.getMaps()[1].get();
    expect(alley->getNPCs().size() == 2, "alley should have 2 NPCs");

    Map* plaza = world.getMaps()[2].get();
    expect(plaza->getNPCs().size() == 2, "plaza should have 2 NPCs");

    // === Test encounter tables ===
    const EncounterTable* centralTable = world.getEncounterTable(World::BiomeType::NEO_KYOTO_CENTRAL);
    expect(centralTable != nullptr, "central encounter table should exist");
    expect(!centralTable->isEmpty(), "central table should not be empty");
    expect(centralTable->getEntries().size() == 3, "central table should have 3 entries");
    expect(centralTable->getTotalWeight() == 100, "central table total weight should be 100");

    const EncounterTable* alleyTable = world.getEncounterTable(World::BiomeType::NEON_ALLEY);
    expect(alleyTable != nullptr, "alley encounter table should exist");
    expect(!alleyTable->isEmpty(), "alley table should not be empty");

    const EncounterTable* plazaTable = world.getEncounterTable(World::BiomeType::CORPORATE_PLAZA);
    expect(plazaTable != nullptr, "plaza encounter table should exist");
    expect(!plazaTable->isEmpty(), "plaza table should not be empty");

    // Test encounter spawning
    auto enemies = centralTable->spawnEncounter(1, 3);
    expect(!enemies.empty(), "spawning encounter should produce at least 1 enemy");
    expect(enemies.size() <= 3, "should not exceed max enemies");
    for (const auto& enemy : enemies) {
        expect(enemy != nullptr, "spawned enemy should not be null");
        expect(enemy->getHealth() > 0, "spawned enemy should have positive health");
    }

    // === Test cyberpunk enemy types ===
    auto thug = std::make_unique<StreetThug>(1);
    expect(thug->getName() == "Street Thug", "street thug name should match");
    expect(thug->getHealth() > 0, "street thug should have positive health");
    expect(thug->getExperienceReward() > 0, "street thug should give XP");

    auto drone = std::make_unique<SecurityDrone>(2);
    expect(drone->getName() == "Security Drone", "security drone name should match");
    expect(drone->getHealth() > thug->getHealth(), "drone should have more HP than thug at higher level");

    auto blackIce = std::make_unique<BlackICE>(2);
    expect(blackIce->getMagic() > blackIce->getStrength(), "blackIce should have higher magic than strength");

    auto cyborg = std::make_unique<CombatCyborg>(3);
    expect(cyborg->getDefense() > thug->getDefense(), "cyborg should have higher defense than thug");

    auto enforcer = std::make_unique<CorporateEnforcer>(3);
    expect(enforcer->getHealth() > cyborg->getHealth(), "enforcer should have more HP than cyborg");

    // === Test quest chain ===
    QuestLog questLog;
    int firstQuestId = ContentFactory::createQuestChain(&questLog);
    expect(firstQuestId == 100, "first quest ID should be 100");
    expect(questLog.getQuests().size() == 3, "should have 3 quests in the chain");

    // Verify quest 1
    Quest* q1 = questLog.getQuest(100);
    expect(q1 != nullptr, "quest 100 should exist");
    expect(q1->getName() == "The Data Chip", "quest 1 name should match");
    expect(q1->getObjectives().size() == 3, "quest 1 should have 3 objectives");
    expect(q1->getExperienceReward() == 350, "quest 1 XP reward should be 350");
    expect(q1->getState() == Quest::State::NOT_STARTED, "quest 1 should start as NOT_STARTED");

    // Verify quest 2
    Quest* q2 = questLog.getQuest(101);
    expect(q2 != nullptr, "quest 101 should exist");
    expect(q2->getName() == "Corporate Intrusion", "quest 2 name should match");
    expect(q2->getObjectives().size() == 3, "quest 2 should have 3 objectives");

    // Verify quest 3
    Quest* q3 = questLog.getQuest(102);
    expect(q3 != nullptr, "quest 102 should exist");
    expect(q3->getName() == "The Truth", "quest 3 name should match");
    expect(q3->getGoldReward() == 300, "quest 3 gold reward should be 300");

    // Test quest progression
    expect(questLog.startQuest(100), "starting quest 100 should succeed");
    expect(q1->getState() == Quest::State::ACTIVE, "quest 1 should be active");
    expect(questLog.completeObjective(100, 0), "completing objective 0 should succeed");
    expect(questLog.completeObjective(100, 1), "completing objective 1 should succeed");
    expect(questLog.completeObjective(100, 2), "completing objective 2 should succeed");
    expect(q1->getState() == Quest::State::COMPLETED, "quest 1 should be completed");

    // === Test NPC dialogue attachment ===
    ContentFactory::attachNPCDialogues(&world);

    // Fixer Kaga should have a dialogue tree
    NPC* fixer = central->getNPCs()[0].get();
    expect(fixer->hasDialogue(), "Fixer Kaga should have a dialogue tree");
    expect(fixer->isInteractable(), "Fixer Kaga should be interactable");

    // Test the dialogue tree
    DialogueSystem dialogue;
    auto fixerTree = std::make_unique<DialogueTree>(0);

    // Copy the tree structure from Fixer Kaga
    DialogueTree* originalTree = fixer->getDialogueTree();
    expect(originalTree != nullptr, "Fixer Kaga's dialogue tree should not be null");
    expect(originalTree->getRootId() == 0, "Fixer Kaga's tree root should be 0");
    expect(originalTree->getNodeCount() == 3, "Fixer Kaga's tree should have 3 nodes");

    const DialogueNode* rootNode = originalTree->getNode(0);
    expect(rootNode != nullptr, "root node should exist");
    expect(rootNode->getSpeaker() == "Fixer Kaga", "root node speaker should be Fixer Kaga");
    expect(rootNode->hasChoices(), "root node should have choices");
    expect(rootNode->getChoices().size() == 2, "root node should have 2 choices");

    // Info Broker should also have dialogue
    NPC* broker = central->getNPCs()[2].get();
    expect(broker->hasDialogue(), "Info Broker should have a dialogue tree");

    // Exec Tanaka should have dialogue
    NPC* exec = plaza->getNPCs()[0].get();
    expect(exec->hasDialogue(), "Exec Tanaka should have a dialogue tree");

    // Smuggler Ryo should have dialogue
    NPC* smuggler = alley->getNPCs()[0].get();
    expect(smuggler->hasDialogue(), "Smuggler Ryo should have a dialogue tree");

    std::cout << "ContentTest passed" << std::endl;
    return 0;
}
