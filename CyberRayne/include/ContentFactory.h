#pragma once

#include <memory>

class QuestLog;
class World;

// Builds the Phase 4 content: quest chain, NPC dialogues, and encounter setup.
// Gameplay logic only — no rendering dependencies.
class ContentFactory {
public:
    // Create and register the main quest chain in the quest log.
    // Returns the quest ID of the first quest.
    static int createQuestChain(QuestLog* questLog);

    // Attach dialogue trees to NPCs placed in the world's maps.
    static void attachNPCDialogues(World* world);
};
