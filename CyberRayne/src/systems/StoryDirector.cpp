#include "../../include/StoryDirector.h"
#include "../../include/DialogueNode.h"
#include "../../include/DialogueTree.h"
#include "../../include/Party.h"
#include "../../include/Quest.h"
#include "../../include/QuestLog.h"

namespace {

const char* const MAP_CENTRAL = "Neo-Kyoto Central";
const char* const MAP_ALLEY = "Neon Alley";
const char* const MAP_PLAZA = "Corporate Plaza";

// Objective indices, matching ContentFactory::createQuestChain.
constexpr std::size_t CHIP_TRAVEL = 0;
constexpr std::size_t CHIP_DEFEAT_THUGS = 1;
constexpr std::size_t CHIP_RETURN = 2;
constexpr std::size_t INTRUSION_TRAVEL = 0;
constexpr std::size_t INTRUSION_DEFEAT_ENFORCER = 1;
constexpr std::size_t INTRUSION_CONFRONT = 2;
constexpr std::size_t TRUTH_TRAVEL = 0;
constexpr std::size_t TRUTH_REPORT = 1;

// A node with no choices. next = -1 ends the conversation.
void addLine(DialogueTree& tree, int id, const std::string& speaker,
             const std::string& text, int next = -1) {
    auto node = std::make_unique<DialogueNode>(id, speaker, text);
    node->setAutoAdvanceTarget(next);
    tree.addNode(std::move(node));
}

void addQuestion(DialogueTree& tree, int id, const std::string& speaker,
                 const std::string& text,
                 const std::vector<DialogueChoice>& choices) {
    auto node = std::make_unique<DialogueNode>(id, speaker, text);
    for (const auto& choice : choices) {
        node->addChoice(choice);
    }
    tree.addNode(std::move(node));
}

}  // namespace

StoryDirector::StoryDirector(QuestLog* questLog, Party* party)
    : m_questLog(questLog), m_party(party) {}

// --- State helpers -----------------------------------------------------------

bool StoryDirector::questActive(int questId) const {
    const Quest* quest = m_questLog ? m_questLog->getQuest(questId) : nullptr;
    return quest && quest->getState() == Quest::State::ACTIVE;
}

bool StoryDirector::questCompleted(int questId) const {
    const Quest* quest = m_questLog ? m_questLog->getQuest(questId) : nullptr;
    return quest && quest->getState() == Quest::State::COMPLETED;
}

bool StoryDirector::objectiveDone(int questId, std::size_t index) const {
    const Quest* quest = m_questLog ? m_questLog->getQuest(questId) : nullptr;
    if (!quest || index >= quest->getObjectives().size()) {
        return false;
    }
    return quest->getObjectives()[index].isCompleted();
}

void StoryDirector::startQuest(int questId) {
    if (!m_questLog || !m_questLog->startQuest(questId)) {
        return;
    }
    if (const Quest* quest = m_questLog->getQuest(questId)) {
        m_notifications.push_back("New quest: " + quest->getName());
    }
}

void StoryDirector::completeObjective(int questId, std::size_t index) {
    if (!questActive(questId) || objectiveDone(questId, index)) {
        return;
    }
    if (!m_questLog->completeObjective(questId, index)) {
        return;
    }

    const Quest* quest = m_questLog->getQuest(questId);
    if (!quest) {
        return;
    }

    if (quest->getState() == Quest::State::COMPLETED) {
        payRewards(questId);
    } else {
        m_notifications.push_back("Done: " + quest->getObjectives()[index].getDescription());
    }
}

int StoryDirector::partyGold() const {
    int total = 0;
    if (m_party) {
        for (const auto& member : m_party->getMembers()) {
            if (member) total += member->getGold();
        }
    }
    return total;
}

