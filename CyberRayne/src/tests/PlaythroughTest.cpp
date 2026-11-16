// Plays the whole vertical slice the way a person would, without a renderer:
// a scripted player reads the quest tracker, walks there tile by tile, talks
// to the people it names, fights every battle that starts on the way, heals,
// equips rewards, saves, and carries on to the ending. Run once for each of
// Rayne's classes.
//
// StoryTest checks the story rules by feeding events straight to the
// StoryDirector. This test checks that the game can actually be played to the
// end: every place is reachable on foot, the tracker alone is enough to know
// where to go, the fights along the way can be survived with the levels and
// items a real run has, and a save taken mid-run resumes it.
//
// GameState owns these systems in the real game but is tied to the renderer,
// so Session below repeats its glue (GameState::update for exploring,
// finishBattle, syncStory, saveGame, loadGame). If that glue changes in
// GameState, change it here too.
#include "../../include/Ability.h"
#include "../../include/BattleSystem.h"
#include "../../include/ContentFactory.h"
#include "../../include/DialogueNode.h"
#include "../../include/DialogueSystem.h"
#include "../../include/DialogueTree.h"
#include "../../include/Enemy.h"
#include "../../include/Inventory.h"
#include "../../include/Map.h"
#include "../../include/NPC.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/QuestLog.h"
#include "../../include/SaveSystem.h"
#include "../../include/StoryDirector.h"
#include "../../include/World.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "PlaythroughTest FAILED: " << message << std::endl;
        std::exit(1);
    }
}

// Same reach as GameState::TALK_RANGE.
constexpr float TALK_RANGE = 1.5f;
constexpr float FRAME = 1.0f / 60.0f;
// Rayne's walking speed in the game, for the play-time estimate.
constexpr float TILES_PER_SECOND = 6.0f;

struct Tile {
    int x = 0;
    int y = 0;
};

struct Stats {
    int steps = 0;
    int battles = 0;
    int storyBattles = 0;
    int wipes = 0;
    int meals = 0;
    int itemsUsed = 0;
    int saves = 0;
    int loads = 0;
    float battleSeconds = 0.0f;
    int linesRead = 0;
};

class Session {
public:
    Session(Player::CharacterClass rayneClass, std::string savePath)
        : m_class(rayneClass), m_savePath(std::move(savePath)) {}

    // GameState::startNewGame
    void newGame() {
        m_party = std::make_unique<Party>();
        m_party->addMember(std::make_unique<Player>(m_class, "Rayne"));
        m_party->addMember(std::make_unique<Player>(Player::CharacterClass::NETRUNNER, "Cyph"));
        m_party->addMember(std::make_unique<Player>(Player::CharacterClass::INFILTRATOR, "Vash"));
        ContentFactory::grantStartingKit(m_party.get());

        m_world = std::make_unique<World>();
        expect(m_world->initialize(), "the world should initialize");
        m_world->setPlayer(m_party->getLeader());
        m_party->getLeader()->setPosition(15.0f, 16.0f);

        m_log = std::make_unique<QuestLog>();
        ContentFactory::createQuestChain(m_log.get());
        beginStory();
    }

    // GameState::saveGame
    void save() {
        SaveWorldState worldState;
        worldState.mapName = mapName();
        SaveSystem saver;
        expect(saver.saveGame(m_savePath, m_party.get(), m_log.get(), &worldState), "saving should work");
        m_hasSave = true;
        ++stats.saves;
    }

    // GameState::loadGame
    void load() {
        expect(m_hasSave, "there should be a save to go back to");
        auto party = std::make_unique<Party>();
        auto log = std::make_unique<QuestLog>();
        SaveWorldState worldState;
        SaveSystem saver;
        expect(saver.loadGame(m_savePath, party.get(), log.get(), &worldState) &&
               !party->isEmpty() && !log->isEmpty(), "the save should load");
        expect(worldState.hasBag && !worldState.mapName.empty(), "the save should hold the bag and the map");

        m_story.reset();
        m_party = std::move(party);
        m_log = std::move(log);
        m_world = std::make_unique<World>();
        expect(m_world->initialize(), "the world should initialize on load");
        Player* leader = m_party->getLeader();
        m_world->setPlayer(leader);
        expect(m_world->travelTo(worldState.mapName, leader->getX(), leader->getY()),
               "the saved map should exist");
        beginStory();
        ++stats.loads;
    }

