#include "../../include/ContentFactory.h"
#include "../../include/QuestLog.h"
#include "../../include/Quest.h"
#include "../../include/World.h"
#include "../../include/Map.h"
#include "../../include/NPC.h"
#include "../../include/DialogueTree.h"
#include "../../include/DialogueNode.h"
#include "../../include/StoryDirector.h"
#include "../../include/Party.h"

#include <iostream>

int ContentFactory::createQuestChain(QuestLog* questLog) {
    if (!questLog) return -1;

    // Quest 1: "The Data Chip" — retrieve a stolen data chip from Neon Alley
    auto quest1 = std::make_unique<Quest>(100, "The Data Chip",
        "Fixer Kaga lost a data chip to street thugs in Neon Alley. Retrieve it.");
    quest1->addObjective(QuestObjective("Travel to Neon Alley"));
    quest1->addObjective(QuestObjective("Defeat the street thugs"));
    quest1->addObjective(QuestObjective("Return the data chip to Fixer Kaga"));
    // Enough, with the fights on the way, to reach level 3 before the plaza.
    quest1->setExperienceReward(350);
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

void ContentFactory::grantStartingKit(Party* party) {
    if (!party) return;
    Inventory& bag = party->getBag();
    bag.add("stim_pack", 3);
    bag.add("defib_patch", 1);
    bag.add("shock_baton", 1);
    bag.add("street_jacket", 1);
}

void ContentFactory::attachNPCDialogues(World* world) {
    if (!world) return;

    // StoryDirector is the single source of NPC dialogue. Attach what each NPC
    // says at the very start of the story; the running game asks the director
    // for a fresh tree on every conversation instead.
    QuestLog openingState;
    createQuestChain(&openingState);
    StoryDirector director(&openingState, nullptr);

    for (const auto& map : world->getMaps()) {
        if (!map) continue;
        for (const auto& npc : map->getNPCs()) {
            if (!npc) continue;
            npc->setDialogueTree(director.buildDialogue(npc->getName()));
        }
    }
}
