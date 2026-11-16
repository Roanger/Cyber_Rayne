#pragma once

#include "../InputAction.h"

#include <cstddef>
#include <string>
#include <vector>

class VulkanRenderer;
class TextRenderer;
class SpriteLibrary;

// A sequence of full-screen pictures, each with a caption that types itself
// out. Confirm finishes the caption, then moves on; Cancel or the menu button
// skips the rest. Stepping through it is plain logic (tested in MenuTest);
// drawing is behind NO_VULKAN.
class Cutscene {
public:
    struct Panel {
        std::string image;    // assets/cutscenes/<image>.png
        std::string caption;
    };

    // The opening shown when a new game starts.
    static std::vector<Panel> introPanels();
    // The picture shown when the story's last quest is done, before the
    // closing card.
    static std::vector<Panel> endingPanels();

    // lastHint names what Confirm does on the final panel ("begin", "continue").
    void start(std::vector<Panel> panels, std::string lastHint = "continue");
    void update(float deltaTime);
    void handleInput(InputAction action);

    // True once the last panel has been dismissed, or it was skipped, or it
    // was never started.
    bool isFinished() const { return m_index >= m_panels.size(); }
    std::size_t getPanelIndex() const { return m_index; }
    std::size_t getPanelCount() const { return m_panels.size(); }
    // The part of the current caption that has been typed so far.
    std::string getVisibleCaption() const;
    bool isCaptionComplete() const;

    void render(VulkanRenderer* renderer, TextRenderer* text, SpriteLibrary* sprites) const;

    static constexpr float CHARS_PER_SECOND = 45.0f;
    static constexpr float FADE_SECONDS = 0.6f;

private:
    std::vector<Panel> m_panels;
    std::string m_lastHint = "continue";
    std::size_t m_index = 0;
    float m_panelTime = 0.0f;   // seconds the current panel has been up
    bool m_typedOut = false;    // caption forced complete by Confirm
};
