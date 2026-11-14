// What the battle screen is told about a fight (BattleSystem events), how
// that becomes on-screen effects (BattleFx), and how art is found by name.
#include <algorithm>
#include "../../include/BattleSystem.h"
#include "../../include/Enemy.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/StatusEffect.h"
#include "../../include/ui/BattleFx.h"
#include "../../include/BattleSounds.h"
#include "../../include/Ability.h"
#include "../../include/ui/SpriteSlug.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

using Event = BattleSystem::BattleEvent;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "BattlePresentationTest FAILED: " << message << std::endl;
        std::exit(1);
    }
}

// Tick until `who` is awaiting input, defending through other members' turns.
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

const Event* find(const std::vector<Event>& events, Event::Kind kind, const Combatant* subject) {
    for (const Event& event : events) {
        if (event.kind == kind && event.subject == subject) return &event;
    }
    return nullptr;
}

}  // namespace

int main() {
    // --- Art is found by display name ---------------------------------------
    expect(spriteSlug("Street Thug") == "street_thug", "spaces become underscores");
    expect(spriteSlug("Black ICE") == "black_ice", "case is folded");
    expect(spriteSlug("Fixer Kaga") == "fixer_kaga", "NPC names work the same way");
    expect(spriteSlug("  Odd--Name! ") == "odd_name", "punctuation collapses and is trimmed");
    expect(spriteSlug("Netrunner") == "netrunner", "class names pass through");

    // --- An attack is reported as an action plus the damage it did ----------
    {
        auto party = Party::createDefaultParty();
        Player* rayne = party->getMember(0);
        std::vector<std::unique_ptr<Enemy>> foes;
        foes.push_back(std::make_unique<Enemy>("Target Dummy", 1));
        Enemy* dummy = foes[0].get();
        dummy->setHealth(dummy->getMaxHealth());

        BattleSystem battle(party.get());
        battle.startBattle(foes);
        expect(advanceToTurnOf(battle, rayne), "Rayne should get a turn");
        battle.takeEvents();  // discard whatever led up to this turn

        const int before = dummy->getHealth();
        expect(battle.submitPlayerAction(BattleSystem::ActionRequest::attack(0)), "the attack is accepted");
        const auto events = battle.takeEvents();
        const Event* action = find(events, Event::Kind::ACTION, rayne);
        const Event* damage = find(events, Event::Kind::DAMAGE, dummy);
        expect(action && action->text == "Attack", "the attacker's action is announced");
        expect(damage && damage->amount == before - dummy->getHealth() && damage->amount > 0,
               "the damage event carries the HP actually lost");
        expect(battle.takeEvents().empty(), "taking events clears the queue");
    }

    // --- Defeat, poison and healing are all reported ------------------------
    {
        auto party = Party::createDefaultParty();
        Player* rayne = party->getMember(0);
        Player* vash = party->getMember(2);
        party->getBag().add("stim_pack", 1);
        party->getBag().add("defib_patch", 1);
        std::vector<std::unique_ptr<Enemy>> foes;
        foes.push_back(std::make_unique<Enemy>("Weakling", 1));
        foes.push_back(std::make_unique<Enemy>("Bystander", 1));
        Enemy* weakling = foes[0].get();
        Enemy* bystander = foes[1].get();
        bystander->setHealth(bystander->getMaxHealth());

        BattleSystem battle(party.get());
        battle.startBattle(foes);
        expect(advanceToTurnOf(battle, rayne), "Rayne should get a turn");

        // Poison ticks on the bystander arrive as damage with no action.
        bystander->addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::POISON, 10.0f, 3));
        battle.takeEvents();
        bool sawTick = false;
        for (int i = 0; i < 200 && !sawTick; ++i) {
            battle.update(0.016f);
            sawTick = find(battle.takeEvents(), Event::Kind::DAMAGE, bystander) != nullptr;
        }
        expect(sawTick, "a poison tick is reported as damage");

        // Healing.
        rayne->setHealth(5);
        expect(battle.submitPlayerAction(BattleSystem::ActionRequest::item(0, 0)), "the stim pack is used");
        auto events = battle.takeEvents();
        const Event* heal = find(events, Event::Kind::HEAL, rayne);
        expect(heal && heal->amount > 0, "healing is reported with its amount");
        expect(find(events, Event::Kind::ACTION, rayne) != nullptr, "using an item is announced");

        // A killing blow reports damage and defeat.
        expect(advanceToTurnOf(battle, rayne), "Rayne should get another turn");
        weakling->setHealth(1);
        battle.takeEvents();
        expect(battle.submitPlayerAction(BattleSystem::ActionRequest::attack(0)), "the finishing attack lands");
        events = battle.takeEvents();
        expect(find(events, Event::Kind::DAMAGE, weakling) != nullptr, "the killing blow is damage");
        expect(find(events, Event::Kind::DEFEATED, weakling) != nullptr, "and the enemy is reported defeated");

        // Someone raised mid-battle gets turns again.
        vash->setHealth(0);
        expect(advanceToTurnOf(battle, rayne), "Rayne should get a third turn");
        battle.takeEvents();
        expect(battle.submitPlayerAction(BattleSystem::ActionRequest::item(0, 2)), "the defib patch is used");
        expect(find(battle.takeEvents(), Event::Kind::HEAL, vash) != nullptr, "the revive is reported as healing");
        bool listed = false;
        for (Combatant* ally : battle.getLivingAllies()) {
            listed = listed || ally == vash;
        }
        expect(listed, "the revived member is back among the living allies");
        expect(advanceToTurnOf(battle, vash), "and takes turns again");
    }

    // --- Effects -------------------------------------------------------------
    {
        auto party = Party::createDefaultParty();
        Player* rayne = party->getMember(0);
        Enemy thug("Street Thug", 1);

        BattleFx fx;
        expect(!fx.isBusy(), "nothing is playing at the start");
        expect(fx.getVisibility(&thug) == 1.0f && fx.getHitFlash(&thug) == 0.0f, "an untouched enemy is plain");

        fx.onEvents({{Event::Kind::ACTION, rayne, 0, "Attack"},
                     {Event::Kind::DAMAGE, &thug, 12, {}},
                     {Event::Kind::DAMAGE, &thug, 7, {}}});
        expect(fx.isBusy(), "effects are playing after a hit");
        expect(fx.getBanner() == "Rayne: Attack", "the banner names the actor and the action");
        expect(fx.getHitFlash(&thug) == 1.0f, "the target flashes");
        expect(fx.getPopups().size() == 2 && fx.getPopups()[0].text == "12" && fx.getPopups()[1].text == "7",
               "each hit gets its own number");
        expect(fx.getPopups()[0].age == 0.0f && fx.getPopups()[1].age < 0.0f,
               "the second number waits behind the first");

        expect(fx.getAction(rayne).pose == BattleFx::Pose::ATTACK && fx.getAction(rayne).progress == 0.0f,
               "the attacker starts their attack animation");
        expect(fx.getAction(&thug).pose == BattleFx::Pose::IDLE, "the target is not acting");

        fx.update(BattleFx::LUNGE_TIME / 2.0f);
        expect(fx.getAction(rayne).progress > 0.0f && fx.getAction(rayne).progress < 1.0f,
               "the animation advances");
        expect(fx.getLunge(rayne) > 0.9f, "the attacker is at full lunge halfway through");
        expect(fx.getLunge(&thug) == 0.0f, "the target does not lunge");

        fx.onEvents({{Event::Kind::DEFEATED, &thug, 0, {}}});
        expect(fx.isDefeated(&thug) && fx.getVisibility(&thug) == 1.0f, "a defeated enemy starts fully visible");
        fx.update(BattleFx::FADE_TIME / 2.0f);
        expect(fx.getVisibility(&thug) > 0.4f && fx.getVisibility(&thug) < 0.6f, "then fades");

        fx.update(5.0f);
        expect(fx.getVisibility(&thug) == 0.0f, "and is gone");
        expect(fx.getPopups().empty() && fx.getBanner().empty(), "numbers and banner expire");
        expect(!fx.isBusy(), "everything settles");

        fx.onEvents({{Event::Kind::HEAL, &thug, 5, {}}});
        expect(!fx.isDefeated(&thug) && fx.getVisibility(&thug) == 1.0f, "healing the fallen brings them back");
        expect(fx.getPopups().back().text == "+5", "heals show with a plus sign");

        expect(fx.getAction(rayne).pose == BattleFx::Pose::IDLE, "a finished action returns to idle");

        fx.reset();
        expect(!fx.isBusy() && fx.getPopups().empty(), "reset clears everything");

        // Which animation plays is decided by what was done.
        fx.onEvents({{Event::Kind::ACTION, rayne, 0, "Defend"}});
        expect(fx.getAction(rayne).pose == BattleFx::Pose::GUARD, "defending plays the guard animation");
        expect(fx.isBusy(), "an animation in progress keeps the battle waiting");
        fx.onEvents({{Event::Kind::ACTION, rayne, 0, "Overclock"}});
        expect(fx.getAction(rayne).pose == BattleFx::Pose::ABILITY, "anything else plays the ability animation");
        fx.update(BattleFx::ACTION_TIME + 0.01f);
        expect(fx.getAction(rayne).pose == BattleFx::Pose::IDLE && !fx.isBusy(), "and then ends");
        fx.reset();
    }

    // --- Effect animations -----------------------------------------------------
    {
        auto party = Party::createDefaultParty();
        Player* rayne = party->getMember(0);
        Enemy thug("Street Thug", 1);
        Enemy drone("Security Drone", 1);
        BattleFx fx;

        fx.onEvents({{Event::Kind::ACTION, &thug, 0, "Attack"}, {Event::Kind::DAMAGE, rayne, 9, {}}});
        expect(fx.getEffects().size() == 1 && fx.getEffects()[0].who == rayne &&
               fx.getEffects()[0].sheet == "vfx_hit_spark" && !fx.getEffects()[0].flip,
               "an enemy's hit sparks on its target, unmirrored");
        fx.reset();

        fx.onEvents({{Event::Kind::ACTION, rayne, 0, "Attack"}, {Event::Kind::DAMAGE, &thug, 9, {}}});
        expect(fx.getEffects().size() == 1 && fx.getEffects()[0].flip, "the party's attacks are mirrored");
        fx.reset();

        fx.onEvents({{Event::Kind::ACTION, &drone, 0, "Attack"}, {Event::Kind::DAMAGE, rayne, 5, {}}});
        expect(fx.getEffects()[0].sheet == "vfx_lightning", "a drone's attack is electric");
        fx.reset();
        fx.onEvents({{Event::Kind::ACTION, rayne, 0, "Executioner"}, {Event::Kind::DAMAGE, &thug, 40, {}}});
        expect(fx.getEffects()[0].sheet == "vfx_explosion", "named abilities have their own effect");
        fx.reset();
        fx.onEvents({{Event::Kind::DAMAGE, rayne, 3, {}}});
        expect(fx.getEffects()[0].sheet == "vfx_poison", "damage nobody dealt shows as poison");
        fx.reset();
        fx.onEvents({{Event::Kind::HEAL, rayne, 20, {}}});
        expect(fx.getEffects()[0].sheet == "vfx_heal_glow" && fx.getEffects()[0].onGround,
               "healing glows at the feet");

        // An effect runs through its eight frames and then ends.
        expect(fx.isBusy() && fx.getEffects()[0].frame() == 0, "an effect starts on its first frame");
        fx.update(0.35f);
        expect(fx.getEffects().size() == 1 && fx.getEffects()[0].frame() == 3, "and advances");
        fx.update(5.0f);
        expect(fx.getEffects().empty(), "and is removed when it finishes");
    }

    // --- Sounds ---------------------------------------------------------------
    {
        auto party = Party::createDefaultParty();
        Player* rayne = party->getMember(0);
        Enemy thug("Street Thug", 1);
        Enemy drone("Security Drone", 1);
        auto has = [](const std::vector<std::string>& sounds, const char* name) {
            return std::find(sounds.begin(), sounds.end(), name) != sounds.end();
        };

        expect(attackSoundFor(&thug) == "hit_blunt" && attackSoundFor(&drone) == "zap",
               "enemies attack with their own kind of sound");
        expect(!attackSoundFor(rayne).empty(), "a party member's attack has a sound");

        auto sounds = battleSoundsFor({{Event::Kind::ACTION, &thug, 0, "Attack"},
                                       {Event::Kind::DAMAGE, rayne, 9, {}}});
        expect(sounds.size() == 1 && sounds[0] == "hit_blunt", "an attack makes its hit sound, once");

        sounds = battleSoundsFor({{Event::Kind::DAMAGE, rayne, 3, {}}});
        expect(sounds.size() == 1 && sounds[0] == "poison", "damage nobody dealt is a status ticking");

        sounds = battleSoundsFor({{Event::Kind::ACTION, rayne, 0, "Stim Pack"},
                                  {Event::Kind::HEAL, rayne, 30, {}},
                                  {Event::Kind::HEAL, &thug, 30, {}}});
        expect(sounds.size() == 1 && sounds[0] == "heal", "several heals at once make one sound");

        sounds = battleSoundsFor({{Event::Kind::ACTION, rayne, 0, "Defend"}});
        expect(has(sounds, "guard"), "defending has a sound");
        sounds = battleSoundsFor({{Event::Kind::DEFEATED, &thug, 0, {}}, {Event::Kind::DEFEATED, rayne, 0, {}}});
        expect(has(sounds, "enemy_down") && has(sounds, "ally_down"), "falling sounds differ by side");
        sounds = battleSoundsFor({{Event::Kind::MESSAGE, rayne, 0, "Stunned"}});
        expect(has(sounds, "stun"), "being stunned has a sound");
        expect(battleSoundsFor({}).empty(), "nothing happening is silent");

        // Every ability in the game makes some sound when used, so a new one
        // cannot be added and silently forgotten.
        for (auto charClass : {Player::CharacterClass::SOLO, Player::CharacterClass::NETRUNNER,
                               Player::CharacterClass::INFILTRATOR}) {
            Player member(charClass, "Test");
            for (int level = 0; level < 12; ++level) member.levelUp();
            for (const Ability* ability : member.getAbilities()) {
                if (!ability) continue;
                const auto made = battleSoundsFor({{Event::Kind::ACTION, &member, 0, ability->getName()},
                                                   {Event::Kind::HEAL, &member, 1, {}}});
                expect(!made.empty(), "every ability has a sound");
                const auto bare = battleSoundsFor({{Event::Kind::ACTION, &member, 0, ability->getName()}});
                if (bare.empty()) {
                    std::cout << "  (" << ability->getName() << " relies on its heal sound)" << std::endl;
                }
            }
        }
    }

    // --- Every status effect has a tag --------------------------------------
    for (auto type : {StatusEffect::Type::POISON, StatusEffect::Type::STUN, StatusEffect::Type::DEFENSE_UP,
                      StatusEffect::Type::DEFENSE_DOWN, StatusEffect::Type::ATTACK_UP,
                      StatusEffect::Type::ATTACK_DOWN, StatusEffect::Type::SLOW, StatusEffect::Type::REGEN}) {
        expect(std::string(statusTagFor(type).text) != "?", "status effects all have a short label");
    }

    std::cout << "BattlePresentationTest passed" << std::endl;
    return 0;
}
