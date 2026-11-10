#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

class QuestLog;
class Party;
class DialogueTree;

// Drives the main quest chain: decides what each NPC says, which quests start
// and advance in response to game events, which fixed fights exist, and which
// districts are open. All story state is derived from the QuestLog, so saving
// the quest log saves the story.
//
// Gameplay logic only — no rendering dependencies.
class StoryDirector {
public:
    // Quest IDs, matching ContentFactory::createQuestChain.
    static constexpr int QUEST_DATA_CHIP = 100;
    static constexpr int QUEST_INTRUSION = 101;
    static constexpr int QUEST_TRUTH = 102;

    // Dialogue nodes that change story state when the conversation reaches them.
    static constexpr int NODE_ACCEPT_CHIP_JOB = 100;
    static constexpr int NODE_RETURN_CHIP = 101;
    static constexpr int NODE_CONFRONT_TANAKA = 102;
    static constexpr int NODE_REPORT_TO_BROKER = 103;
    // The Street Vendor's meal: restores the whole party for a price.
    static constexpr int NODE_VENDOR_MEAL = 110;
    static constexpr int VENDOR_MEAL_COST = 20;

    // Tags identifying the fixed story fights.
    static constexpr const char* ENCOUNTER_THUGS = "thugs";
    static constexpr const char* ENCOUNTER_ENFORCER = "enforcer";

    // A fixed fight that should be standing on a map right now.
    struct EncounterSpec {
        std::string tag;
        std::string mapName;
        float x = 0.0f;
        float y = 0.0f;
        // Where the party falls back to after fleeing, so the fight does not
        // restart on the spot.
        float retreatX = 0.0f;
        float retreatY = 0.0f;
        // (enemy type name, level). The first entry is the one visible on the map.
        std::vector<std::pair<std::string, int>> enemies;
    };

    // Neither pointer is owned. party may be null (no rewards are paid).
    StoryDirector(QuestLog* questLog, Party* party);

    // --- Events from the game ---------------------------------------------
    void onMapEntered(const std::string& mapName);
    void onEncounterWon(const std::string& tag);
    // Call each time a conversation with npcName shows a new node.
    void onDialogueNode(const std::string& npcName, int nodeId);

    // --- Queries ----------------------------------------------------------
    // A fresh dialogue tree for this NPC, reflecting current story progress.
    // Never returns null.
    std::unique_ptr<DialogueTree> buildDialogue(const std::string& npcName) const;

    bool isMapUnlocked(const std::string& mapName) const;
    // Why a locked map is locked, for showing to the player.
    std::string getLockedMessage(const std::string& mapName) const;

    // The fixed fight that belongs on this map right now, if any.
    bool getPendingEncounter(const std::string& mapName, EncounterSpec& out) const;

    // The quest and objective to show in the HUD. False when nothing is active.
    bool getTrackedObjective(std::string& questName, std::string& objective) const;

    bool isStoryComplete() const;
    // True once, right after the final quest is completed.
    bool consumeEndingTrigger();

    // Messages for the player ("New quest: ..."), oldest first. Clears the queue.
    std::vector<std::string> takeNotifications();

private:
    bool questActive(int questId) const;
    bool questCompleted(int questId) const;
    bool objectiveDone(int questId, std::size_t index) const;

    void startQuest(int questId);
    void completeObjective(int questId, std::size_t index);
    void payRewards(int questId);
    int partyGold() const;

    QuestLog* m_questLog;
    Party* m_party;
    std::vector<std::string> m_notifications;
    bool m_endingTriggered = false;
};
