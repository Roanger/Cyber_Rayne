#pragma once

#include <string>
#include <vector>

// A single branching choice within a dialogue node.
struct DialogueChoice {
    std::string text;           // The choice text shown to the player.
    int targetNodeId;           // ID of the node this choice leads to. -1 ends the dialogue.
};

// A single entry in a dialogue tree: speaker name, spoken text, and optional choices.
class DialogueNode {
public:
    DialogueNode(int id, const std::string& speaker, const std::string& text);
    ~DialogueNode() = default;

    int getId() const { return m_id; }
    const std::string& getSpeaker() const { return m_speaker; }
    const std::string& getText() const { return m_text; }

    void addChoice(const DialogueChoice& choice);
    const std::vector<DialogueChoice>& getChoices() const { return m_choices; }

    // If there are no choices, the node auto-advances to this node.
    // -1 means the dialogue ends after this node.
    void setAutoAdvanceTarget(int targetId) { m_autoAdvanceTarget = targetId; }
    int getAutoAdvanceTarget() const { return m_autoAdvanceTarget; }

    bool hasChoices() const { return !m_choices.empty(); }

private:
    int m_id;
    std::string m_speaker;
    std::string m_text;
    std::vector<DialogueChoice> m_choices;
    int m_autoAdvanceTarget;  // Used when there are no explicit choices.
};