    std::string mapName() const {
        return m_world->getCurrentMap() ? m_world->getCurrentMap()->getName() : std::string();
    }
    Tile leaderTile() const {
        const Player* leader = m_party->getLeader();
        return {static_cast<int>(leader->getX()), static_cast<int>(leader->getY())};
    }
    Party& party() { return *m_party; }
    World& world() { return *m_world; }
    StoryDirector& story() { return *m_story; }
    QuestLog& log() { return *m_log; }
    bool endingSeen() const { return m_endingSeen; }
    const std::string& lastBlockedMessage() const { return m_lastBlockedMessage; }
    void clearBlockedMessage() { m_lastBlockedMessage.clear(); }

    // One tile in a direction: GameState::tryStep, then frames of
    // GameState::update until the step and anything it set off are over.
    // Returns false if the party was wiped and the save was reloaded.
    bool step(int dx, int dy) {
        Player* leader = m_party->getLeader();
        const Tile from = leaderTile();
        if (m_world->isNpcAt(from.x + dx, from.y + dy)) {
            leader->face(dx, dy);
            return true;
        }
        leader->move(static_cast<float>(dx), static_cast<float>(dy), m_world->getCurrentMap());
        ++stats.steps;

        for (int frame = 0; frame < 120; ++frame) {
            for (const auto& member : m_party->getMembers()) {
                member->update(FRAME);
            }
            m_world->update(FRAME);
            m_world->checkMapTransition();

            const std::string blocked = m_world->consumeBlockedMap();
            if (!blocked.empty()) {
                m_lastBlockedMessage = m_story->getLockedMessage(blocked);
            }
            if (mapName() != m_lastMap) {
                m_lastMap = mapName();
                m_story->onMapEntered(m_lastMap);
                syncStory();
            }

            bool encountered = m_world->checkEnemyEncounter();
            if (!encountered && m_party->getLeader()->consumeStepCompleted()) {
                encountered = m_world->onPlayerStep();
            }
            if (encountered) {
                // A visible enemy is met partway onto its tile. After the
                // fight the game carries on with the same step, so do that
                // here too rather than leave the leader between two tiles.
                if (!fight()) {
                    return false;
                }
            }
            if (!m_party->getLeader()->isMoving()) {
                break;
            }
        }
        expect(!m_party->getLeader()->isMoving(), "a step should finish within two seconds");
        return true;
    }

    // Talk to whoever is in reach, taking the given reply when asked.
    void talk(const std::string& npcName, int choiceIndex = 0) {
        NPC* npc = m_world->findNpcNearPlayer(TALK_RANGE);
        expect(npc && npc->getName() == npcName, "should be standing next to " + npcName);

        DialogueSystem dialogue;
        expect(dialogue.startDialogue(m_story->buildDialogue(npcName)), npcName + " should have something to say");
        int reported = -1;
        int guard = 0;
        while (dialogue.isActive() && ++guard < 100) {
            const DialogueNode* node = dialogue.getCurrentNode();
            if (node && node->getId() != reported) {
                reported = node->getId();
                ++stats.linesRead;
                m_story->onDialogueNode(npcName, reported);
            }
            if (dialogue.getState() == DialogueSystem::State::SHOWING_CHOICES) {
                for (int i = 0; i < choiceIndex; ++i) {
                    dialogue.handleInput(InputAction::MOVE_DOWN);
                }
            }
            dialogue.handleInput(InputAction::CONFIRM);
        }
        expect(dialogue.isFinished(), "the conversation with " + npcName + " should end");
        if (m_story->consumeEndingTrigger()) {
            m_endingSeen = true;
        }
        syncStory();
    }

