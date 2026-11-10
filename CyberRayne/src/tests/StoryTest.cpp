// Plays the whole main quest chain through StoryDirector and World without a
// renderer: dialogue effects, travel objectives, fixed fights, the plaza lock,
// rewards, and the ending trigger.
#include "../../include/ContentFactory.h"
#include "../../include/DialogueNode.h"
#include "../../include/DialogueSystem.h"
#include "../../include/DialogueTree.h"
#include "../../include/Enemy.h"
#include "../../include/Map.h"
#include "../../include/NPC.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/Quest.h"
#include "../../include/QuestLog.h"
#include "../../include/StoryDirector.h"
#include "../../include/World.h"
#include "../../include/ui/TextWrap.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "StoryTest FAILED: " << message << std::endl;
        std::exit(1);
    }
}

// Talk to an NPC the way GameState does: report each node as it appears,
// taking the given choice whenever one is offered.
void talk(StoryDirector& director, const std::string& npc, int choiceIndex = 0) {
    DialogueSystem dialogue;
    expect(dialogue.startDialogue(director.buildDialogue(npc)), npc + " should have something to say");

    int reported = -1;
    int guard = 0;
    while (dialogue.isActive() && ++guard < 100) {
        const DialogueNode* node = dialogue.getCurrentNode();
        if (node && node->getId() != reported) {
            reported = node->getId();
            director.onDialogueNode(npc, reported);
        }
        if (dialogue.getState() == DialogueSystem::State::SHOWING_CHOICES) {
            for (int i = 0; i < choiceIndex; ++i) {
                dialogue.handleInput(InputAction::MOVE_DOWN);
            }
        }
        dialogue.handleInput(InputAction::CONFIRM);
    }
    expect(dialogue.isFinished(), "conversation with " + npc + " should end");
}

Quest::State stateOf(const QuestLog& log, int id) {
    return log.getQuest(id)->getState();
}

// Walk the leader one tile and let the step finish, checking transitions each
// frame as the game loop does.
void step(World& world, Player& player, float dx, float dy) {
    player.move(dx, dy, world.getCurrentMap());
    for (int i = 0; i < 20; ++i) {
        player.update(1.0f / 60.0f);
        world.checkMapTransition();
    }
}

}  // namespace

