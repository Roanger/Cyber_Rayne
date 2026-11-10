#pragma once

#include <memory>

class QuestLog;
class World;
class Party;

// Builds the Phase 4 content: quest chain, NPC dialogues, and encounter setup.
// Gameplay logic only — no rendering dependencies.
class ContentFactory {
public:
    // Create and register the main quest chain in the quest log.
    // Returns the quest ID of the first quest.
    static int createQuestChain(QuestLog* questLog);

    // Put the new-game supplies in the party's bag.
    static void grantStartingKit(Party* party);

    // Attach dialogue trees to NPCs placed in the world's maps.
    static void attachNPCDialogues(World* world);
};
