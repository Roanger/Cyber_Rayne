#include "../../include/ContentFactory.h"
#include "../../include/QuestLog.h"
#include "../../include/Quest.h"
#include "../../include/World.h"
#include "../../include/Map.h"
#include "../../include/NPC.h"
#include "../../include/DialogueTree.h"
#include "../../include/DialogueNode.h"

#include <iostream>

int ContentFactory::createQuestChain(QuestLog* questLog) {
    if (!questLog) return -1;

    // Quest 1: "The Data Chip" — retrieve a stolen data chip from Neon Alley
    auto quest1 = std::make_unique<Quest>(100, "The Data Chip",
        "Fixer Kaga lost a data chip to street thugs in Neon Alley. Retrieve it.");
    quest1->addObjective(QuestObjective("Travel to Neon Alley"));
    quest1->addObjective(QuestObjective("Defeat the street thugs"));
    quest1->addObjective(QuestObjective("Return the data chip to Fixer Kaga"));
    quest1->setExperienceReward(200);
    quest1->setGoldReward(100);
    questLog->addQuest(std::move(quest1));

    // Quest 2: "Corporate Intrusion" — infiltrate Corporate Plaza (unlocked after Quest 1)
    auto quest2 = std::make_unique<Quest>(101, "Corporate Intrusion",
        "Use the data chip to access Corporate Plaza and confront the exec.");
    quest2->addObjective(QuestObjective("Travel to Corporate Plaza"));
    quest2->addObjective(QuestObjective("Defeat the Corporate Enforcer"));
    quest2->addObjective(QuestObjective("Confront Exec Tanaka"));
    quest2->setExperienceReward(400);
    quest2->setGoldReward(250);
    questLog->addQuest(std::move(quest2));

    // Quest 3: "The Truth" — final confrontation (unlocked after Quest 2)
    auto quest3 = std::make_unique<Quest>(102, "The Truth",
        "Exec Tanaka revealed a conspiracy. Return to Neo-Kyoto Central and report to the Info Broker.");
    quest3->addObjective(QuestObjective("Return to Neo-Kyoto Central"));
    quest3->addObjective(QuestObjective("Report to the Info Broker"));
    quest3->setExperienceReward(600);
    quest3->setGoldReward(300);
    questLog->addQuest(std::move(quest3));

    return 100;
}

void ContentFactory::attachNPCDialogues(World* world) {
    if (!world) return;

    for (const auto& map : world->getMaps()) {
        if (!map) continue;

        for (const auto& npc : map->getNPCs()) {
            if (!npc) continue;

            const std::string& name = npc->getName();
            const std::string& mapName = map->getName();

            if (name == "Fixer Kaga" && mapName == "Neo-Kyoto Central") {
                auto tree = std::make_unique<DialogueTree>(0);

                auto node0 = std::make_unique<DialogueNode>(0, "Fixer Kaga",
                    "Hey choom, I lost a data chip to some street thugs in Neon Alley. Can you get it back?");
                node0->addChoice({"I'll help you.", 1});
                node0->addChoice({"Not my problem.", 2});
                tree->addNode(std::move(node0));

                auto node1 = std::make_unique<DialogueNode>(1, "Fixer Kaga",
                    "Good. Head through the north exit to Neon Alley. Watch your back.");
                node1->setAutoAdvanceTarget(-1);
                tree->addNode(std::move(node1));

                auto node2 = std::make_unique<DialogueNode>(2, "Fixer Kaga",
                    "Your loss. Come back if you change your mind.");
                node2->setAutoAdvanceTarget(-1);
                tree->addNode(std::move(node2));

                npc->setDialogueTree(std::move(tree));
            } else if (name == "Info Broker" && mapName == "Neo-Kyoto Central") {
                auto tree = std::make_unique<DialogueTree>(0);

                auto node0 = std::make_unique<DialogueNode>(0, "Info Broker",
                    "I deal in secrets. You got creds?");
                node0->addChoice({"What do you know about the corporations?", 1});
                node0->addChoice({"Just browsing.", 2});
                tree->addNode(std::move(node0));

                auto node1 = std::make_unique<DialogueNode>(1, "Info Broker",
                    "Tanaka Corp has been running experiments on people. The data chip Kaga lost? It's proof.");
                node1->setAutoAdvanceTarget(-1);
                tree->addNode(std::move(node1));

                auto node2 = std::make_unique<DialogueNode>(2, "Info Broker",
                    "Come back when you're serious.");
                node2->setAutoAdvanceTarget(-1);
                tree->addNode(std::move(node2));

                npc->setDialogueTree(std::move(tree));
            } else if (name == "Smuggler Ryo" && mapName == "Neon Alley") {
                auto tree = std::make_unique<DialogueTree>(0);

                auto node0 = std::make_unique<DialogueNode>(0, "Smuggler Ryo",
                    "Pssst. You looking for something? I might have what you need.");
                node0->addChoice({"Where are the thugs with the data chip?", 1});
                node0->addChoice({"Just passing through.", 2});
                tree->addNode(std::move(node0));

                auto node1 = std::make_unique<DialogueNode>(1, "Smuggler Ryo",
                    "They're holed up near the north end. Be careful, they've got a Combat Cyborg.");
                node1->setAutoAdvanceTarget(-1);
                tree->addNode(std::move(node1));

                auto node2 = std::make_unique<DialogueNode>(2, "Smuggler Ryo",
                    "Suit yourself.");
                node2->setAutoAdvanceTarget(-1);
                tree->addNode(std::move(node2));

                npc->setDialogueTree(std::move(tree));
            } else if (name == "Exec Tanaka" && mapName == "Corporate Plaza") {
                auto tree = std::make_unique<DialogueTree>(0);

                auto node0 = std::make_unique<DialogueNode>(0, "Exec Tanaka",
                    "So you're the one causing all the trouble. Do you even know what that data chip contains?");
                node0->addChoice({"The truth about your experiments.", 1});
                node0->addChoice({"Give it up, Tanaka.", 2});
                tree->addNode(std::move(node0));

                auto node1 = std::make_unique<DialogueNode>(1, "Exec Tanaka",
                    "Experiments? We're saving humanity. You're just too blind to see it. But it doesn't matter now.");
                node1->setAutoAdvanceTarget(-1);
                tree->addNode(std::move(node1));

                auto node2 = std::make_unique<DialogueNode>(2, "Exec Tanaka",
                    "You think you've won? This is bigger than you can imagine.");
                node2->setAutoAdvanceTarget(-1);
                tree->addNode(std::move(node2));

                npc->setDialogueTree(std::move(tree));
            }
        }
    }
}