int main() {
    // --- Text wrapping ------------------------------------------------------
    {
        auto lines = wrapText("the quick brown fox", 10);
        expect(lines.size() == 2, "wrap should produce two lines");
        expect(lines[0] == "the quick" && lines[1] == "brown fox", "wrap should break at spaces");
        expect(wrapText("abcdefghij", 4).size() == 3, "an overlong word should be hard-split");
        expect(wrapText("", 10).empty(), "empty text wraps to nothing");
        for (const auto& line : wrapText("Hey choom, I lost a data chip to some street thugs.", 12)) {
            expect(line.size() <= 12, "no wrapped line may exceed the budget");
        }
    }

    // --- Map transitions ----------------------------------------------------
    // Regression: the transition check used to fire mid-step, and the
    // interrupted step then carried the player through Neon Alley's far exit,
    // landing them in Corporate Plaza straight from Central.
    {
        World world;
        expect(world.initialize(), "world should initialize");
        Player rayne(Player::CharacterClass::SOLO, "Rayne");
        rayne.initialize();
        world.setPlayer(&rayne);
        rayne.teleport(15.0f, 2.0f);

        step(world, rayne, 0.0f, -1.0f);
        expect(world.getCurrentMap()->getName() == "Neon Alley", "north of Central is Neon Alley");
        expect(rayne.getX() == 15.0f && rayne.getY() == 23.0f, "player arrives at the alley's south end");
        expect(!rayne.isMoving(), "arriving cancels the step in progress");
        expect(world.getCurrentBiome() == World::BiomeType::NEON_ALLEY, "biome follows the map");

        // A locked destination turns the player back and reports it.
        world.setMapLocked("Corporate Plaza", true);
        rayne.teleport(15.0f, 2.0f);
        step(world, rayne, 0.0f, -1.0f);
        expect(world.getCurrentMap()->getName() == "Neon Alley", "a locked plaza cannot be entered");
        expect(world.consumeBlockedMap() == "Corporate Plaza", "the blocked map is reported");
        expect(world.consumeBlockedMap().empty(), "the blocked map is reported once");
        expect(rayne.getY() == 2.0f, "the player is turned back off the exit");

        world.setMapLocked("Corporate Plaza", false);
        step(world, rayne, 0.0f, -1.0f);
        expect(world.getCurrentMap()->getName() == "Corporate Plaza", "an unlocked plaza can be entered");
    }

    // --- Story encounters in the world --------------------------------------
    {
        World world;
        world.initialize();
        Player rayne(Player::CharacterClass::SOLO, "Rayne");
        rayne.initialize();
        world.setPlayer(&rayne);
        rayne.teleport(15.0f, 16.0f);

        expect(!world.ensureStoryEncounter("bad", 15.0f, 14.0f, {{"NoSuchEnemy", 1}}),
               "an unknown enemy type is refused");
        expect(!world.hasStoryEncounter(), "a refused encounter leaves nothing behind");

        const std::size_t before = world.getCurrentMap()->getEnemies().size();
        expect(world.ensureStoryEncounter("test", 15.0f, 14.0f,
                                          {{"StreetThug", 2}, {"StreetThug", 2}, {"CombatCyborg", 2}}),
               "a valid story encounter spawns");
        expect(world.getCurrentMap()->getEnemies().size() <= before + 1,
               "only the marker stands on the map");
        world.ensureStoryEncounter("test", 15.0f, 14.0f, {{"StreetThug", 2}});
        expect(world.getCurrentMap()->getEnemies().size() <= before + 1,
               "ensuring twice does not duplicate the fight");

        rayne.teleport(15.0f, 14.0f);
        expect(world.checkEnemyEncounter(), "walking into the marker starts the fight");
        expect(world.getActiveEncounterTag() == "test", "the fight carries its tag");
        expect(world.getActiveEncounter().size() == 3, "the escorts join the fight");
        expect(!world.hasStoryEncounter(), "the marker leaves the map");
        world.endEncounter();
        expect(world.getActiveEncounterTag().empty(), "the tag clears when the fight ends");
    }

    // --- NPC lookup ----------------------------------------------------------
    {
        World world;
        world.initialize();
        Player rayne(Player::CharacterClass::SOLO, "Rayne");
        rayne.initialize();
        world.setPlayer(&rayne);

        rayne.teleport(14.0f, 16.0f);  // directly below Fixer Kaga at (14, 15)
        NPC* near = world.findNpcNearPlayer(1.5f);
        expect(near && near->getName() == "Fixer Kaga", "the adjacent NPC is found");
        expect(world.isNpcAt(14, 15), "Kaga's tile is occupied");
        rayne.teleport(1.0f, 22.0f);
        expect(world.findNpcNearPlayer(1.5f) == nullptr, "no NPC is in range out in the corner");
    }

    // --- Loading into a map --------------------------------------------------
    {
        World world;
        world.initialize();
        Player rayne(Player::CharacterClass::SOLO, "Rayne");
        rayne.initialize();
        world.setPlayer(&rayne);

        expect(world.travelTo("Neon Alley", 15.4f, 8.0f), "a known map can be travelled to");
        expect(world.getCurrentMap()->getName() == "Neon Alley", "travel changes the map");
        expect(world.getCurrentBiome() == World::BiomeType::NEON_ALLEY, "travel changes the biome");
        expect(rayne.getX() == 15.0f && rayne.getY() == 8.0f, "a mid-step position snaps to a tile");

        expect(world.travelTo("Corporate Plaza", 8.0f, 3.0f), "travel to the plaza works");
        expect(rayne.getX() == 12.0f && rayne.getY() == 18.0f, "a wall tile falls back to the arrival point");
        expect(world.travelTo("Neo-Kyoto Central", 14.0f, 15.0f), "travel to Central works");
        expect(rayne.getX() == 15.0f && rayne.getY() == 16.0f, "an NPC's tile falls back to the arrival point");
        expect(world.travelTo("Neo-Kyoto Central", -50.0f, 900.0f), "an off-map position is tolerated");
        expect(rayne.getX() == 15.0f && rayne.getY() == 16.0f, "an off-map position falls back too");

        expect(!world.travelTo("Atlantis", 1.0f, 1.0f), "an unknown map is refused");
        expect(world.getCurrentMap()->getName() == "Neo-Kyoto Central", "a refused trip changes nothing");
    }

    // --- The Street Vendor's meal -------------------------------------------
    {
        QuestLog vendorLog;
        ContentFactory::createQuestChain(&vendorLog);
        auto crew = Party::createDefaultParty();
        StoryDirector vendorStory(&vendorLog, crew.get());
        Player* lead = crew->getLeader();
        Player* second = crew->getMember(1);

        lead->setHealth(5);
        second->setHealth(0);
        second->setMana(0);

        talk(vendorStory, "Street Vendor", 0);
        expect(lead->getHealth() == 5, "no gold, no meal");
        expect(vendorStory.takeNotifications().empty(), "a refused meal announces nothing");

        // Gold split across members still counts as the party's purse.
        lead->addGold(15);
        second->addGold(30);
        talk(vendorStory, "Street Vendor", 1);
        expect(lead->getHealth() == 5 && lead->getGold() == 15, "declining costs nothing");

        talk(vendorStory, "Street Vendor", 0);
        expect(lead->getHealth() == lead->getMaxHealth(), "the meal restores health");
        expect(second->getHealth() == second->getMaxHealth() && second->getMana() == second->getMaxMana(),
               "the meal revives and restores everyone");
        expect(lead->getGold() + second->getGold() == 45 - StoryDirector::VENDOR_MEAL_COST,
               "the meal costs exactly its price");
        expect(lead->getGold() >= 0 && second->getGold() >= 0, "nobody goes into debt");
    }

    // --- The whole quest chain ----------------------------------------------
    QuestLog log;
    ContentFactory::createQuestChain(&log);
    auto party = Party::createDefaultParty();
    StoryDirector story(&log, party.get());
    StoryDirector::EncounterSpec spec;
    std::string questName;
    std::string objective;

    const int goldAtStart = party->getLeader()->getGold();

    expect(!story.getTrackedObjective(questName, objective), "nothing is tracked before the story starts");
    expect(!story.isMapUnlocked("Corporate Plaza"), "the plaza starts locked");
    expect(story.isMapUnlocked("Neon Alley"), "the alley is always open");
    expect(!story.getLockedMessage("Corporate Plaza").empty(), "a locked map explains itself");

    // Declining leaves the quest untouched and can be retried.
    talk(story, "Fixer Kaga", 1);
    expect(stateOf(log, 100) == Quest::State::NOT_STARTED, "declining does not start the quest");
    story.onMapEntered("Neon Alley");
    expect(!story.getPendingEncounter("Neon Alley", spec), "no thugs before the job is accepted");

    // Events out of order must not advance anything.
    story.onEncounterWon(StoryDirector::ENCOUNTER_THUGS);
    story.onDialogueNode("Fixer Kaga", StoryDirector::NODE_RETURN_CHIP);
    story.onDialogueNode("Exec Tanaka", StoryDirector::NODE_CONFRONT_TANAKA);
    story.onDialogueNode("Info Broker", StoryDirector::NODE_REPORT_TO_BROKER);
    expect(log.getActiveQuestCount() == 0 && log.getCompletedQuestCount() == 0,
           "out-of-order events change nothing");
    story.takeNotifications();

    // Quest 1: The Data Chip
    talk(story, "Fixer Kaga", 0);
    expect(stateOf(log, 100) == Quest::State::ACTIVE, "accepting starts The Data Chip");
    auto notes = story.takeNotifications();
    expect(notes.size() == 1 && notes[0] == "New quest: The Data Chip", "starting a quest is announced");
    expect(story.takeNotifications().empty(), "notifications are delivered once");
    expect(story.getTrackedObjective(questName, objective) && questName == "The Data Chip" &&
           objective == "Travel to Neon Alley", "the first objective is tracked");

    expect(!story.getPendingEncounter("Neon Alley", spec), "thugs wait until the player arrives");
    story.onMapEntered("Neon Alley");
    expect(story.getPendingEncounter("Neon Alley", spec), "thugs appear once the alley is entered");
    expect(spec.tag == StoryDirector::ENCOUNTER_THUGS && spec.enemies.size() == 3, "the thug fight is specified");
    expect(!story.getPendingEncounter("Neo-Kyoto Central", spec), "thugs are only in the alley");

    talk(story, "Fixer Kaga");
    expect(stateOf(log, 100) == Quest::State::ACTIVE, "Kaga cannot be paid off before the chip is recovered");

    story.onEncounterWon(StoryDirector::ENCOUNTER_THUGS);
    expect(!story.getPendingEncounter("Neon Alley", spec), "beaten thugs stay beaten");
    expect(!story.isMapUnlocked("Corporate Plaza"), "the plaza stays locked until the chip is returned");

    story.takeNotifications();
    talk(story, "Fixer Kaga");
    expect(stateOf(log, 100) == Quest::State::COMPLETED, "returning the chip completes The Data Chip");
    expect(stateOf(log, 101) == Quest::State::ACTIVE, "Corporate Intrusion follows on");
    expect(party->getLeader()->getGold() == goldAtStart + 100, "the quest pays its gold once");
    notes = story.takeNotifications();
    expect(notes.size() == 4 && notes[0].rfind("Quest complete: The Data Chip", 0) == 0 &&
           notes[1] == "Received: Mono-Katana" && notes[2] == "Received: Stim Pack  x2" &&
           notes[3] == "New quest: Corporate Intrusion",
           "completion, item rewards, and the next quest are announced in order");
    expect(party->getBag().count("mono_katana") == 1 && party->getBag().count("stim_pack") == 2,
           "quest item rewards land in the bag");
    expect(story.isMapUnlocked("Corporate Plaza"), "the chip opens the plaza");

    talk(story, "Fixer Kaga");
    expect(party->getLeader()->getGold() == goldAtStart + 100, "talking again does not pay again");
    expect(party->getBag().count("mono_katana") == 1, "talking again does not hand out items again");

    // Quest 2: Corporate Intrusion
    talk(story, "Exec Tanaka");
    expect(stateOf(log, 101) == Quest::State::ACTIVE, "Tanaka cannot be confronted past his Enforcer");
    story.onMapEntered("Corporate Plaza");
    expect(story.getPendingEncounter("Corporate Plaza", spec) &&
           spec.tag == StoryDirector::ENCOUNTER_ENFORCER, "the Enforcer guards the plaza");
    expect(spec.enemies.front().first == "CorporateEnforcer", "the Enforcer is the visible enemy");
    story.onEncounterWon(StoryDirector::ENCOUNTER_ENFORCER);
    talk(story, "Exec Tanaka", 1);
    expect(stateOf(log, 101) == Quest::State::COMPLETED, "either reply to Tanaka completes the quest");
    expect(stateOf(log, 102) == Quest::State::ACTIVE, "The Truth follows on");
    expect(party->getLeader()->getGold() == goldAtStart + 350, "the second quest pays out");
    expect(party->getBag().count("kevlar_weave") == 1 && party->getBag().count("trauma_kit") == 1,
           "the second quest's items land in the bag");

    // Quest 3: The Truth
    talk(story, "Info Broker");
    expect(stateOf(log, 102) == Quest::State::ACTIVE, "the report needs the trip back to Central first");
    expect(!story.consumeEndingTrigger(), "no ending yet");
    story.onMapEntered("Neo-Kyoto Central");
    talk(story, "Info Broker");
    expect(stateOf(log, 102) == Quest::State::COMPLETED, "reporting completes The Truth");
    expect(story.isStoryComplete(), "the story is complete");
    expect(story.consumeEndingTrigger(), "the ending fires");
    expect(!story.consumeEndingTrigger(), "the ending fires once");
    expect(party->getLeader()->getGold() == goldAtStart + 650, "the final quest pays out");
    expect(!story.getTrackedObjective(questName, objective), "nothing is tracked after the ending");
    talk(story, "Info Broker");
    expect(!story.consumeEndingTrigger(), "talking again does not replay the ending");

    // Every NPC in the world has something to say at every point.
    {
        World world;
        world.initialize();
        for (const auto& map : world.getMaps()) {
            for (const auto& npc : map->getNPCs()) {
                auto tree = story.buildDialogue(npc->getName());
                expect(tree && tree->getNode(tree->getRootId()) != nullptr,
                       npc->getName() + " should have a root line");
            }
        }
        // And the fixed fights stand on walkable tiles of real maps.
        for (const char* tag : {"thugs", "enforcer"}) {
            QuestLog probeLog;
            ContentFactory::createQuestChain(&probeLog);
            StoryDirector probe(&probeLog, nullptr);
            StoryDirector::EncounterSpec s;
            const bool isThugs = std::string(tag) == "thugs";
            if (isThugs) {
                probeLog.startQuest(100);
                probe.onMapEntered("Neon Alley");
            } else {
                probeLog.getQuest(100)->setState(Quest::State::COMPLETED);
                probeLog.startQuest(101);
                probe.onMapEntered("Corporate Plaza");
            }
            const std::string mapName = isThugs ? "Neon Alley" : "Corporate Plaza";
            expect(probe.getPendingEncounter(mapName, s), std::string(tag) + " should be pending");
            const Map* where = nullptr;
            for (const auto& map : world.getMaps()) {
                if (map->getName() == s.mapName) where = map.get();
            }
            expect(where != nullptr, "story fights name a real map");
            expect(where->isTileWalkable(static_cast<int>(s.x), static_cast<int>(s.y)),
                   std::string(tag) + " should stand on a walkable tile");
            expect(where->isTileWalkable(static_cast<int>(s.retreatX), static_cast<int>(s.retreatY)),
                   std::string(tag) + " retreat point should be walkable");
        }
    }

    std::cout << "StoryTest passed" << std::endl;
    return 0;
}
