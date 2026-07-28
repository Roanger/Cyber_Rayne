#pragma once

#include <string>
#include <memory>

class DialogueTree;

class NPC {
public:
    NPC(const std::string& name, float x, float y);
    ~NPC();

    // Basic properties
    const std::string& getName() const { return m_name; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }

    // Dialogue support
    void setDialogueTree(std::unique_ptr<DialogueTree> tree);
    DialogueTree* getDialogueTree() const { return m_dialogueTree.get(); }
    bool hasDialogue() const { return m_dialogueTree != nullptr; }

    // Interaction
    void interact();
    bool isInteractable() const { return m_interactable; }
    void setInteractable(bool interactable) { m_interactable = interactable; }

    // Update and render
    void update(float deltaTime);
    void render();

private:
    std::string m_name;
    float m_x;
    float m_y;
    bool m_interactable;
    std::unique_ptr<DialogueTree> m_dialogueTree;
};
