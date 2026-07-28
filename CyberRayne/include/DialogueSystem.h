#pragma once

#include <memory>
#include <string>

#include "DialogueTree.h"
#include "InputAction.h"

// Manages the active dialogue state: which node is showing,
// which choice is selected, and advancing through the conversation.
// Gameplay logic only — no rendering dependencies.
class DialogueSystem {
public:
    enum class State {
        IDLE,           // No dialogue active.
        SHOWING_NODE,   // A dialogue node is on screen.
        SHOWING_CHOICES,// Choices are being presented.
        FINISHED        // Dialogue has ended.
    };

    DialogueSystem();
    ~DialogueSystem() = default;

    // Start a dialogue tree. Returns false if the tree is empty.
    bool startDialogue(std::unique_ptr<DialogueTree> tree);

    // Advance the dialogue: confirm to proceed past a node without choices,
    // or confirm a selected choice.
    void handleInput(InputAction action);

    // Query current state.
    State getState() const { return m_state; }
    bool isActive() const { return m_state != State::IDLE && m_state != State::FINISHED; }
    bool isFinished() const { return m_state == State::FINISHED; }

    // Access the current node being displayed.
    const DialogueNode* getCurrentNode() const { return m_currentNode; }

    // Get the index of the currently highlighted choice.
    int getSelectedChoiceIndex() const { return m_selectedChoice; }

    // Get the tree being shown (may be null when idle).
    const DialogueTree* getTree() const { return m_tree.get(); }

    // Take ownership of the finished tree back (useful for re-triggering).
    std::unique_ptr<DialogueTree> releaseTree();

private:
    void transitionToNode(int nodeId);
    void selectChoice(int index);
    void advance();

    std::unique_ptr<DialogueTree> m_tree;
    const DialogueNode* m_currentNode;
    State m_state;
    int m_selectedChoice;
};