    Stats stats;

private:
    void beginStory() {
        m_story = std::make_unique<StoryDirector>(m_log.get(), m_party.get());
        m_lastMap = mapName();
        syncStory();
    }

    // GameState::syncStory
    void syncStory() {
        m_world->setMapLocked("Corporate Plaza", !m_story->isMapUnlocked("Corporate Plaza"));
        StoryDirector::EncounterSpec spec;
        if (m_story->getPendingEncounter(mapName(), spec)) {
            expect(m_world->ensureStoryEncounter(spec.tag, spec.x, spec.y, spec.enemies),
                   "the story fight should be placeable");
        }
        m_story->takeNotifications();
    }

    // The battle loop of GameState::update, with the scripted player choosing
    // commands, then GameState::finishBattle. Returns false after a wipe.
    bool fight() {
        const std::string tag = m_world->getActiveEncounterTag();
        ++stats.battles;
        if (!tag.empty()) {
            ++stats.storyBattles;
        }

        BattleSystem battle(m_party.get());
        battle.startBattle(m_world->getActiveEncounter());
        int ticks = 0;
        for (; ticks < 400000 && battle.getResult() == BattleSystem::BattleResult::ONGOING; ++ticks) {
            if (battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION) {
                chooseCommand(battle);
            }
            battle.update(0.016f);
            battle.takeEvents();
        }
        const auto result = battle.getResult();
        expect(result != BattleSystem::BattleResult::ONGOING, "a battle should end");
        stats.battleSeconds += static_cast<float>(ticks) * 0.016f;

        m_world->endEncounter();
        if (!tag.empty() && result == BattleSystem::BattleResult::PLAYER_WIN) {
            m_story->onEncounterWon(tag);
        }
        syncStory();

        if (result == BattleSystem::BattleResult::PLAYER_LOSE) {
            // Game over: the card offers the last save.
            ++stats.wipes;
            load();
            return false;
        }
        // Nobody stays down after a win.
        for (const auto& member : m_party->getMembers()) {
            if (!member->isAlive()) {
                member->setHealth(1);
            }
        }
        return true;
    }

    // What a sensible player does on their turn: raise the fallen, patch up
    // anyone badly hurt, otherwise hit the weakest enemy with the strongest
    // thing they can afford.
    void chooseCommand(BattleSystem& battle) {
        Combatant* actor = battle.getCurrentActor();
        const auto consumables = m_party->getBag().getStacksOfType(Item::ItemType::CONSUMABLE);
        const auto& members = m_party->getMembers();

        int itemIndex = -1;
        int itemTarget = -1;
        for (std::size_t m = 0; m < members.size() && itemIndex < 0; ++m) {
            const Player* member = members[m].get();
            const bool down = !member->isAlive();
            const bool hurt = member->isAlive() && member->getHealth() * 100 < member->getMaxHealth() * 35;
            if (!down && !hurt) continue;
            for (std::size_t i = 0; i < consumables.size(); ++i) {
                if (consumables[i]->prototype->canUseOn(member)) {
                    itemIndex = static_cast<int>(i);
                    itemTarget = static_cast<int>(m);
                    break;
                }
            }
        }
        if (itemIndex >= 0 &&
            battle.submitPlayerAction(BattleSystem::ActionRequest::item(itemIndex, itemTarget))) {
            ++stats.itemsUsed;
            return;
        }

        int bestAbility = -1;
        int bestPower = actor->getEffectiveStrength();
        const auto& abilities = actor->getAbilities();
        for (std::size_t i = 0; i < abilities.size(); ++i) {
            const Ability* ability = abilities[i];
            if (ability->getKind() == Ability::Kind::DAMAGE && ability->canAfford(actor) &&
                ability->computeMagnitude(actor) > bestPower) {
                bestAbility = static_cast<int>(i);
                bestPower = ability->computeMagnitude(actor);
            }
        }

        int target = -1;
        int lowest = 1 << 30;
        int index = 0;
        for (Combatant* enemy : battle.getEnemies()) {
            if (!enemy || !enemy->isAlive()) continue;
            if (enemy->getHealth() < lowest) {
                lowest = enemy->getHealth();
                target = index;
            }
            ++index;
        }
        if (bestAbility < 0 ||
            !battle.submitPlayerAction(BattleSystem::ActionRequest::ability(bestAbility, target))) {
            battle.submitPlayerAction(BattleSystem::ActionRequest::attack(target));
        }
    }

