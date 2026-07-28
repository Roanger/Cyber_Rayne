#include "../../include/Quest.h"
#include "../../include/QuestLog.h"
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
    QuestLog questLog;
    expect(questLog.isEmpty(), "quest log should start empty");

    // Create a quest with two objectives.
    auto quest = std::make_unique<Quest>(1, "Neon Streets", "Explore the neon district and find the data chip.");
    quest->addObjective(QuestObjective("Travel to the neon district"));
    quest->addObjective(QuestObjective("Retrieve the data chip"));
    quest->setExperienceReward(150);
    quest->setGoldReward(50);
    questLog.addQuest(std::move(quest));

    expect(questLog.getQuests().size() == 1, "quest log should have 1 quest");
    expect(questLog.getQuest(1) != nullptr, "should find quest by ID");
    expect(questLog.getQuest(1)->getState() == Quest::State::NOT_STARTED, "quest should start as NOT_STARTED");
    expect(questLog.getQuest(1)->getObjectives().size() == 2, "quest should have 2 objectives");

    // Start the quest.
    expect(questLog.startQuest(1), "startQuest should succeed");
    expect(questLog.getQuest(1)->getState() == Quest::State::ACTIVE, "quest should be ACTIVE");
    expect(!questLog.startQuest(1), "starting an already-active quest should fail");
    expect(questLog.getActiveQuestCount() == 1, "should have 1 active quest");

    // Complete the first objective.
    expect(questLog.completeObjective(1, 0), "completing objective 0 should succeed");
    expect(questLog.getQuest(1)->getObjectives()[0].isCompleted(), "objective 0 should be completed");
    expect(questLog.getQuest(1)->getState() == Quest::State::ACTIVE, "quest should still be active with 1 objective left");

    // Complete the second objective — should auto-complete the quest.
    expect(questLog.completeObjective(1, 1), "completing objective 1 should succeed");
    expect(questLog.getQuest(1)->getState() == Quest::State::COMPLETED, "quest should be COMPLETED when all objectives done");
    expect(questLog.getCompletedQuestCount() == 1, "should have 1 completed quest");
    expect(questLog.getActiveQuestCount() == 0, "should have 0 active quests after completion");

    // Verify rewards.
    expect(questLog.getQuest(1)->getExperienceReward() == 150, "exp reward should be 150");
    expect(questLog.getQuest(1)->getGoldReward() == 50, "gold reward should be 50");

    // Test invalid operations.
    expect(!questLog.completeObjective(999, 0), "completing objective on non-existent quest should fail");
    expect(!questLog.completeObjective(1, 99), "completing invalid objective index should fail");

    // Test a quest with no objectives — allObjectivesCompleted should return false.
    auto emptyQuest = std::make_unique<Quest>(2, "Empty", "A quest with no objectives");
    emptyQuest->setState(Quest::State::ACTIVE);
    expect(!emptyQuest->allObjectivesCompleted(), "quest with no objectives should not be 'all completed'");
    questLog.addQuest(std::move(emptyQuest));

    std::cout << "QuestTest passed" << std::endl;
    return 0;
}
