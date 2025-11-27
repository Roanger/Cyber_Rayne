# Cyber_Rayne

A 2D Cyberpunk JRPG built from scratch using C++ and Vulkan.

## 🎮 Overview

Cyber_Rayne is a role-playing game set in a futuristic cyber world. It features a custom game engine built with modern C++ and utilizes Vulkan for high-performance 2D rendering.

## ✨ Features

- **Custom Engine**: Built on top of Vulkan for robust graphics performance.
- **Turn-Based Combat**: Strategic battle system with spells, items, and enemy AI.
- **Character System**: Class-based character selection (Warrior, Mage, Rogue) with unique stats.
- **Entity Management**: Comprehensive system for NPCs, Enemies, and interactive items.
- **UI System**: Custom UI manager for menus, battle interfaces, and dialogue.

## 🛠️ Tech Stack

- **Language**: C++17/20
- **Graphics API**: Vulkan
- **Build System**: CMake
- **IDE**: Visual Studio 2022

## 🚀 Getting Started

### Prerequisites
- Visual Studio 2022 (with C++ Desktop Development workload)
- Vulkan SDK
- CMake

### Building
1. Clone the repository:
   `git clone https://github.com/Roanger/Cyber_Rayne.git`
2. Open the folder in Visual Studio.
3. Let CMake configure the project.
4. Build and Run.

## 📂 Project Structure

- `src/core`: Core game loop and state management.
- `src/graphics`: Vulkan renderer implementation.
- `src/entities`: Game objects (Player, Enemy, etc.).
- `src/systems`: Game logic systems (Battle, UI).
- `assets`: Game assets (textures, data files).
