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

bool Party::useItem(const std::string& itemId, std::size_t memberIndex, std::string* message) {
    auto say = [message](const std::string& text) {
        if (message) *message = text;
    };

    Player* target = getMember(memberIndex);
    const Inventory::Stack* stack = m_bag.find(itemId);
    if (!target || !stack || !stack->prototype) {
        say("That can't be used.");
        return false;
    }
    if (stack->prototype->getType() != Item::ItemType::CONSUMABLE) {
        say(stack->prototype->getName() + " can't be used like that.");
        return false;
    }
    if (!stack->prototype->canUseOn(target)) {
        say(stack->prototype->getName() + " would have no effect on " + target->getName() + ".");
        return false;
    }

    // Copy what the message needs first: removing the last one frees the stack.
    const std::string itemName = stack->prototype->getName();
    stack->prototype->use(target);
    m_bag.remove(itemId, 1);
    say("Used " + itemName + " on " + target->getName() + ".");
    return true;
}

bool Party::equipFromBag(std::size_t memberIndex, const std::string& itemId) {
    Player* member = getMember(memberIndex);
    const Inventory::Stack* stack = m_bag.find(itemId);
    if (!member || !stack || !stack->prototype) {
        return false;
    }

    const Item::ItemType slot = stack->prototype->getType();
    if (slot != Item::ItemType::WEAPON && slot != Item::ItemType::ARMOR) {
        return false;
    }

    auto item = ItemCatalog::create(itemId);
    if (!item || !m_bag.remove(itemId, 1)) {
        return false;
    }

    unequipToBag(memberIndex, slot);
    if (slot == Item::ItemType::WEAPON) {
        member->equipWeapon(std::move(item));
    } else {
        member->equipArmor(std::move(item));
    }
    return true;
}

bool Party::unequipToBag(std::size_t memberIndex, Item::ItemType slot) {
    Player* member = getMember(memberIndex);
    if (!member) {
        return false;
    }

    std::unique_ptr<Item> removed;
    if (slot == Item::ItemType::WEAPON) {
        removed = member->unequipWeapon();
    } else if (slot == Item::ItemType::ARMOR) {
        removed = member->unequipArmor();
    }
    if (!removed) {
        return false;
    }
    // Gear that did not come from the catalog has no ID to stack under; it is
    // dropped rather than corrupting the bag.
    m_bag.add(removed->getId(), 1);
    return true;
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