    Player::CharacterClass m_class;
    std::string m_savePath;
    bool m_hasSave = false;
    std::unique_ptr<Party> m_party;
    std::unique_ptr<QuestLog> m_log;
    std::unique_ptr<World> m_world;
    std::unique_ptr<StoryDirector> m_story;
    std::string m_lastMap;
    std::string m_lastBlockedMessage;
    bool m_endingSeen = false;
};

// --- The scripted player ------------------------------------------------------

// Where each district's way out to its neighbour is, as a player learns them.
bool exitToward(const std::string& from, const std::string& to, std::vector<Tile>& tiles) {
    auto order = [](const std::string& name) {
        if (name == "Neo-Kyoto Central") return 0;
        if (name == "Neon Alley") return 1;
        if (name == "Corporate Plaza") return 2;
        return -1;
    };
    const int a = order(from);
    const int b = order(to);
    if (a < 0 || b < 0 || a == b) return false;
    tiles.clear();
    if (a == 0) {
        tiles = {{14, 1}, {15, 1}, {16, 1}};                       // north, to the alley
    } else if (a == 1 && b > a) {
        tiles = {{14, 1}, {15, 1}, {16, 1}};                       // north, to the plaza
    } else if (a == 1) {
        tiles = {{14, 24}, {15, 24}, {16, 24}};                    // south, to Central
    } else {
        tiles = {{11, 19}, {12, 19}, {13, 19}};                    // south, to the alley
    }
    return true;
}

// The first step of the shortest walk from the leader to any goal tile.
// Visible enemies are walked around when there is a way; NPCs are solid.
bool firstStepToward(Session& game, const std::vector<Tile>& goals, bool avoidEnemies, int& dx, int& dy) {
    const Map* map = game.world().getCurrentMap();
    const int w = map->getWidth();
    const int h = map->getHeight();
    auto at = [w](int x, int y) { return static_cast<std::size_t>(y * w + x); };

    std::vector<char> goal(static_cast<std::size_t>(w * h), 0);
    for (const Tile& tile : goals) {
        if (tile.x >= 0 && tile.y >= 0 && tile.x < w && tile.y < h) goal[at(tile.x, tile.y)] = 1;
    }
    std::vector<char> blocked(static_cast<std::size_t>(w * h), 0);
    if (avoidEnemies) {
        for (const auto& enemy : map->getEnemies()) {
            const int ex = static_cast<int>(enemy->getX());
            const int ey = static_cast<int>(enemy->getY());
            if (ex >= 0 && ey >= 0 && ex < w && ey < h && !goal[at(ex, ey)]) blocked[at(ex, ey)] = 1;
        }
    }

    const Tile start = game.leaderTile();
    std::vector<int> cameFrom(static_cast<std::size_t>(w * h), -1);
    std::queue<Tile> open;
    open.push(start);
    cameFrom[at(start.x, start.y)] = static_cast<int>(at(start.x, start.y));
    const int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    while (!open.empty()) {
        const Tile here = open.front();
        open.pop();
        if (goal[at(here.x, here.y)] && !(here.x == start.x && here.y == start.y)) {
            // Walk the trail back to the tile next to the start.
            Tile cursor = here;
            for (;;) {
                const int parent = cameFrom[at(cursor.x, cursor.y)];
                const Tile before{parent % w, parent / w};
                if (before.x == start.x && before.y == start.y) break;
                cursor = before;
            }
            dx = cursor.x - start.x;
            dy = cursor.y - start.y;
            return true;
        }
        for (const auto& dir : dirs) {
            const int nx = here.x + dir[0];
            const int ny = here.y + dir[1];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
            if (cameFrom[at(nx, ny)] >= 0 || blocked[at(nx, ny)]) continue;
            if (!map->isTileWalkable(nx, ny) || game.world().isNpcAt(nx, ny)) continue;
            cameFrom[at(nx, ny)] = static_cast<int>(at(here.x, here.y));
            open.push({nx, ny});
        }
    }
    return false;
}

