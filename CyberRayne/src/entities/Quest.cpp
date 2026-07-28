#include "../../include/Quest.h"

QuestObjective::QuestObjective(const std::string& description)
    : m_description(description), m_state(State::NOT_STARTED) {}

Quest::Quest(int id, const std::string& name, const std::string& description)
    : m_id(id), m_name(name), m_description(description),
      m_state(State::NOT_STARTED), m_expReward(0), m_goldReward(0) {}

void Quest::addObjective(const QuestObjective& objective) {
    m_objectives.push_back(objective);
}

void Quest::completeObjective(std::size_t index) {
    if (index >= m_objectives.size()) {
        return;
    }
    m_objectives[index].setState(QuestObjective::State::COMPLETED);

    if (allObjectivesCompleted()) {
        m_state = State::COMPLETED;
    }
}

bool Quest::allObjectivesCompleted() const {
    if (m_objectives.empty()) {
        return false;
    }
    for (const auto& obj : m_objectives) {
        if (!obj.isCompleted()) {
            return false;
        }
    }
    return true;
}