void StoryDirector::payRewards(int questId) {
    const Quest* quest = m_questLog ? m_questLog->getQuest(questId) : nullptr;
    if (!quest) {
        return;
    }

    // Every member gets the full XP so companions keep pace with Rayne; gold
    // goes to the leader, who carries the party's purse.
    if (m_party) {
        for (const auto& member : m_party->getMembers()) {
            if (member) {
                member->addExperience(quest->getExperienceReward());
            }
        }
        if (Player* leader = m_party->getLeader()) {
            leader->addGold(quest->getGoldReward());
        }
    }

    m_notifications.push_back("Quest complete: " + quest->getName() + "  +" +
                              std::to_string(quest->getExperienceReward()) + " XP  +" +
                              std::to_string(quest->getGoldReward()) + " G");

    // Item rewards, by quest.
    struct ItemReward { int questId; const char* itemId; int count; };
    static const ItemReward itemRewards[] = {
        {QUEST_DATA_CHIP, "mono_katana", 1},
        {QUEST_DATA_CHIP, "stim_pack", 2},
        {QUEST_INTRUSION, "kevlar_weave", 1},
        {QUEST_INTRUSION, "trauma_kit", 1},
    };
    if (m_party) {
        for (const ItemReward& reward : itemRewards) {
            if (reward.questId != questId) continue;
            if (!m_party->getBag().add(reward.itemId, reward.count)) continue;
            const Inventory::Stack* stack = m_party->getBag().find(reward.itemId);
            m_notifications.push_back("Received: " + stack->prototype->getName() +
                                      (reward.count > 1 ? "  x" + std::to_string(reward.count) : ""));
        }
    }
}

// --- Events ------------------------------------------------------------------

void StoryDirector::onMapEntered(const std::string& mapName) {
    if (mapName == MAP_ALLEY) {
        completeObjective(QUEST_DATA_CHIP, CHIP_TRAVEL);
    } else if (mapName == MAP_PLAZA) {
        completeObjective(QUEST_INTRUSION, INTRUSION_TRAVEL);
    } else if (mapName == MAP_CENTRAL) {
        completeObjective(QUEST_TRUTH, TRUTH_TRAVEL);
    }
}

void StoryDirector::onEncounterWon(const std::string& tag) {
    if (tag == ENCOUNTER_THUGS) {
        completeObjective(QUEST_DATA_CHIP, CHIP_DEFEAT_THUGS);
    } else if (tag == ENCOUNTER_ENFORCER) {
        completeObjective(QUEST_INTRUSION, INTRUSION_DEFEAT_ENFORCER);
    }
}

void StoryDirector::onDialogueNode(const std::string& npcName, int nodeId) {
    // Each effect re-checks its precondition, so a node ID alone can never
    // skip ahead in the story.
    if (npcName == "Fixer Kaga" && nodeId == NODE_ACCEPT_CHIP_JOB) {
        startQuest(QUEST_DATA_CHIP);
    } else if (npcName == "Fixer Kaga" && nodeId == NODE_RETURN_CHIP) {
        if (objectiveDone(QUEST_DATA_CHIP, CHIP_DEFEAT_THUGS)) {
            completeObjective(QUEST_DATA_CHIP, CHIP_RETURN);
            if (questCompleted(QUEST_DATA_CHIP)) {
                startQuest(QUEST_INTRUSION);
            }
        }
    } else if (npcName == "Exec Tanaka" && nodeId == NODE_CONFRONT_TANAKA) {
        if (objectiveDone(QUEST_INTRUSION, INTRUSION_DEFEAT_ENFORCER)) {
            completeObjective(QUEST_INTRUSION, INTRUSION_CONFRONT);
            if (questCompleted(QUEST_INTRUSION)) {
                startQuest(QUEST_TRUTH);
            }
        }
    } else if (npcName == "Street Vendor" && nodeId == NODE_VENDOR_MEAL) {
        // Take the price from whoever is carrying gold, leader first.
        if (m_party && partyGold() >= VENDOR_MEAL_COST) {
            int owed = VENDOR_MEAL_COST;
            for (const auto& member : m_party->getMembers()) {
                if (!member || owed <= 0) continue;
                const int paid = member->getGold() < owed ? member->getGold() : owed;
                if (paid > 0 && member->spendGold(paid)) {
                    owed -= paid;
                }
            }
            for (const auto& member : m_party->getMembers()) {
                if (!member) continue;
                member->setHealth(member->getMaxHealth());
                member->setMana(member->getMaxMana());
            }
            m_notifications.push_back("The party is fully restored.");
        }
    } else if (npcName == "Info Broker" && nodeId == NODE_REPORT_TO_BROKER) {
        if (objectiveDone(QUEST_TRUTH, TRUTH_TRAVEL)) {
            const bool wasComplete = questCompleted(QUEST_TRUTH);
            completeObjective(QUEST_TRUTH, TRUTH_REPORT);
            if (!wasComplete && questCompleted(QUEST_TRUTH)) {
                m_endingTriggered = true;
            }
        }
    }
}

// --- Queries -----------------------------------------------------------------

