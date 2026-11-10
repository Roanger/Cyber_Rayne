#pragma once

#include <string>

class InputConfig;

// What to call each control on screen, for whatever the player is using right
// now: the keys actually bound on a keyboard, or the button names of their
// controller. Hints ask here instead of hard-coding "Enter".
//
// The game loop reports which device was touched last; everything that draws
// a hint reads the names. Gameplay logic only — no platform dependencies.
namespace InputPrompts {

enum class Device { KEYBOARD, PLAYSTATION, XBOX };

void setDevice(Device device);
Device device();

// Keyboard names come from these bindings, so a rebound key shows correctly.
// Not owned; pass nullptr to fall back to the default key names.
void setBindings(const InputConfig* bindings);

// Which button names a controller uses, judged from the name it reports.
// Anything that is not a PlayStation pad is shown Xbox-style, which is also
// how Steam Input presents controllers.
Device deviceForGamepadName(const std::string& name);

std::string confirm();    // "Enter" / "Cross" / "A"
std::string cancel();     // "Esc" / "Circle" / "B"
std::string menu();       // "M" / "Options" / "Start"
std::string upDown();     // "Up/Down" / "D-Pad"
std::string leftRight();  // "Left/Right" / "D-Pad"

}  // namespace InputPrompts
