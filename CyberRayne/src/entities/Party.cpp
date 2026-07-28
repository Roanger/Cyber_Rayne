#include "../../include/Party.h"
#include <algorithm>

Party::Party() = default;
Party::~Party() = default;

void Party::addMember(std::unique_ptr<Player> member) {
    if (member) {
        m_members.push_back(std::move(member));
    }
}

Player* Party::getMember(std::size_t index) const {
    if (index < m_members.size()) {
        return m_members[index].get();
    }
    return nullptr;
}

bool Party::isDefeated() const {
    return countAlive() == 0;
}

int Party::countAlive() const {
    int count = 0;
    for (const auto& member : m_members) {
        if (member && member->isAlive()) {
            ++count;
        }
    }
    return count;
}

Player* Party::getFirstAliveMember() const {
    for (const auto& member : m_members) {
        if (member && member->isAlive()) {
            return member.get();
        }
    }
    return nullptr;
}

void Party::resetAtb() {
    for (auto& member : m_members) {
        if (member) {
            member->resetAtb();
        }
    }
}

std::unique_ptr<Party> Party::createDefaultParty() {
    auto party = std::make_unique<Party>();

    auto rayne = std::make_unique<Player>(Player::CharacterClass::SOLO, "Rayne");
    rayne->setHealth(rayne->getMaxHealth());
    rayne->setMana(rayne->getMaxMana());
    party->addMember(std::move(rayne));

    auto cyph = std::make_unique<Player>(Player::CharacterClass::NETRUNNER, "Cyph");
    cyph->setHealth(cyph->getMaxHealth());
    cyph->setMana(cyph->getMaxMana());
    party->addMember(std::move(cyph));

    auto vash = std::make_unique<Player>(Player::CharacterClass::INFILTRATOR, "Vash");
    vash->setHealth(vash->getMaxHealth());
    vash->setMana(vash->getMaxMana());
    party->addMember(std::move(vash));

    return party;
}
