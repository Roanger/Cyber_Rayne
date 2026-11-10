#pragma once

#include <deque>
#include <memory>
#include <string>
#include <vector>

class VulkanRenderer;
class TextRenderer;
class DialogueSystem;

// What the party got out of a won battle, for the victory card.
struct BattleSummary {
    int experience = 0;
    int gold = 0;
    // One line per member who gained a level, e.g. "Rayne reached level 3".
    std::vector<std::string> levelUps;
    // Members who were down at the end and got back up.
    std::vector<std::string> revived;
};

// Exploration overlay: quest tracker, talk prompt, toast messages, the dialogue
// box, and the ending card. Draws only; all story state comes from callers.
class StoryHud {
public:
    StoryHud();
    ~StoryHud();

    bool initialize(VulkanRenderer* renderer);

    // Queue a message to show briefly at the top of the screen.
    void pushToast(const std::string& message);
    void clearToasts();
    void update(float deltaTime);

    // questName empty hides the tracker; prompt empty hides the prompt.
    void renderExploration(VulkanRenderer* renderer, const std::string& mapName,
                           const std::string& questName, const std::string& objective,
                           const std::string& prompt);
    void renderDialogue(VulkanRenderer* renderer, const DialogueSystem& dialogue);
    void renderEnding(VulkanRenderer* renderer);
    void renderToast(VulkanRenderer* renderer);
    void renderBattleResult(VulkanRenderer* renderer, const BattleSummary& summary);
    void renderGameOver(VulkanRenderer* renderer, bool canLoad);

    // Shared with other overlays so the font is uploaded once.
    TextRenderer* text() const { return m_text.get(); }

    // Seconds each toast stays on screen.
    static constexpr float TOAST_SECONDS = 3.5f;

private:
    std::unique_ptr<TextRenderer> m_text;
    std::deque<std::string> m_toasts;
    float m_toastTimer = 0.0f;
};
