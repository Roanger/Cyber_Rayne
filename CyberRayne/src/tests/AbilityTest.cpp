// Covers the ability system: per-class kits, target resolution, and the
// cost-validation rule that an unaffordable ability must not consume the turn.

#include "../../include/Ability.h"
#include "../../include/AbilityRegistry.h"
#include "../../include/BattleSystem.h"
#include "../../include/Enemy.h"
#include "../../include/Party.h"
#include "../../include/Player.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Tick the battle until a party member is awaiting input.
bool advanceToPlayerTurn(BattleSystem& battle) {
    for (int i = 0; i < 20000; ++i) {
        if (battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION) {
            return true;
        }
        battle.update(0.016f);
    }
    return false;
}
}

int main() {
    // --- Every class has a usable level 1 kit -----------------------------
    for (int i = 0; i < Player::CHARACTER_CLASS_COUNT; ++i) {
        auto charClass = static_cast<Player::CharacterClass>(i);
        const auto& kit = AbilityRegistry::forClass(charClass);
        expect(!kit.empty(), "every class should have abilities defined");

        auto atLevelOne = AbilityRegistry::unlockedFor(charClass, 1);
        expect(!atLevelOne.empty(), "every class should have an ability at level 1");

        // Unlock levels must be ascending so the level-up grant order is sane.
        int previous = 0;
        for (const Ability* ability : kit) {
            expect(ability->getUnlockLevel() >= previous, "kit should be ordered by unlock level");
            previous = ability->getUnlockLevel();
            expect(ability->getManaCost() >= 0, "mana cost should not be negative");
        }
    }

    // --- Players learn abilities on creation and on level up ---------------
    {
        Player netrunner(Player::CharacterClass::NETRUNNER, "Cyph");
        expect(!netrunner.getAbilities().empty(), "a new character should know its level 1 abilities");

        std::size_t before = netrunner.getAbilities().size();
        for (int i = 0; i < 5; ++i) {
            netrunner.levelUp();
        }
        expect(netrunner.getAbilities().size() > before, "leveling should unlock more abilities");

        // Granting is idempotent — loading a save must not duplicate the kit.
        std::size_t afterLevels = netrunner.getAbilities().size();
        netrunner.refreshAbilities();
        expect(netrunner.getAbilities().size() == afterLevels,
               "refreshing abilities should not duplicate them");
    }

    // --- Classes have distinct kits ---------------------------------------
    {
        Player solo(Player::CharacterClass::SOLO, "Rook");
        Player infil(Player::CharacterClass::INFILTRATOR, "Wisp");
        expect(solo.getAbilities().front() != infil.getAbilities().front(),
               "different classes should not share their opening ability");
    }

    // --- Only the Netrunner can heal an ally -------------------------------
    {
        bool netrunnerHeals = false;
        for (const Ability* a : AbilityRegistry::forClass(Player::CharacterClass::NETRUNNER)) {
            if (a->getKind() == Ability::Kind::HEAL && a->getTarget() == Ability::Target::SINGLE_ALLY) {
                netrunnerHeals = true;
            }
        }
        expect(netrunnerHeals, "the netrunner should have an ally-targeted heal");
    }

    // --- Cost validation must not consume the turn --------------------------
    {
        auto party = std::make_unique<Party>();
        auto caster = std::make_unique<Player>(Player::CharacterClass::NETRUNNER, "Broke");
        caster->setMana(0);  // Cannot afford anything
        party->addMember(std::move(caster));

        std::vector<std::unique_ptr<Enemy>> foes;
        foes.push_back(std::make_unique<Enemy>("Target Dummy", 1));

        BattleSystem battle(party.get());
        battle.startBattle(foes);
        expect(advanceToPlayerTurn(battle), "a player turn should come up");

        Combatant* actor = battle.getCurrentActor();
        expect(actor != nullptr, "there should be a current actor");

        bool accepted = battle.submitPlayerAction(BattleSystem::ActionRequest::ability(0, 0));
        expect(!accepted, "an unaffordable ability should be rejected");
        expect(!battle.getLastRejectionReason().empty(), "rejection should explain itself");

        // The turn survives: the same character is still waiting, and can act.
        expect(battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION,
               "a rejected ability should not consume the turn");
        expect(battle.getCurrentActor() == actor, "the same actor should still be waiting");
        expect(battle.submitPlayerAction(BattleSystem::BattleAction::ATTACK),
               "the actor should still be able to attack after a rejected ability");
    }

    // --- Out-of-range ability index is rejected -----------------------------
    {
        auto party = Party::createDefaultParty();
        std::vector<std::unique_ptr<Enemy>> foes;
        foes.push_back(std::make_unique<Enemy>("Target Dummy", 1));

        BattleSystem battle(party.get());
        battle.startBattle(foes);
        expect(advanceToPlayerTurn(battle), "a player turn should come up");

        expect(!battle.submitPlayerAction(BattleSystem::ActionRequest::ability(999, 0)),
               "an out-of-range ability index should be rejected");
        expect(battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION,
               "a rejected index should not consume the turn");
    }

    // --- An affordable ability spends mana and lands ------------------------
    {
        auto party = std::make_unique<Party>();
        auto caster = std::make_unique<Player>(Player::CharacterClass::NETRUNNER, "Charged");
        party->addMember(std::move(caster));

        std::vector<std::unique_ptr<Enemy>> foes;
        foes.push_back(std::make_unique<Enemy>("Target Dummy", 1));

        BattleSystem battle(party.get());
        battle.startBattle(foes);
        expect(advanceToPlayerTurn(battle), "a player turn should come up");

        Player* member = party->getMember(0);
        const int manaBefore = member->getMana();
        const int enemyHpBefore = foes[0]->getHealth();

        expect(battle.submitPlayerAction(BattleSystem::ActionRequest::ability(0, 0)),
               "an affordable ability should be accepted");
        expect(member->getMana() < manaBefore, "using an ability should spend mana");
        expect(foes[0]->getHealth() < enemyHpBefore, "a damage ability should hurt its target");
    }

    // --- Manual targeting picks the requested enemy -------------------------
    {
        auto party = Party::createDefaultParty();
        std::vector<std::unique_ptr<Enemy>> foes;
        foes.push_back(std::make_unique<Enemy>("First", 1));
        foes.push_back(std::make_unique<Enemy>("Second", 1));

        BattleSystem battle(party.get());
        battle.startBattle(foes);
        expect(advanceToPlayerTurn(battle), "a player turn should come up");

        const int secondHpBefore = foes[1]->getHealth();
        expect(battle.submitPlayerAction(BattleSystem::ActionRequest::attack(1)),
               "attacking the second enemy should be accepted");
        expect(foes[1]->getHealth() < secondHpBefore,
               "the chosen target should take the damage, not the first enemy");
    }

    // --- Area abilities hit every enemy -------------------------------------
    {
        const Ability* suppressing = AbilityRegistry::find("solo_suppressing_fire");
        expect(suppressing != nullptr, "suppressing fire should exist");
        expect(suppressing->getTarget() == Ability::Target::ALL_ENEMIES,
               "suppressing fire should target all enemies");
        expect(!suppressing->needsTargetChoice(),
               "an area ability should not ask for a target");

        auto party = std::make_unique<Party>();
        party->addMember(std::make_unique<Player>(Player::CharacterClass::SOLO, "Rook"));

        std::vector<std::unique_ptr<Enemy>> foes;
        foes.push_back(std::make_unique<Enemy>("First", 1));
        foes.push_back(std::make_unique<Enemy>("Second", 1));
        const int firstBefore = foes[0]->getHealth();
        const int secondBefore = foes[1]->getHealth();

        BattleSystem battle(party.get());
        battle.startBattle(foes);
        expect(advanceToPlayerTurn(battle), "a player turn should come up");
        expect(battle.submitPlayerAction(BattleSystem::ActionRequest::ability(0, -1)),
               "suppressing fire should be accepted");

        expect(foes[0]->getHealth() < firstBefore, "area damage should hit the first enemy");
        expect(foes[1]->getHealth() < secondBefore, "area damage should hit the second enemy");
    }

    // --- Status-applying abilities attach their effect ----------------------
    {
        const Ability* neurotoxin = AbilityRegistry::find("inf_neurotoxin");
        expect(neurotoxin != nullptr, "neurotoxin should exist");
        expect(neurotoxin->appliesStatus(), "neurotoxin should apply a status");
        expect(neurotoxin->getStatusType() == StatusEffect::Type::POISON,
               "neurotoxin should apply poison");

        Player caster(Player::CharacterClass::INFILTRATOR, "Wisp");
        Enemy victim("Mark", 1);
        neurotoxin->execute(&caster, {&victim});
        expect(victim.hasStatusEffect(StatusEffect::Type::POISON),
               "the target should be poisoned");
    }

    // --- Enemies have kits and use the same execution path -------------------
    {
        const std::vector<const Ability*> droneKit = AbilityRegistry::forEnemy("Security Drone");
        expect(!droneKit.empty(), "the security drone should have an ability");

        // An unknown enemy name yields an empty kit rather than crashing.
        expect(AbilityRegistry::forEnemy("Nonexistent Foe").empty(),
               "an unknown enemy should have no abilities");
    }

    // --- Ids are unique and resolvable ---------------------------------------
    {
        expect(AbilityRegistry::find("net_patch") != nullptr, "patch should resolve by id");
        expect(AbilityRegistry::find("no_such_ability") == nullptr,
               "an unknown id should resolve to nullptr");
    }

    std::cout << "AbilityTest passed" << std::endl;
    return 0;
}