enum class Walk { ARRIVED, MOVED, WIPED };

// Take one step toward a set of tiles on a named map, crossing districts as
// needed. ARRIVED means the leader stands on one of the tiles.
Walk walkToward(Session& game, const std::string& map, const std::vector<Tile>& tiles) {
    std::vector<Tile> goals = tiles;
    if (game.mapName() != map) {
        expect(exitToward(game.mapName(), map, goals), "there should be a way from " + game.mapName() + " to " + map);
    } else {
        const Tile here = game.leaderTile();
        for (const Tile& tile : goals) {
            if (tile.x == here.x && tile.y == here.y) return Walk::ARRIVED;
        }
    }
    int dx = 0;
    int dy = 0;
    if (!firstStepToward(game, goals, true, dx, dy)) {
        expect(firstStepToward(game, goals, false, dx, dy),
               "there should be a walkable route in " + game.mapName() + " toward " + map);
    }
    return game.step(dx, dy) ? Walk::MOVED : Walk::WIPED;
}

// The four tiles beside an NPC: close enough to talk.
std::vector<Tile> tilesBeside(Session& game, const std::string& map, const std::string& npcName) {
    for (const auto& candidate : game.world().getMaps()) {
        if (candidate->getName() != map) continue;
        for (const auto& npc : candidate->getNPCs()) {
            if (npc->getName() != npcName) continue;
            const int x = static_cast<int>(npc->getX());
            const int y = static_cast<int>(npc->getY());
            return {{x, y - 1}, {x, y + 1}, {x - 1, y}, {x + 1, y}};
        }
    }
    expect(false, npcName + " should live in " + map);
    return {};
}

// Between fights: put on anything better that is lying in the bag, and use
// healing items on anyone below half health, as from the pause menu.
void lookAfterParty(Session& game) {
    Party& party = game.party();
    for (Item::ItemType slot : {Item::ItemType::WEAPON, Item::ItemType::ARMOR}) {
        for (;;) {
            const auto gear = party.getBag().getStacksOfType(slot);
            bool equipped = false;
            for (const Inventory::Stack* stack : gear) {
                const std::string id = stack->id;
                for (std::size_t m = 0; m < party.getMembers().size() && !equipped; ++m) {
                    const Player* member = party.getMember(m);
                    const Item* worn = slot == Item::ItemType::WEAPON ? member->getEquippedWeapon()
                                                                     : member->getEquippedArmor();
                    if (!worn) {
                        equipped = party.equipFromBag(m, id);
                    }
                }
                if (equipped) break;
            }
            if (!equipped) break;
        }
    }

    for (std::size_t m = 0; m < party.getMembers().size(); ++m) {
        const Player* member = party.getMember(m);
        for (int guard = 0; guard < 8 && member->getHealth() * 2 < member->getMaxHealth(); ++guard) {
            std::string usable;
            for (const Inventory::Stack* stack : party.getBag().getStacksOfType(Item::ItemType::CONSUMABLE)) {
                if (stack->prototype->canUseOn(member)) {
                    usable = stack->id;
                    break;
                }
            }
            if (usable.empty() || !party.useItem(usable, m)) break;
            ++game.stats.itemsUsed;
        }
    }
}

int partyGold(Session& game) {
    int gold = 0;
    for (const auto& member : game.party().getMembers()) gold += member->getGold();
    return gold;
}

