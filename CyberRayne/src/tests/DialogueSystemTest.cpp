#include "../../include/DialogueSystem.h"
#include "../../include/DialogueTree.h"
#include "../../include/DialogueNode.h"
#include "../../include/InputAction.h"
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
} // namespace

int main() {
    // Build a simple branching dialogue:
    //   Node 0 (NPC greeting) -> choices: "Ask about the city" -> Node 1
    //                                        "Say goodbye"      -> end
    //   Node 1 (NPC info)     -> auto-advance -> end

    auto tree = std::make_unique<DialogueTree>(0);

    auto node0 = std::make_unique<DialogueNode>(0, "Fixer", "Hey choom, you new around here?");
    node0->addChoice({"Ask about the city", 1});
    node0->addChoice({"Say goodbye", -1});
    tree->addNode(std::move(node0));

    auto node1 = std::make_unique<DialogueNode>(1, "Fixer", "This is Neo-Kyoto. Watch your back.");
    node1->setAutoAdvanceTarget(-1);
    tree->addNode(std::move(node1));

    DialogueSystem dialogue;
    expect(!dialogue.isActive(), "dialogue should start idle");
    expect(dialogue.startDialogue(std::move(tree)), "startDialogue should succeed with a valid tree");
    expect(dialogue.isActive(), "dialogue should be active after starting");
    expect(dialogue.getState() == DialogueSystem::State::SHOWING_NODE, "should be showing the root node");

    const DialogueNode* current = dialogue.getCurrentNode();
    expect(current != nullptr, "current node should not be null");
    expect(current->getId() == 0, "should be at root node");
    expect(current->getSpeaker() == "Fixer", "speaker should be Fixer");
    expect(current->hasChoices(), "root node should have choices");

    // Confirm to enter choice selection.
    dialogue.handleInput(InputAction::CONFIRM);
    expect(dialogue.getState() == DialogueSystem::State::SHOWING_CHOICES, "should be showing choices after confirm");

    // Navigate down to the second choice ("Say goodbye").
    dialogue.handleInput(InputAction::MOVE_DOWN);
    expect(dialogue.getSelectedChoiceIndex() == 1, "second choice should be selected");

    // Navigate back up to the first choice.
    dialogue.handleInput(InputAction::MOVE_UP);
    expect(dialogue.getSelectedChoiceIndex() == 0, "first choice should be selected");

    // Confirm the first choice ("Ask about the city") -> Node 1.
    dialogue.handleInput(InputAction::CONFIRM);
    expect(dialogue.getCurrentNode()->getId() == 1, "should transition to node 1");
    expect(dialogue.getState() == DialogueSystem::State::SHOWING_NODE, "should be showing node 1");

    // Node 1 has no choices, auto-advance to -1 (end).
    dialogue.handleInput(InputAction::CONFIRM);
    expect(dialogue.isFinished(), "dialogue should be finished after auto-advance to -1");

    // Test empty tree rejection.
    auto emptyTree = std::make_unique<DialogueTree>(0);
    DialogueSystem dialogue2;
    expect(!dialogue2.startDialogue(std::move(emptyTree)), "empty tree should be rejected");

    // Test linear dialogue (no choices, auto-advance chain).
    auto linearTree = std::make_unique<DialogueTree>(0);
    auto ln0 = std::make_unique<DialogueNode>(0, "Shopkeep", "Welcome!");
    ln0->setAutoAdvanceTarget(1);
    linearTree->addNode(std::move(ln0));

    auto ln1 = std::make_unique<DialogueNode>(1, "Shopkeep", "Come again!");
    ln1->setAutoAdvanceTarget(-1);
    linearTree->addNode(std::move(ln1));

    DialogueSystem dialogue3;
    expect(dialogue3.startDialogue(std::move(linearTree)), "linear tree should start");
    expect(dialogue3.getCurrentNode()->getId() == 0, "should start at node 0");
    dialogue3.handleInput(InputAction::CONFIRM);
    expect(dialogue3.getCurrentNode()->getId() == 1, "should advance to node 1");
    dialogue3.handleInput(InputAction::CONFIRM);
    expect(dialogue3.isFinished(), "should finish after node 1 auto-advances to -1");

    std::cout << "DialogueSystemTest passed" << std::endl;
    return 0;
}
