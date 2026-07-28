#include "../../include/QuestLog.h"

QuestLog::QuestLog() = default;

void QuestLog::addQuest(std::unique_ptr<Quest> quest) {
    if (quest) {
        m_quests.push_back(std::move(quest));
    }
}

bool QuestLog::startQuest(int questId) {
    Quest* quest = getQuest(questId);
    if (!quest || quest->getState() != Quest::State::NOT_STARTED) {
        return false;
    }
    quest->setState(Quest::State::ACTIVE);
    return true;
}

bool QuestLog::completeObjective(int questId, std::size_t objectiveIndex) {
    Quest* quest = getQuest(questId);
    if (!quest || quest->getState() != Quest::State::ACTIVE) {
        return false;
    }
    if (objectiveIndex >= quest->getObjectives().size()) {
        return false;
    }
    quest->completeObjective(objectiveIndex);
    return true;
}

Quest* QuestLog::getQuest(int questId) {
    for (auto& quest : m_quests) {
        if (quest && quest->getId() == questId) {
            return quest.get();
        }
    }
    return nullptr;
}

const Quest* QuestLog::getQuest(int questId) const {
    for (const auto& quest : m_quests) {
        if (quest && quest->getId() == questId) {
            return quest.get();
        }
    }
    return nullptr;
}

std::size_t QuestLog::getActiveQuestCount() const {
    std::size_t count = 0;
    for (const auto& quest : m_quests) {
        if (quest && quest->getState() == Quest::State::ACTIVE) {
            ++count;
        }
    }
    return count;
}

std::size_t QuestLog::getCompletedQuestCount() const {
    std::size_t count = 0;
    for (const auto& quest : m_quests) {
        if (quest && quest->getState() == Quest::State::COMPLETED) {
            ++count;
        }
    }
    return count;
}
