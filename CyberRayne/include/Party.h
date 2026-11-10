#pragma once

#include <memory>
#include <vector>

#include "Inventory.h"
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

    // The shared bag. Items belong to the party, not to individual members.
    Inventory& getBag() { return m_bag; }
    const Inventory& getBag() const { return m_bag; }

    // Use one consumable from the bag on a member. On failure nothing is
    // consumed. message, if given, receives a line for the player either way.
    bool useItem(const std::string& itemId, std::size_t memberIndex, std::string* message = nullptr);

    // Move a weapon or armor from the bag onto a member. Whatever they were
    // wearing in that slot goes back in the bag.
    bool equipFromBag(std::size_t memberIndex, const std::string& itemId);
    // Take off a member's weapon or armor and return it to the bag. slot must
    // be WEAPON or ARMOR.
    bool unequipToBag(std::size_t memberIndex, Item::ItemType slot);

    // Reset all members' ATB gauges (e.g., on battle start).
    void resetAtb();

    // Convenience factory for the vertical slice: Rayne + two companions.
    static std::unique_ptr<Party> createDefaultParty();

private:
    std::vector<std::unique_ptr<Player>> m_members;
    Inventory m_bag;
};
