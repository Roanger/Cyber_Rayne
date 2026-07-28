#include "../../include/DialogueSystem.h"
#include <iostream>

DialogueSystem::DialogueSystem()
    : m_currentNode(nullptr), m_state(State::IDLE), m_selectedChoice(0) {}

bool DialogueSystem::startDialogue(std::unique_ptr<DialogueTree> tree) {
    if (!tree || tree->isEmpty()) {
        return false;
    }

    m_tree = std::move(tree);
    m_state = State::SHOWING_NODE;
    m_selectedChoice = 0;
    transitionToNode(m_tree->getRootId());

    if (!m_currentNode) {
        m_state = State::FINISHED;
        return false;
    }

    return true;
}

void DialogueSystem::handleInput(InputAction action) {
    if (m_state == State::IDLE || m_state == State::FINISHED || !m_currentNode) {
        return;
    }

    if (m_state == State::SHOWING_NODE) {
        // Node without choices: confirm advances.
        if (!m_currentNode->hasChoices()) {
            if (action == InputAction::CONFIRM) {
                advance();
            }
            return;
        }

        // Node has choices: confirm enters choice selection mode.
        if (action == InputAction::CONFIRM) {
            m_state = State::SHOWING_CHOICES;
            m_selectedChoice = 0;
        }
        return;
    }

    if (m_state == State::SHOWING_CHOICES) {
        const auto& choices = m_currentNode->getChoices();
        int choiceCount = static_cast<int>(choices.size());

        switch (action) {
            case InputAction::MOVE_UP:
                if (m_selectedChoice > 0) {
                    --m_selectedChoice;
                }
                break;
            case InputAction::MOVE_DOWN:
                if (m_selectedChoice < choiceCount - 1) {
                    ++m_selectedChoice;
                }
                break;
            case InputAction::CONFIRM:
                selectChoice(m_selectedChoice);
                break;
            case InputAction::CANCEL:
                // Cancel returns to showing the node text.
                m_state = State::SHOWING_NODE;
                break;
            default:
                break;
        }
    }
}

void DialogueSystem::transitionToNode(int nodeId) {
    if (!m_tree) {
        m_currentNode = nullptr;
        m_state = State::FINISHED;
        return;
    }

    if (nodeId < 0) {
        m_currentNode = nullptr;
        m_state = State::FINISHED;
        return;
    }

    m_currentNode = m_tree->getNode(nodeId);
    if (!m_currentNode) {
        m_state = State::FINISHED;
    } else {
        m_state = State::SHOWING_NODE;
        m_selectedChoice = 0;
    }
}

void DialogueSystem::selectChoice(int index) {
    if (!m_currentNode) return;

    const auto& choices = m_currentNode->getChoices();
    if (index < 0 || index >= static_cast<int>(choices.size())) {
        return;
    }

    int targetId = choices[index].targetNodeId;
    transitionToNode(targetId);
}

void DialogueSystem::advance() {
    if (!m_currentNode) return;
    transitionToNode(m_currentNode->getAutoAdvanceTarget());
}

std::unique_ptr<DialogueTree> DialogueSystem::releaseTree() {
    m_currentNode = nullptr;
    m_state = State::IDLE;
    m_selectedChoice = 0;
    return std::move(m_tree);
}
