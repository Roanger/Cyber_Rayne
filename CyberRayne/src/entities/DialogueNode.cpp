#include "../../include/DialogueNode.h"

DialogueNode::DialogueNode(int id, const std::string& speaker, const std::string& text)
    : m_id(id), m_speaker(speaker), m_text(text), m_autoAdvanceTarget(-1) {}

void DialogueNode::addChoice(const DialogueChoice& choice) {
    m_choices.push_back(choice);
}
