// The item catalog, the shared party bag, using items in and out of battle,
// and equipment moving between the bag and party members.
#include "../../include/BattleSystem.h"
#include "../../include/ContentFactory.h"
#include "../../include/Enemy.h"
#include "../../include/Equipment.h"
#include "../../include/Inventory.h"
#include "../../include/Party.h"
#include "../../include/Player.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "InventoryTest FAILED: " << message << std::endl;
        std::exit(1);
    }
}

// Tick until the given member is the one awaiting input, passing other
// members' turns by defending. Returns false if it never comes up.
bool advanceToTurnOf(BattleSystem& battle, const Combatant* who) {
    for (int i = 0; i < 40000; ++i) {
        if (battle.getResult() != BattleSystem::BattleResult::ONGOING) return false;
        if (battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION) {
            if (battle.getCurrentActor() == who) return true;
            battle.submitPlayerAction(BattleSystem::ActionRequest::defend());
        }
        battle.update(0.016f);
    }
    return false;
}

}  // namespace

int main() {
    // --- Catalog -------------------------------------------------------------
    {
        const char* ids[] = {"stim_pack", "trauma_kit", "defib_patch", "shock_baton",
                             "mono_katana", "street_jacket", "kevlar_weave"};
        for (const char* id : ids) {
            auto item = ItemCatalog::create(id);
            expect(item != nullptr, std::string(id) + " should be in the catalog");
            expect(item->getId() == id, std::string(id) + " should know its own ID");
            expect(!item->getName().empty() && !item->getDescription().empty(),
                   std::string(id) + " should have a name and description");
        }
        expect(ItemCatalog::create("no_such_item") == nullptr, "unknown IDs produce nothing");
        expect(ItemCatalog::create("") == nullptr, "the empty ID produces nothing");
        expect(ItemCatalog::exists("stim_pack") && !ItemCatalog::exists("nope"), "exists matches create");
        expect(ItemCatalog::create("mono_katana")->getAttackBonus() >
                   ItemCatalog::create("shock_baton")->getAttackBonus(),
               "the reward weapon should beat the starting one");
        expect(ItemCatalog::create("kevlar_weave")->getDefenseBonus() >
                   ItemCatalog::create("street_jacket")->getDefenseBonus(),
               "the reward armor should beat the starting one");
    }

    // --- Bag -----------------------------------------------------------------
    {
        Inventory bag;
        expect(bag.isEmpty(), "a new bag is empty");
        expect(bag.add("stim_pack", 2) && bag.add("stim_pack"), "adding stacks");
        expect(bag.count("stim_pack") == 3 && bag.getStacks().size() == 1, "same item shares a stack");
        expect(!bag.add("no_such_item") && bag.getStacks().size() == 1, "unknown items are refused");
        expect(!bag.add("stim_pack", 0) && !bag.add("stim_pack", -4) && bag.count("stim_pack") == 3,
               "non-positive counts are refused");
        expect(!bag.remove("stim_pack", 4) && bag.count("stim_pack") == 3,
               "removing more than is held removes nothing");
        expect(!bag.remove("stim_pack", 0) && !bag.remove("trauma_kit"), "bad removals are refused");
        bag.add("shock_baton");
        bag.add("defib_patch");
        expect(bag.getStacksOfType(Item::ItemType::CONSUMABLE).size() == 2 &&
                   bag.getStacksOfType(Item::ItemType::WEAPON).size() == 1 &&
                   bag.getStacksOfType(Item::ItemType::ARMOR).empty(),
               "stacks filter by type");
        expect(bag.getStacks()[0].id == "stim_pack" && bag.getStacks()[2].id == "defib_patch",
               "stacks keep pickup order");
        expect(bag.remove("stim_pack", 3) && bag.count("stim_pack") == 0 && bag.find("stim_pack") == nullptr,
               "an emptied stack disappears");
        expect(bag.getStacks().size() == 2, "other stacks are untouched");
    }

    // --- Using items outside battle -----------------------------------------
    {
        auto party = Party::createDefaultParty();
        Player* rayne = party->getMember(0);
        Player* cyph = party->getMember(1);
        party->getBag().add("stim_pack", 2);
        party->getBag().add("defib_patch", 1);
        party->getBag().add("shock_baton", 1);
        std::string message;

        expect(!party->useItem("stim_pack", 0, &message) && party->getBag().count("stim_pack") == 2,
               "healing someone at full health wastes nothing");
        expect(!message.empty(), "a refused use explains itself");

        rayne->setHealth(10);
        expect(party->useItem("stim_pack", 0, &message), "a wounded member can be healed");
        expect(rayne->getHealth() == 50 && party->getBag().count("stim_pack") == 1, "one pack heals 40 and is consumed");

        cyph->setHealth(0);
        expect(!party->useItem("stim_pack", 1) && cyph->getHealth() == 0 && party->getBag().count("stim_pack") == 1,
               "a stim pack cannot raise the fallen");
        expect(!party->useItem("defib_patch", 0) && party->getBag().count("defib_patch") == 1,
               "a defib patch does nothing for the living");
        expect(party->useItem("defib_patch", 1), "a defib patch raises the fallen");
        expect(cyph->isAlive() && cyph->getHealth() == cyph->getMaxHealth() * 25 / 100,
               "they return at a quarter health");
        expect(party->getBag().find("defib_patch") == nullptr, "the last patch leaves the bag");

        expect(!party->useItem("shock_baton", 0), "gear cannot be used as a consumable");
        expect(!party->useItem("trauma_kit", 0), "items not in the bag cannot be used");
        expect(!party->useItem("stim_pack", 99), "a missing member cannot be targeted");
        expect(party->getBag().count("stim_pack") == 1 && party->getBag().count("shock_baton") == 1,
               "refused uses consume nothing");
    }

    // --- Equipment -----------------------------------------------------------
    {
        auto party = Party::createDefaultParty();
        Player* rayne = party->getMember(0);
        Inventory& bag = party->getBag();
        bag.add("shock_baton", 1);
        bag.add("mono_katana", 1);
        bag.add("street_jacket", 1);
        bag.add("stim_pack", 1);
        const int baseAttack = rayne->getEffectiveStrength();
        const int baseDefense = rayne->getEffectiveDefense();

        expect(party->equipFromBag(0, "shock_baton"), "a weapon in the bag can be equipped");
        expect(rayne->getEffectiveStrength() == baseAttack + 4 && bag.count("shock_baton") == 0,
               "equipping applies the bonus and takes the item out of the bag");

        expect(party->equipFromBag(0, "mono_katana"), "equipping over an occupied slot swaps");
        expect(rayne->getEffectiveStrength() == baseAttack + 8, "the new weapon's bonus replaces the old");
        expect(bag.count("shock_baton") == 1 && bag.count("mono_katana") == 0,
               "the old weapon returns to the bag");

        expect(party->equipFromBag(0, "street_jacket") && rayne->getEffectiveDefense() == baseDefense + 3,
               "armor goes in its own slot");
        expect(rayne->getEffectiveStrength() == baseAttack + 8, "equipping armor leaves the weapon alone");

        expect(!party->equipFromBag(0, "stim_pack") && bag.count("stim_pack") == 1, "consumables cannot be equipped");
        expect(!party->equipFromBag(0, "kevlar_weave"), "gear not in the bag cannot be equipped");
        expect(!party->equipFromBag(42, "shock_baton") && bag.count("shock_baton") == 1,
               "a missing member cannot equip, and nothing is lost");

        expect(party->unequipToBag(0, Item::ItemType::WEAPON), "a weapon can be taken off");
        expect(rayne->getEffectiveStrength() == baseAttack && bag.count("mono_katana") == 1,
               "taking it off drops the bonus and returns it to the bag");
        expect(!party->unequipToBag(0, Item::ItemType::WEAPON), "an empty slot has nothing to take off");
        expect(!party->unequipToBag(0, Item::ItemType::CONSUMABLE), "only weapon and armor are slots");

        // Nothing is created or destroyed by all that shuffling.
        party->unequipToBag(0, Item::ItemType::ARMOR);
        expect(bag.count("shock_baton") == 1 && bag.count("mono_katana") == 1 &&
                   bag.count("street_jacket") == 1 && bag.count("stim_pack") == 1 && bag.getStacks().size() == 4,
               "every item is accounted for");
    }

    // --- Items in battle -----------------------------------------------------
    {
        auto party = Party::createDefaultParty();
        Player* rayne = party->getMember(0);
        Player* vash = party->getMember(2);
        party->getBag().add("shock_baton", 1);   // not a battle item; must not shift indices
        party->getBag().add("stim_pack", 2);
        party->getBag().add("defib_patch", 1);

        std::vector<std::unique_ptr<Enemy>> foes;
        foes.push_back(std::make_unique<Enemy>("Target Dummy", 1));
        BattleSystem battle(party.get());
        battle.startBattle(foes);

        expect(advanceToTurnOf(battle, rayne), "Rayne should get a turn");
        const int healthyHp = rayne->getHealth();
        expect(rayne->getHealth() == rayne->getMaxHealth() || healthyHp > 0, "Rayne is standing");

        // Index 0 is the stim pack (first consumable), index 1 the defib patch.
        rayne->setHealth(rayne->getMaxHealth());
        expect(!battle.submitPlayerAction(BattleSystem::ActionRequest::item(0, 0)),
               "a stim pack on a healthy ally is rejected");
        expect(battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION && battle.getCurrentActor() == rayne,
               "a rejected item does not cost the turn");
        expect(!battle.getLastRejectionReason().empty(), "the rejection explains itself");
        expect(party->getBag().count("stim_pack") == 2, "a rejected item is not consumed");
        expect(!battle.submitPlayerAction(BattleSystem::ActionRequest::item(7, 0)), "a missing item is rejected");
        expect(!battle.submitPlayerAction(BattleSystem::ActionRequest::item(0, 9)), "a missing target is rejected");

        vash->setHealth(0);
        expect(!battle.submitPlayerAction(BattleSystem::ActionRequest::item(0, 2)),
               "a stim pack on a fallen ally is rejected");
        expect(battle.submitPlayerAction(BattleSystem::ActionRequest::item(1, 2)),
               "a defib patch on a fallen ally is accepted");
        expect(vash->isAlive(), "the fallen ally is back up");
        expect(party->getBag().count("defib_patch") == 0, "the patch is consumed");

        expect(advanceToTurnOf(battle, rayne), "Rayne should get another turn");
        rayne->setHealth(1);
        // The patch is gone, so the stim pack is still consumable index 0.
        expect(battle.submitPlayerAction(BattleSystem::ActionRequest::item(0, -1)),
               "target -1 uses the item on the actor");
        expect(rayne->getHealth() == 41, "the stim pack healed the actor for 40");
        expect(party->getBag().count("stim_pack") == 1, "one stim pack is consumed");
        expect(party->getBag().count("shock_baton") == 1, "gear in the bag is untouched by battle items");
    }

    // --- Starting kit --------------------------------------------------------
    {
        auto party = Party::createDefaultParty();
        ContentFactory::grantStartingKit(party.get());
        expect(party->getBag().count("stim_pack") == 3 && party->getBag().count("defib_patch") == 1,
               "a new game starts with healing supplies");
        expect(!party->getBag().getStacksOfType(Item::ItemType::WEAPON).empty() &&
                   !party->getBag().getStacksOfType(Item::ItemType::ARMOR).empty(),
               "a new game starts with something to equip");
        ContentFactory::grantStartingKit(nullptr);  // must not crash
    }

    std::cout << "InventoryTest passed" << std::endl;
    return 0;
}