bool partyIsHurt(Session& game) {
    int health = 0;
    int most = 0;
    for (const auto& member : game.party().getMembers()) {
        health += member->getHealth();
        most += member->getMaxHealth();
    }
    return health * 100 < most * 55;
}

struct Outcome {
    Stats stats;
    int level = 0;
    int gold = 0;
    float minutes = 0.0f;
};

Outcome play(Player::CharacterClass rayneClass, const std::string& savePath) {
    Session game(rayneClass, savePath);
    game.newGame();
    game.save();

    std::string questName;
    std::string objective;
    expect(!game.story().getTrackedObjective(questName, objective), "nothing is tracked before talking to Kaga");

    bool triedLockedPlaza = false;
    bool reloadedOnPurpose = false;
    std::size_t completedAtLastSave = 0;

    // Each turn of the loop: look after the party, decide where the tracker
    // says to go, and take one step or one action toward it. Deciding again
    // every step is what lets a wipe and reload, or a fight on the way, drop
    // straight back into the plan.
    for (int turn = 0; turn < 20000 && !game.endingSeen(); ++turn) {
        lookAfterParty(game);

        // Save whenever a quest has been handed in, like a careful player.
        if (game.log().getCompletedQuestCount() > completedAtLastSave) {
            completedAtLastSave = game.log().getCompletedQuestCount();
            game.save();
            // Once, quit and resume from that save, to prove a run survives it.
            if (!reloadedOnPurpose) {
                reloadedOnPurpose = true;
                const int goldBefore = partyGold(game);
                const int levelBefore = game.party().getLeader()->getLevel();
                const std::string mapBefore = game.mapName();
                game.load();
                expect(partyGold(game) == goldBefore && game.party().getLeader()->getLevel() == levelBefore &&
                       game.mapName() == mapBefore && game.log().getCompletedQuestCount() == completedAtLastSave,
                       "resuming a save should change nothing");
                continue;
            }
        }

        // A hot meal when the party is in a bad way and the stall is in town.
        if (game.mapName() == "Neo-Kyoto Central" && partyIsHurt(game) && partyGold(game) >= 20) {
            const Walk walk = walkToward(game, "Neo-Kyoto Central",
                                         tilesBeside(game, "Neo-Kyoto Central", "Street Vendor"));
            if (walk == Walk::ARRIVED) {
                game.talk("Street Vendor", 0);
                expect(!partyIsHurt(game), "the vendor's meal should restore the party");
                ++game.stats.meals;
            }
            continue;
        }

        // Nothing tracked yet: the story starts with Fixer Kaga.
        std::string map = "Neo-Kyoto Central";
        std::string npc = "Fixer Kaga";
        std::vector<Tile> fightAt;
        bool justTravel = false;
        if (game.story().getTrackedObjective(questName, objective)) {
            if (objective == "Travel to Neon Alley") {
                map = "Neon Alley"; justTravel = true;
            } else if (objective == "Defeat the street thugs" || objective == "Defeat the Corporate Enforcer") {
                map = objective == "Defeat the street thugs" ? "Neon Alley" : "Corporate Plaza";
                if (game.mapName() == map) {
                    StoryDirector::EncounterSpec spec;
                    expect(game.story().getPendingEncounter(map, spec), "the tracked fight should be waiting");
                    fightAt = {{static_cast<int>(spec.x), static_cast<int>(spec.y)}};
                } else {
                    justTravel = true;
                }
            } else if (objective == "Return the data chip to Fixer Kaga") {
                // Before going back: the plaza gate should still be shut.
                if (!triedLockedPlaza) {
                    game.clearBlockedMessage();
                    std::vector<Tile> gate;
                    exitToward("Neon Alley", "Corporate Plaza", gate);
                    const Walk walk = walkToward(game, "Neon Alley", gate);
                    if (walk == Walk::WIPED) continue;
                    if (!game.lastBlockedMessage().empty()) {
                        expect(game.mapName() == "Neon Alley", "a locked gate turns the party back");
                        triedLockedPlaza = true;
                    } else {
                        expect(walk != Walk::ARRIVED, "standing in the gate should have turned the party back");
                    }
                    continue;
                }
            } else if (objective == "Travel to Corporate Plaza") {
                map = "Corporate Plaza"; justTravel = true;
            } else if (objective == "Confront Exec Tanaka") {
                map = "Corporate Plaza"; npc = "Exec Tanaka";
            } else if (objective == "Return to Neo-Kyoto Central") {
                justTravel = true;
            } else if (objective == "Report to the Info Broker") {
                npc = "Info Broker";
            } else {
                expect(false, "the scripted player does not know the objective '" + objective + "'");
            }
        }

        if (justTravel) {
            if (game.mapName() != map) {
                walkToward(game, map, {});
            }
            // Arriving ticks the travel objective; the next turn reads the new one.
            continue;
        }
        if (!fightAt.empty()) {
            // Walking onto the marker starts the fight; winning clears the objective.
            walkToward(game, map, fightAt);
            continue;
        }
        const Walk walk = walkToward(game, map, tilesBeside(game, map, npc));
        if (walk == Walk::ARRIVED) {
            game.talk(npc, 0);
        }
    }

    expect(game.endingSeen(), "the scripted player should reach the ending");
    expect(game.story().isStoryComplete() && game.log().getCompletedQuestCount() == 3u,
           "all three quests should be complete");
    expect(triedLockedPlaza, "the locked plaza gate should have been tried");
    expect(game.stats.storyBattles >= 2, "both story fights should have been fought");
    expect(game.stats.loads >= 1, "the run should have resumed from a save at least once");

    Outcome outcome;
    outcome.stats = game.stats;
    outcome.level = game.party().getLeader()->getLevel();
    outcome.gold = partyGold(game);
    // Walking at the game's speed, battle time as simulated (the screen adds
    // animation on top), and a few seconds to read each line of dialogue.
    const float seconds = static_cast<float>(game.stats.steps) / TILES_PER_SECOND +
                          game.stats.battleSeconds + static_cast<float>(game.stats.linesRead) * 4.0f;
    outcome.minutes = seconds / 60.0f;
    return outcome;
}

}  // namespace

