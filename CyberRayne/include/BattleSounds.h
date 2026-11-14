#pragma once

#include "BattleSystem.h"

#include <string>
#include <vector>

// Which sound effects a batch of battle events calls for, by name (see
// AudioManager::playSfx). One batch is everything that happened in one step
// of the battle. Gameplay logic only.
//
// Attacks are grouped by kind rather than one sound per ability: a gunshot
// covers all the Solo's gun moves. New abilities need a line in the table in
// BattleSounds.cpp, or they are silent.
std::vector<std::string> battleSoundsFor(const std::vector<BattleSystem::BattleEvent>& events);

// The sound of a combatant's plain attack.
std::string attackSoundFor(const Combatant* who);