std::unique_ptr<DialogueTree> StoryDirector::buildDialogue(const std::string& npcName) const {
    auto tree = std::make_unique<DialogueTree>(0);
    const std::string& who = npcName;

    if (who == "Fixer Kaga") {
        if (questCompleted(QUEST_DATA_CHIP)) {
            if (questActive(QUEST_INTRUSION)) {
                addLine(*tree, 0, who, "The plaza gate is at the north end of Neon Alley. "
                                       "That chip will get you through. Go see what Tanaka is hiding.");
            } else {
                addLine(*tree, 0, who, "You did good, choom. This city owes you one.");
            }
        } else if (questActive(QUEST_DATA_CHIP)) {
            if (objectiveDone(QUEST_DATA_CHIP, CHIP_DEFEAT_THUGS)) {
                addLine(*tree, 0, who, "You got it back? Let me see... This is Tanaka Corp "
                                       "encryption. No wonder they wanted it gone.",
                        NODE_RETURN_CHIP);
                addLine(*tree, NODE_RETURN_CHIP, who,
                        "Keep the chip. It opens the gate to Corporate Plaza, north of the alley. "
                        "Find Exec Tanaka and make him talk. Here's your cut.");
            } else {
                addLine(*tree, 0, who, "The thugs ran north into Neon Alley with my chip. "
                                       "Take the north exit and get it back.");
            }
        } else {
            addQuestion(*tree, 0, who,
                        "Hey choom, I lost a data chip to some street thugs in Neon Alley. "
                        "Can you get it back?",
                        {{"I'll help you.", NODE_ACCEPT_CHIP_JOB}, {"Not my problem.", 2}});
            addLine(*tree, NODE_ACCEPT_CHIP_JOB, who,
                    "Good. Head through the north exit to Neon Alley. Watch your back.");
            addLine(*tree, 2, who, "Your loss. Come back if you change your mind.");
        }
    } else if (who == "Info Broker") {
        if (questCompleted(QUEST_TRUTH)) {
            addLine(*tree, 0, who, "Every feed in Neo-Kyoto is burning with it. Stay low, Rayne.");
        } else if (questActive(QUEST_TRUTH) && objectiveDone(QUEST_TRUTH, TRUTH_TRAVEL)) {
            addLine(*tree, 0, who, "You look like you've seen a ghost. What did Tanaka tell you?",
                    NODE_REPORT_TO_BROKER);
            addLine(*tree, NODE_REPORT_TO_BROKER, who,
                    "Human experiments, sold as salvation. I'll push this to every feed in the "
                    "city before sunrise. Tanaka Corp can't bury it now. Lie low. "
                    "This is only the beginning.");
        } else {
            addQuestion(*tree, 0, who, "I deal in secrets. You got creds?",
                        {{"What do you know about the corporations?", 1}, {"Just browsing.", 2}});
            addLine(*tree, 1, who, "Tanaka Corp has been running experiments on people. "
                                   "The data chip Kaga lost? It's proof.");
            addLine(*tree, 2, who, "Come back when you're serious.");
        }
    } else if (who == "Smuggler Ryo") {
        if (objectiveDone(QUEST_DATA_CHIP, CHIP_DEFEAT_THUGS)) {
            addLine(*tree, 0, who, "You dropped those thugs? Remind me not to owe you money.");
        } else {
            addQuestion(*tree, 0, who,
                        "Pssst. You looking for something? I might have what you need.",
                        {{"Where are the thugs with the data chip?", 1}, {"Just passing through.", 2}});
            addLine(*tree, 1, who, "They're holed up near the north end. "
                                   "Be careful, they've got a Combat Cyborg.");
            addLine(*tree, 2, who, "Suit yourself.");
        }
    } else if (who == "Exec Tanaka") {
        if (questCompleted(QUEST_INTRUSION)) {
            addLine(*tree, 0, who, "We are not finished, you and I.");
        } else if (questActive(QUEST_INTRUSION) &&
                   objectiveDone(QUEST_INTRUSION, INTRUSION_DEFEAT_ENFORCER)) {
            addQuestion(*tree, 0, who,
                        "So you're the one causing all the trouble. "
                        "Do you even know what that data chip contains?",
                        {{"The truth about your experiments.", 1}, {"Give it up, Tanaka.", 2}});
            addLine(*tree, 1, who, "Experiments? We're saving humanity. "
                                   "You're just too blind to see it.",
                    NODE_CONFRONT_TANAKA);
            addLine(*tree, 2, who, "You think you've won? This is bigger than you can imagine.",
                    NODE_CONFRONT_TANAKA);
            addLine(*tree, NODE_CONFRONT_TANAKA, who,
                    "Run to your Info Broker with what you know. It changes nothing.");
        } else if (questActive(QUEST_INTRUSION)) {
            addLine(*tree, 0, who, "Enforcer! Remove these intruders from my plaza.");
        } else {
            addLine(*tree, 0, who, "This plaza is private property.");
        }
    } else if (who == "Street Vendor") {
        const std::string price = std::to_string(VENDOR_MEAL_COST) + " G";
        // The reply is decided now, from what the party can afford, so the
        // meal node is only reachable when it can be paid for.
        const bool canPay = partyGold() >= VENDOR_MEAL_COST;
        addQuestion(*tree, 0, who,
                    "Synth-noodles, hot and cheap! A round for your crew puts everyone "
                    "back on their feet. " + price + ".",
                    {{"A round for everyone. (" + price + ")", canPay ? NODE_VENDOR_MEAL : 1},
                     {"Not right now.", 2}});
        addLine(*tree, NODE_VENDOR_MEAL, who, "Eat up. You look like you needed it.");
        addLine(*tree, 1, who, "You're short, choom. Come back with " + price + ".");
        addLine(*tree, 2, who, "Suit yourself. I'll be here.");
    } else if (who == "Hacker Zero") {
        addLine(*tree, 0, who, "Black ICE is crawling all over this alley. "
                               "Hit it with tech, not fists.");
    } else if (who == "Security Chief") {
        addLine(*tree, 0, who, "Move along. Corporate business.");
    } else {
        addLine(*tree, 0, who, "...");
    }

    return tree;
}

