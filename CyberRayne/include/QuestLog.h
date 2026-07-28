#pragma once

#include <vector>
#include <memory>

#include "Quest.h"

// A journal that tracks all quests the player has encountered.
class QuestLog {
public:
    QuestLog();
    ~QuestLog() = default;

    // Add a quest to the journal. The quest starts in NOT_STARTED state.
    void addQuest(std::unique_ptr<Quest> quest);

    // Start a quest (transition from NOT_STARTED to ACTIVE).
    bool startQuest(int questId);

    // Complete an objective on a quest. Returns false if quest/objective not found.
    bool completeObjective(int questId, std::size_t objectiveIndex);

    // Get a quest by ID.
    Quest* getQuest(int questId);
    const Quest* getQuest(int questId) const;

    // Get all quests.
    const std::vector<std::unique_ptr<Quest>>& getQuests() const { return m_quests; }

    // Query quest counts.
    std::size_t getActiveQuestCount() const;
    std::size_t getCompletedQuestCount() const;

    bool isEmpty() const { return m_quests.empty(); }

private:
    std::vector<std::unique_ptr<Quest>> m_quests;
};