int main() {
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / ("cyberrayne_playthrough_" + std::to_string(std::rand()));
    std::filesystem::create_directories(dir);
    const std::string savePath = (dir / "savegame.txt").string();

    const Player::CharacterClass classes[] = {Player::CharacterClass::NETRUNNER, Player::CharacterClass::SOLO,
                                              Player::CharacterClass::INFILTRATOR};
    std::vector<std::string> report;
    for (Player::CharacterClass rayneClass : classes) {
        const Outcome outcome = play(rayneClass, savePath);
        const std::string name = Player::classToString(rayneClass);

        // A wipe is allowed (the run reloads and carries on, as a player
        // would), but a slice that kills a careful player again and again is
        // not finished.
        expect(outcome.stats.wipes <= 2, name + " run wiped " + std::to_string(outcome.stats.wipes) + " times");
        expect(outcome.level >= 3, name + " run should end at level 3 or higher");

        char line[256];
        std::snprintf(line, sizeof(line),
                      "%-12s level %d, %3d G, %4d steps, %2d battles (%d story), %d wipes, %d meals, "
                      "%2d items, about %.0f min without battle animation",
                      name.c_str(), outcome.level, outcome.gold, outcome.stats.steps, outcome.stats.battles,
                      outcome.stats.storyBattles, outcome.stats.wipes, outcome.stats.meals,
                      outcome.stats.itemsUsed, static_cast<double>(outcome.minutes));
        report.push_back(line);
    }

    std::error_code ignored;
    std::filesystem::remove_all(dir, ignored);

    std::cout << "\nScripted playthrough of the whole slice:" << std::endl;
    for (const std::string& line : report) {
        std::cout << "  " << line << std::endl;
    }
    std::cout << "PlaythroughTest passed" << std::endl;
    return 0;
}