bool StoryDirector::isMapUnlocked(const std::string& mapName) const {
    if (mapName == MAP_PLAZA) {
        // The recovered chip is the gate pass.
        return questCompleted(QUEST_DATA_CHIP);
    }
    return true;
}

std::string StoryDirector::getLockedMessage(const std::string& mapName) const {
    if (mapName == MAP_PLAZA) {
        return "The plaza gate is sealed. You need an access chip.";
    }
    return "You can't go that way yet.";
}

bool StoryDirector::getPendingEncounter(const std::string& mapName, EncounterSpec& out) const {
    if (mapName == MAP_ALLEY && questActive(QUEST_DATA_CHIP) &&
        objectiveDone(QUEST_DATA_CHIP, CHIP_TRAVEL) &&
        !objectiveDone(QUEST_DATA_CHIP, CHIP_DEFEAT_THUGS)) {
        out.tag = ENCOUNTER_THUGS;
        out.mapName = MAP_ALLEY;
        out.x = 15.0f;
        out.y = 5.0f;
        out.retreatX = 15.0f;
        out.retreatY = 8.0f;
        // Tuned for a level 1 party of any class; see AGENTS.md "Combat balance".
        out.enemies = {{"StreetThug", 2}, {"StreetThug", 2}, {"CombatCyborg", 3}};
        return true;
    }

    if (mapName == MAP_PLAZA && questActive(QUEST_INTRUSION) &&
        objectiveDone(QUEST_INTRUSION, INTRUSION_TRAVEL) &&
        !objectiveDone(QUEST_INTRUSION, INTRUSION_DEFEAT_ENFORCER)) {
        out.tag = ENCOUNTER_ENFORCER;
        out.mapName = MAP_PLAZA;
        out.x = 12.0f;
        out.y = 16.0f;
        out.retreatX = 12.0f;
        out.retreatY = 18.0f;
        // Tuned for a level 3 party, which the first quest's XP guarantees.
        out.enemies = {{"CorporateEnforcer", 3}, {"SecurityDrone", 3}, {"SecurityDrone", 3}};
        return true;
    }

    return false;
}

bool StoryDirector::getTrackedObjective(std::string& questName, std::string& objective) const {
    if (!m_questLog) {
        return false;
    }
    for (const auto& quest : m_questLog->getQuests()) {
        if (!quest || quest->getState() != Quest::State::ACTIVE) {
            continue;
        }
        for (const auto& obj : quest->getObjectives()) {
            if (!obj.isCompleted()) {
                questName = quest->getName();
                objective = obj.getDescription();
                return true;
            }
        }
    }
    return false;
}

bool StoryDirector::isStoryComplete() const {
    return questCompleted(QUEST_TRUTH);
}

bool StoryDirector::consumeEndingTrigger() {
    const bool triggered = m_endingTriggered;
    m_endingTriggered = false;
    return triggered;
}

std::vector<std::string> StoryDirector::takeNotifications() {
    std::vector<std::string> out;
    out.swap(m_notifications);
    return out;
}
