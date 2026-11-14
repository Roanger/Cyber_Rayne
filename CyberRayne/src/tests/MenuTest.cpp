// Pause menu and settings menu behaviour, without a renderer.
#include "../../include/InputConfig.h"
#include "../../include/Party.h"
#include "../../include/ui/PauseMenu.h"
#include "../../include/AudioSettings.h"
#include "../../include/ui/SettingsMenu.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "MenuTest FAILED: " << message << std::endl;
        std::exit(1);
    }
}

using Command = PauseMenu::Command;
using Screen = PauseMenu::Screen;

// Open the menu and put the root cursor on an entry.
void goTo(PauseMenu& menu, int entry) {
    menu.open();
    for (int i = 0; i < entry; ++i) menu.handleInput(InputAction::MOVE_DOWN);
}

}  // namespace

int main() {
    // --- Root navigation -----------------------------------------------------
    {
        PauseMenu menu;
        menu.open();
        expect(menu.getScreen() == Screen::ROOT && menu.getIndex() == PauseMenu::ENTRY_RESUME,
               "the menu opens on Resume");
        expect(static_cast<int>(PauseMenu::entries().size()) == PauseMenu::ENTRY_COUNT,
               "every entry has a label");

        expect(menu.handleInput(InputAction::CONFIRM) == Command::RESUME, "Resume resumes");
        expect(menu.handleInput(InputAction::CANCEL) == Command::RESUME, "Esc on the root resumes");
        expect(menu.handleInput(InputAction::PAUSE) == Command::RESUME, "the pause key resumes");

        expect(menu.handleInput(InputAction::MOVE_UP) == Command::NONE, "moving is not a command");
        expect(menu.getIndex() == PauseMenu::ENTRY_QUIT, "up from the top wraps to the bottom");
        menu.handleInput(InputAction::MOVE_DOWN);
        expect(menu.getIndex() == PauseMenu::ENTRY_RESUME, "down from the bottom wraps to the top");

        goTo(menu, PauseMenu::ENTRY_PARTY);
        expect(menu.handleInput(InputAction::CONFIRM) == Command::NONE && menu.getScreen() == Screen::PARTY,
               "Party opens the party screen");
        menu.handleInput(InputAction::MOVE_DOWN);
        expect(menu.getIndex() == PauseMenu::ENTRY_PARTY, "the root cursor does not move behind a sub-screen");
        expect(menu.handleInput(InputAction::CANCEL) == Command::NONE && menu.getScreen() == Screen::ROOT,
               "Esc returns from a sub-screen instead of resuming");

        goTo(menu, PauseMenu::ENTRY_QUESTS);
        menu.handleInput(InputAction::CONFIRM);
        expect(menu.getScreen() == Screen::QUESTS, "Quests opens the quest screen");
        expect(menu.handleInput(InputAction::OPEN_MENU) == Command::RESUME, "the menu key resumes from anywhere");

        goTo(menu, PauseMenu::ENTRY_SAVE);
        expect(menu.handleInput(InputAction::CONFIRM) == Command::SAVE, "Save Game saves");
        menu.setStatus("Game saved.");
        expect(menu.getStatus() == "Game saved.", "the status line is kept");

        goTo(menu, PauseMenu::ENTRY_SETTINGS);
        expect(menu.handleInput(InputAction::CONFIRM) == Command::OPEN_SETTINGS, "Settings opens settings");

        // Quit needs two presses, and moving away disarms it.
        goTo(menu, PauseMenu::ENTRY_QUIT);
        expect(menu.handleInput(InputAction::CONFIRM) == Command::NONE && menu.isQuitArmed(),
               "the first press on Quit only warns");
        expect(!menu.getStatus().empty(), "the warning is shown");
        menu.handleInput(InputAction::MOVE_UP);
        expect(!menu.isQuitArmed() && menu.getStatus().empty(), "moving away disarms Quit");
        menu.handleInput(InputAction::MOVE_DOWN);
        expect(menu.handleInput(InputAction::CONFIRM) == Command::NONE, "Quit warns again after being disarmed");
        expect(menu.handleInput(InputAction::CONFIRM) == Command::QUIT_TO_TITLE, "the second press quits");

        menu.open();
        expect(!menu.isQuitArmed() && menu.getStatus().empty() && menu.getIndex() == 0,
               "reopening resets the menu");

        // Without a party, the screens that act on one stay shut.
        goTo(menu, PauseMenu::ENTRY_ITEMS);
        menu.handleInput(InputAction::CONFIRM);
        expect(menu.getScreen() == Screen::ROOT, "Items needs a party");
        goTo(menu, PauseMenu::ENTRY_EQUIPMENT);
        menu.handleInput(InputAction::CONFIRM);
        expect(menu.getScreen() == Screen::ROOT, "Equipment needs a party");
    }

    // --- Items screen --------------------------------------------------------
    {
        auto party = Party::createDefaultParty();
        Party* p = party.get();
        Player* rayne = party->getMember(0);
        Player* cyph = party->getMember(1);
        party->getBag().add("stim_pack", 2);
        party->getBag().add("shock_baton", 1);

        PauseMenu menu;
        goTo(menu, PauseMenu::ENTRY_ITEMS);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(menu.getScreen() == Screen::ITEMS && menu.getListIndex() == 0, "Items opens on the first stack");

        // Gear is not usable from here.
        menu.handleInput(InputAction::MOVE_DOWN, p);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(menu.getScreen() == Screen::ITEMS && !menu.getStatus().empty(),
               "choosing gear explains where to equip it");
        menu.handleInput(InputAction::MOVE_DOWN, p);
        expect(menu.getListIndex() == 0, "the list wraps");

        // Use a stim pack on the second member.
        cyph->setHealth(5);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(menu.getScreen() == Screen::ITEM_TARGET && menu.getListIndex() == 0,
               "a consumable asks who to use it on");
        menu.handleInput(InputAction::CONFIRM, p);
        expect(party->getBag().count("stim_pack") == 2 && menu.getScreen() == Screen::ITEM_TARGET,
               "using it on someone healthy wastes nothing and stays on the target list");
        expect(rayne->getHealth() == rayne->getMaxHealth(), "the healthy target is unchanged");
        menu.handleInput(InputAction::MOVE_DOWN, p);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(cyph->getHealth() == 45 && party->getBag().count("stim_pack") == 1, "the wounded member is healed");
        expect(menu.getScreen() == Screen::ITEM_TARGET, "with more left, the target list stays open");

        cyph->setHealth(5);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(party->getBag().count("stim_pack") == 0, "the last one is used");
        expect(menu.getScreen() == Screen::ITEMS, "running out returns to the item list");
        expect(menu.getListIndex() == 0 && party->getBag().getStacks().size() == 1,
               "the cursor lands on a stack that still exists");

        menu.handleInput(InputAction::CANCEL, p);
        expect(menu.getScreen() == Screen::ROOT, "Esc leaves the item list");

        // An empty bag is harmless.
        party->getBag().clear();
        goTo(menu, PauseMenu::ENTRY_ITEMS);
        menu.handleInput(InputAction::CONFIRM, p);
        menu.handleInput(InputAction::MOVE_DOWN, p);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(menu.getScreen() == Screen::ITEMS && menu.getListIndex() == 0, "an empty bag can be browsed safely");
    }

    // --- Equipment screen ----------------------------------------------------
    {
        auto party = Party::createDefaultParty();
        Party* p = party.get();
        Player* cyph = party->getMember(1);
        party->getBag().add("shock_baton", 1);
        party->getBag().add("street_jacket", 1);
        party->getBag().add("stim_pack", 1);
        const int baseAttack = cyph->getEffectiveStrength();
        const int baseDefense = cyph->getEffectiveDefense();

        PauseMenu menu;
        goTo(menu, PauseMenu::ENTRY_EQUIPMENT);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(menu.getScreen() == Screen::EQUIP_MEMBER, "Equipment starts by choosing a member");
        menu.handleInput(InputAction::MOVE_DOWN, p);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(menu.getScreen() == Screen::EQUIP_SLOT && menu.getListIndex() == 0, "then a slot, weapon first");

        menu.handleInput(InputAction::CONFIRM, p);
        expect(menu.getScreen() == Screen::EQUIP_CHOICE, "then a piece from the bag");
        menu.handleInput(InputAction::CONFIRM, p);
        expect(cyph->getEffectiveStrength() == baseAttack + 4 && party->getBag().count("shock_baton") == 0,
               "the chosen weapon goes on the chosen member");
        expect(menu.getScreen() == Screen::EQUIP_SLOT && !menu.getStatus().empty(),
               "equipping reports back and returns to the slots");
        expect(party->getMember(0)->getEquippedWeapon() == nullptr, "nobody else was touched");

        // Armor slot.
        menu.handleInput(InputAction::MOVE_DOWN, p);
        menu.handleInput(InputAction::CONFIRM, p);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(cyph->getEffectiveDefense() == baseDefense + 3, "armor goes in the armor slot");
        expect(menu.getListIndex() == 1, "the slot cursor stays on the slot just edited");

        // Remove the weapon: with nothing of that type left in the bag,
        // "(Remove)" is the only choice.
        menu.handleInput(InputAction::MOVE_UP, p);
        menu.handleInput(InputAction::CONFIRM, p);
        expect(menu.getScreen() == Screen::EQUIP_CHOICE && menu.getListIndex() == 0, "the choice list opens at the top");
        menu.handleInput(InputAction::CONFIRM, p);
        expect(cyph->getEffectiveStrength() == baseAttack && party->getBag().count("shock_baton") == 1,
               "Remove takes the weapon off and returns it to the bag");

        // Removing from an empty slot is reported, not fatal.
        menu.handleInput(InputAction::CONFIRM, p);
        menu.handleInput(InputAction::MOVE_DOWN, p);  // past the baton to (Remove)
        menu.handleInput(InputAction::CONFIRM, p);
        expect(menu.getStatus() == "Nothing to remove.", "removing from an empty slot says so");

        menu.handleInput(InputAction::CANCEL, p);
        expect(menu.getScreen() == Screen::EQUIP_MEMBER && menu.getListIndex() == 1,
               "Esc steps back to the member list, on the same member");
        menu.handleInput(InputAction::CANCEL, p);
        expect(menu.getScreen() == Screen::ROOT, "and then to the root");
        expect(party->getBag().count("stim_pack") == 1, "consumables never show up as gear");
    }

    // --- Key bindings --------------------------------------------------------
    {
        InputConfig config;
        expect(config.isComplete(), "the defaults bind every action");
        expect(InputConfig::allActions().size() == 8, "there are eight rebindable actions");
        for (InputAction action : InputConfig::allActions()) {
            expect(std::string(InputConfig::actionName(action)) != "?", "every action has a name");
            expect(!InputConfig::keyName(config.getKeyForAction(action)).empty(), "every default key has a name");
        }
        expect(InputConfig::keyName('W') == "W" && InputConfig::keyName(32) == "Space", "keys are named readably");

        const int upKey = config.getKeyForAction(InputAction::MOVE_UP);
        const int enterKey = config.getKeyForAction(InputAction::CONFIRM);
        const int escKey = InputConfig::escapeKey();

        SettingsMenu settings;
        settings.open();
        expect(SettingsMenu::rowCount() == 13, "three volumes, eight actions, reset, and back");
        expect(!settings.wantsRawKey(), "nothing is being rebound at first");

        // Volume rows come first and are changed with Left and Right.
        {
            AudioSettings start;
            start.master = 10;
            start.music = 7;
            start.effects = 1;
            settings.setVolumes(start);
            expect(!settings.consumeVolumeChanged(), "setting the volumes is not a change by the player");
            settings.handleInput(InputAction::MOVE_RIGHT, config);
            expect(settings.getVolumes().master == 10 && !settings.consumeVolumeChanged(),
                   "volume stops at the top");
            settings.handleInput(InputAction::MOVE_LEFT, config);
            expect(settings.getVolumes().master == 9 && settings.consumeVolumeChanged() &&
                       !settings.consumeVolumeChanged(),
                   "Left turns the master volume down, reported once");
            expect(settings.handleInput(InputAction::CONFIRM, config) == SettingsMenu::Command::NONE &&
                       !settings.wantsRawKey() && !settings.getStatus().empty(),
                   "Confirm on a volume row explains itself and binds nothing");
            settings.handleInput(InputAction::MOVE_DOWN, config);
            settings.handleInput(InputAction::MOVE_RIGHT, config);
            expect(settings.getVolumes().music == 8 && settings.getVolumes().master == 9, "each row has its own level");
            settings.handleInput(InputAction::MOVE_DOWN, config);
            settings.handleInput(InputAction::MOVE_LEFT, config);
            settings.handleInput(InputAction::MOVE_LEFT, config);
            expect(settings.getVolumes().effects == 0, "volume stops at zero");
            settings.consumeVolumeChanged();
            // On to the first key binding.
            settings.handleInput(InputAction::MOVE_DOWN, config);
            settings.handleInput(InputAction::MOVE_LEFT, config);
            expect(!settings.consumeVolumeChanged(), "Left does nothing on a key binding row");
            expect(settings.getIndex() == SettingsMenu::VOLUME_ROWS, "the key bindings follow the volumes");
        }

        // Volume settings survive a round trip through a file.
        {
            namespace fs = std::filesystem;
            const fs::path dir = fs::temp_directory_path() / "cyberrayne_audiocfg" / "nested";
            fs::remove_all(dir.parent_path());
            const std::string path = (dir / "audio.cfg").string();
            AudioSettings saved;
            saved.master = 4;
            saved.music = 0;
            saved.effects = 9;
            expect(saved.saveToFile(path), "volumes save, creating directories");
            AudioSettings loaded;
            expect(loaded.loadFromFile(path) && loaded.master == 4 && loaded.music == 0 && loaded.effects == 9,
                   "volumes load");
            { std::ofstream out(path); out << "master=99\nmusic=-3\neffects=banana\n"; }
            AudioSettings clamped;
            expect(clamped.loadFromFile(path) && clamped.master == 10 && clamped.music == 0 &&
                       clamped.effects == 10,
                   "out-of-range values are clamped and nonsense is skipped");
            { std::ofstream out(path); out << "garbage\n"; }
            AudioSettings untouched;
            untouched.music = 3;
            expect(!untouched.loadFromFile(path) && untouched.music == 3, "an unusable file changes nothing");
            expect(!untouched.loadFromFile((dir / "missing.cfg").string()), "a missing file is reported");
            fs::remove_all(dir.parent_path());
            expect(AudioSettings::toVolume(7) > 0.69f && AudioSettings::toVolume(7) < 0.71f &&
                       AudioSettings::toVolume(99) == 1.0f,
                   "steps convert to a 0-1 volume");
        }

        // Rebind Move Up to W.
        expect(settings.handleInput(InputAction::CONFIRM, config) == SettingsMenu::Command::NONE &&
                   settings.wantsRawKey(),
               "choosing an action waits for a key");
        expect(settings.handleInput(InputAction::MOVE_DOWN, config) == SettingsMenu::Command::NONE &&
                   settings.getIndex() == SettingsMenu::VOLUME_ROWS,
               "navigation is ignored while waiting for a key");
        settings.handleRawKey('W', config);
        expect(!settings.wantsRawKey(), "one key ends the wait");
        expect(config.getKeyForAction(InputAction::MOVE_UP) == 'W', "the action has its new key");
        expect(config.getActionForKey(upKey) == InputConfig::NO_ACTION, "the old key is freed");
        expect(config.isComplete(), "the bindings are still complete");
        expect(settings.consumeChanged() && !settings.consumeChanged(), "a change is reported once");

        // A key that belongs to another action is refused.
        settings.handleInput(InputAction::CONFIRM, config);
        settings.handleRawKey(enterKey, config);
        expect(config.getKeyForAction(InputAction::MOVE_UP) == 'W' &&
                   config.getActionForKey(enterKey) == InputAction::CONFIRM,
               "a key in use is not stolen");
        expect(!settings.getStatus().empty() && !settings.consumeChanged(), "the refusal is explained and changes nothing");

        // Esc cancels, and cannot be taken from Cancel.
        settings.handleInput(InputAction::CONFIRM, config);
        settings.handleRawKey(escKey, config);
        expect(config.getKeyForAction(InputAction::MOVE_UP) == 'W' &&
                   config.getActionForKey(escKey) == InputAction::CANCEL,
               "Esc backs out of a rebind");

        // Rebinding an action to the key it already has is fine.
        settings.handleInput(InputAction::CONFIRM, config);
        settings.handleRawKey('W', config);
        expect(config.getKeyForAction(InputAction::MOVE_UP) == 'W' && config.isComplete(),
               "re-pressing the same key keeps the binding");

        // Raw keys are ignored unless a rebind is in progress.
        settings.handleRawKey('Q', config);
        expect(config.getActionForKey('Q') == InputConfig::NO_ACTION, "stray keys bind nothing");

        // Round trip through a file.
        namespace fs = std::filesystem;
        const fs::path dir = fs::temp_directory_path() / "cyberrayne_menutest" / "nested";
        fs::remove_all(dir.parent_path());
        const std::string path = (dir / "input.cfg").string();
        expect(config.saveToFile(path), "bindings save, creating directories");
        InputConfig loaded;
        expect(loaded.loadFromFile(path), "bindings load");
        expect(loaded.getKeyForAction(InputAction::MOVE_UP) == 'W' && loaded.isComplete(),
               "the rebind survives the round trip");

        // Unusable files leave the defaults alone.
        InputConfig untouched;
        expect(!untouched.loadFromFile((dir / "missing.cfg").string()) && untouched.isComplete(),
               "a missing file keeps the defaults");
        const char* badFiles[] = {"garbage", "", "87:0;", "87:99;", "x:y;", "87:0;87:1;"};
        for (const char* bad : badFiles) {
            { std::ofstream out(path); out << bad << "\n"; }
            InputConfig victim;
            expect(!victim.loadFromFile(path), std::string("an unusable file is refused: '") + bad + "'");
            expect(victim.isComplete() && victim.getKeyForAction(InputAction::MOVE_UP) == upKey,
                   std::string("and the defaults survive: '") + bad + "'");
        }
        fs::remove_all(dir.parent_path());
        expect(!InputConfig::defaultConfigPath().empty(), "there is always a default config path");

        // Reset and Back.
        for (int i = 0; i < 8; ++i) settings.handleInput(InputAction::MOVE_DOWN, config);
        settings.handleInput(InputAction::CONFIRM, config);
        expect(config.getKeyForAction(InputAction::MOVE_UP) == upKey && settings.consumeChanged(),
               "Reset restores the defaults and reports a change");
        settings.handleInput(InputAction::MOVE_DOWN, config);
        expect(settings.handleInput(InputAction::CONFIRM, config) == SettingsMenu::Command::CLOSE, "Back closes");
        expect(settings.handleInput(InputAction::CANCEL, config) == SettingsMenu::Command::CLOSE, "Esc closes");
        settings.handleInput(InputAction::MOVE_DOWN, config);
        expect(settings.getIndex() == 0, "the row cursor wraps");
    }

    std::cout << "MenuTest passed" << std::endl;
    return 0;
}
