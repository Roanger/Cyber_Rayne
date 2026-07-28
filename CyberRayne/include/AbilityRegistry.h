#pragma once

#include <string>
#include <vector>

#include "Ability.h"
#include "Player.h"

// The catalogue of every ability in the game, and the rules for who learns
// what and when. Abilities are owned here for the process lifetime; everyone
// else holds non-owning pointers.
namespace AbilityRegistry {

// Look up a single ability by id. Returns nullptr if unknown.
const Ability* find(const std::string& id);

// Every ability a class learns, in unlock order.
const std::vector<const Ability*>& forClass(Player::CharacterClass charClass);

// The subset of a class's kit unlocked at or below the given level.
std::vector<const Ability*> unlockedFor(Player::CharacterClass charClass, int level);

// Abilities newly unlocked by reaching exactly this level.
std::vector<const Ability*> unlockedAtLevel(Player::CharacterClass charClass, int level);

// Enemy kits, keyed by the enemy's type name (Enemy::getName()).
const std::vector<const Ability*>& forEnemy(const std::string& enemyName);

}  // namespace AbilityRegistry
