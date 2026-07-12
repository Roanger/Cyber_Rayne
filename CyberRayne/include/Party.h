#pragma once

#include <memory>
#include <vector>

#include "Player.h"

// A party of player-controlled characters.
// The protagonist (Rayne) is always the first member in this vertical slice.
class Party {
public:
    Party();
    ~Party();

    // Add a character to the party.
    void addMember(std::unique_ptr<Player> member);

    // Accessors
    std::size_t size() const { return m_members.size(); }
    bool isEmpty() const { return m_members.empty(); }
    Player* getLeader() const { return isEmpty() ? nullptr : m_members[0].get(); }
    Player* getMember(std::size_t index) const;
    const std::vector<std::unique_ptr<Player>>& getMembers() const { return m_members; }

    // Party-wide helpers
    bool isDefeated() const;
    int countAlive() const;
    Player* getFirstAliveMember() const;

    // Reset all members' ATB gauges (e.g., on battle start).
    void resetAtb();

    // Convenience factory for the vertical slice: Rayne + two companions.
    static std::unique_ptr<Party> createDefaultParty();

private:
    std::vector<std::unique_ptr<Player>> m_members;
};
