#pragma once

#include <string>
#include <vector>

// A single objective within a quest.
class QuestObjective {
public:
    enum class State {
        NOT_STARTED,
        IN_PROGRESS,
        COMPLETED,
        FAILED
    };

    QuestObjective(const std::string& description);

    const std::string& getDescription() const { return m_description; }
    State getState() const { return m_state; }

    void setState(State state) { m_state = state; }
    bool isCompleted() const { return m_state == State::COMPLETED; }
    bool isFailed() const { return m_state == State::FAILED; }

private:
    std::string m_description;
    State m_state;
};

// A quest with a series of objectives, a reward, and an overall status.
class Quest {
public:
    enum class State {
        NOT_STARTED,
        ACTIVE,
        COMPLETED,
        FAILED
    };

    Quest(int id, const std::string& name, const std::string& description);

    int getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    const std::string& getDescription() const { return m_description; }
    State getState() const { return m_state; }

    void addObjective(const QuestObjective& objective);
    const std::vector<QuestObjective>& getObjectives() const { return m_objectives; }
    std::vector<QuestObjective>& getObjectivesMutable() { return m_objectives; }

    void setState(State state) { m_state = state; }

    // Mark an objective at the given index as completed.
    // Automatically completes the quest if all objectives are done.
    void completeObjective(std::size_t index);

    // Check if all objectives are completed.
    bool allObjectivesCompleted() const;

    // Rewards
    int getExperienceReward() const { return m_expReward; }
    int getGoldReward() const { return m_goldReward; }
    void setExperienceReward(int exp) { m_expReward = exp; }
    void setGoldReward(int gold) { m_goldReward = gold; }

private:
    int m_id;
    std::string m_name;
    std::string m_description;
    State m_state;
    std::vector<QuestObjective> m_objectives;
    int m_expReward;
    int m_goldReward;
};
