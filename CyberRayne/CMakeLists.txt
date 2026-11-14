# CMakeLists.txt : CMake project for Cyber Rayne
#
# This file defines the project structure and build configuration.

cmake_minimum_required(VERSION 3.10)

project(CyberRayne)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Warnings on for all of our own targets. stb_image is silenced at its include site.
if(MSVC)
    add_compile_options(/W4)
    set_source_files_properties(src/systems/AudioBackendMiniaudio.cpp PROPERTIES COMPILE_OPTIONS "/w")
else()
    add_compile_options(-Wall -Wextra)
    set_source_files_properties(src/systems/AudioBackendMiniaudio.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()

if(WIN32)
    # Set Vulkan SDK path
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_SDK $ENV{VULKAN_SDK})
    else()
        set(VULKAN_SDK "C:/VulkanSDK/1.4.321.1")
    endif()

    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK}/Include")
    set(Vulkan_LIBRARIES "${VULKAN_SDK}/Lib/vulkan-1.lib")

    # Find glslangValidator
    find_program(GLSLANG_VALIDATOR glslangValidator HINTS "${VULKAN_SDK}/Bin")

    # If not found, try the default path constructed from var
    if(NOT GLSLANG_VALIDATOR)
        set(GLSLANG_VALIDATOR "${VULKAN_SDK}/Bin/glslangValidator.exe")
    endif()
else()
    find_package(Vulkan REQUIRED)
    find_package(glfw3 REQUIRED)
    find_program(GLSLANG_VALIDATOR glslangValidator)
endif()

# Add executable
add_executable(CyberRayne
    src/main.cpp
    src/core/Game.cpp
    src/core/GameState.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Item.cpp
    src/entities/Ability.cpp
    src/systems/AbilityRegistry.cpp
    src/entities/NPC.cpp
    src/entities/Combatant.cpp
    src/entities/Party.cpp
    src/entities/StatusEffect.cpp
    src/entities/DialogueNode.cpp
    src/entities/DialogueTree.cpp
    src/entities/Quest.cpp
    src/entities/QuestLog.cpp
    src/entities/Equipment.cpp
    src/entities/Inventory.cpp
    src/core/Map.cpp
    src/core/Tile.cpp
    src/core/World.cpp
    src/systems/CharacterSelectionSystem.cpp
    src/systems/BattleSystem.cpp
    src/systems/UIManager.cpp
    src/systems/DialogueSystem.cpp
    src/systems/SaveSystem.cpp
    src/systems/EncounterTable.cpp
    src/systems/ContentFactory.cpp
    src/systems/StoryDirector.cpp
    src/systems/AudioManager.cpp
    src/systems/AudioBackendMiniaudio.cpp
    src/systems/BattleSounds.cpp
    src/ui/StoryHud.cpp
    src/ui/PauseMenu.cpp
    src/ui/SettingsMenu.cpp
    src/ui/Theme.cpp
    src/ui/BattleFx.cpp
    src/ui/BattleScene.cpp
    src/ui/SpriteLibrary.cpp
    src/systems/InputConfig.cpp
    src/systems/InputState.cpp
    src/systems/InputPrompts.cpp
    src/systems/AudioSettings.cpp
    src/graphics/VulkanRenderer.cpp
    src/graphics/TileTextureGen.cpp
    src/ui/MenuSystem.cpp
    src/ui/BitmapFont.cpp
    src/ui/TextRenderer.cpp
    src/ui/UIWidget.cpp
    src/ui/UILabel.cpp
    src/ui/UIButton.cpp
    src/ui/UIPanel.cpp
    src/ui/UIProgressBar.cpp
    src/ui/UIRoot.cpp
)

# Add test executables
include(CTest)

set(GAMEPLAY_TEST_SUPPORT_SOURCES
    src/entities/Inventory.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/entities/Item.cpp
    src/entities/Ability.cpp
    src/systems/AbilityRegistry.cpp
    src/entities/NPC.cpp
    src/entities/Combatant.cpp
    src/entities/Party.cpp
    src/entities/StatusEffect.cpp
    src/entities/Equipment.cpp
    src/entities/DialogueNode.cpp
    src/entities/DialogueTree.cpp
    src/core/Map.cpp
    src/core/Tile.cpp
)

add_executable(CharacterSelectionTest
    src/tests/CharacterSelectionTest.cpp
    src/systems/CharacterSelectionSystem.cpp
    src/entities/EnemyTypes.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)
add_executable(BattleSystemTest
    src/tests/BattleSystemTest.cpp
    src/systems/BattleSystem.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)
add_executable(EnemyTypesTest
    src/tests/EnemyTypesTest.cpp
    src/entities/Enemy.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Combatant.cpp
    src/entities/StatusEffect.cpp
)

add_executable(BitmapFontTest
    src/tests/BitmapFontTest.cpp
    src/ui/BitmapFont.cpp
)

add_executable(DialogueSystemTest
    src/tests/DialogueSystemTest.cpp
    src/systems/DialogueSystem.cpp
    src/entities/DialogueNode.cpp
    src/entities/DialogueTree.cpp
)

add_executable(QuestTest
    src/tests/QuestTest.cpp
    src/entities/Quest.cpp
    src/entities/QuestLog.cpp
)

add_executable(ProgressionTest
    src/tests/ProgressionTest.cpp
    src/systems/BattleSystem.cpp
    src/entities/Equipment.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)

add_executable(SaveLoadTest
    src/tests/SaveLoadTest.cpp
    src/systems/SaveSystem.cpp
    src/entities/Quest.cpp
    src/entities/QuestLog.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)

add_executable(ContentTest
    src/tests/ContentTest.cpp
    src/core/World.cpp
    src/systems/EncounterTable.cpp
    src/systems/ContentFactory.cpp
    src/systems/StoryDirector.cpp
    src/systems/DialogueSystem.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Quest.cpp
    src/entities/QuestLog.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)

add_executable(Phase5Test
    src/tests/Phase5Test.cpp
    src/systems/AudioManager.cpp
    src/systems/AudioBackendNone.cpp
    src/systems/InputConfig.cpp
    src/systems/InputState.cpp
    src/systems/InputPrompts.cpp
    src/systems/AudioSettings.cpp
)

add_executable(AbilityTest
    src/tests/AbilityTest.cpp
    src/systems/BattleSystem.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)

add_executable(StatusEffectTest
    src/tests/StatusEffectTest.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)

add_executable(EncounterTest
    src/tests/EncounterTest.cpp
    src/core/World.cpp
    src/systems/EncounterTable.cpp
    src/systems/ContentFactory.cpp
    src/systems/StoryDirector.cpp
    src/systems/DialogueSystem.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Quest.cpp
    src/entities/QuestLog.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)

add_executable(StoryTest
    src/tests/StoryTest.cpp
    src/core/World.cpp
    src/systems/EncounterTable.cpp
    src/systems/ContentFactory.cpp
    src/systems/StoryDirector.cpp
    src/systems/DialogueSystem.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Quest.cpp
    src/entities/QuestLog.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)

add_executable(InventoryTest
    src/tests/InventoryTest.cpp
    src/systems/BattleSystem.cpp
    src/systems/ContentFactory.cpp
    src/systems/StoryDirector.cpp
    src/core/World.cpp
    src/systems/EncounterTable.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Quest.cpp
    src/entities/QuestLog.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)

add_executable(BalanceTest
    src/tests/BalanceTest.cpp
    src/systems/BattleSystem.cpp
    src/systems/ContentFactory.cpp
    src/systems/StoryDirector.cpp
    src/core/World.cpp
    src/systems/EncounterTable.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Quest.cpp
    src/entities/QuestLog.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
)

add_executable(BattlePresentationTest
    src/tests/BattlePresentationTest.cpp
    src/systems/BattleSystem.cpp
    src/ui/BattleFx.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
    src/systems/BattleSounds.cpp
)

add_executable(MenuTest
    src/tests/MenuTest.cpp
    src/ui/PauseMenu.cpp
    src/ui/SettingsMenu.cpp
    src/systems/InputConfig.cpp
    ${GAMEPLAY_TEST_SUPPORT_SOURCES}
    src/systems/InputPrompts.cpp
    src/systems/AudioSettings.cpp
)

foreach(test_target CharacterSelectionTest BattleSystemTest EnemyTypesTest BitmapFontTest DialogueSystemTest QuestTest ProgressionTest SaveLoadTest ContentTest Phase5Test EncounterTest StatusEffectTest AbilityTest StoryTest MenuTest InventoryTest BalanceTest BattlePresentationTest)
    target_include_directories(${test_target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )
    target_compile_definitions(${test_target} PRIVATE NO_VULKAN)
    add_test(NAME ${test_target} COMMAND ${test_target})
endforeach()

if(WIN32)
    add_executable(VulkanTest src/tests/VulkanTest.cpp)
    target_link_libraries(VulkanTest ${Vulkan_LIBRARIES})
    target_include_directories(VulkanTest PRIVATE
        ${Vulkan_INCLUDE_DIRS}
    )
endif()

# Include directories
if(WIN32)
    target_include_directories(CyberRayne PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )
else()
    target_include_directories(CyberRayne PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )
endif()

# Header files
set(HEADERS
    include/Game.h
    include/GameState.h
    include/Player.h
    include/Enemy.h
    include/BattleSystem.h
    include/World.h
    include/Map.h
    include/Tile.h
    include/TileTextureGen.h
    include/Ability.h
    include/AbilityRegistry.h
    include/Item.h
    include/CharacterSelectionSystem.h
    include/MenuSystem.h
    include/Combatant.h
    include/Party.h
    include/StatusEffect.h
    include/DialogueNode.h
    include/DialogueTree.h
    include/DialogueSystem.h
    include/Quest.h
    include/QuestLog.h
    include/Equipment.h
    include/SaveSystem.h
    include/EncounterTable.h
    include/ContentFactory.h
    include/AudioManager.h
    include/InputConfig.h
    include/ui/BitmapFont.h
    include/ui/BitmapFontData.h
    include/ui/TextRenderer.h
    include/ui/UIWidget.h
    include/ui/UILabel.h
    include/ui/UIButton.h
    include/ui/UIPanel.h
    include/ui/UIProgressBar.h
    include/ui/UIRoot.h
)

if(WIN32)
    # Link Vulkan
    target_link_libraries(CyberRayne ${Vulkan_LIBRARIES})
else()
    target_link_libraries(CyberRayne Vulkan::Vulkan glfw)
    # miniaudio loads the system's sound libraries itself at run time.
    find_package(Threads REQUIRED)
    target_link_libraries(CyberRayne Threads::Threads ${CMAKE_DL_LIBS} m)
endif()

# Third-party audio sources (miniaudio, stb_vorbis) are not written for our
# warning level; keep the project's own code at zero warnings without them.
if(MSVC)
    set_source_files_properties(src/systems/AudioBackendMiniaudio.cpp PROPERTIES COMPILE_OPTIONS "/W0")
else()
    set_source_files_properties(src/systems/AudioBackendMiniaudio.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()

# For the test, we don't need Vulkan
# target_link_libraries(CharacterSelectionTest ${Vulkan_LIBRARIES})

if(WIN32)
    if (CMAKE_VERSION VERSION_GREATER 3.12)
      set_property(TARGET CyberRayne PROPERTY CXX_STANDARD 20)
    endif()
endif()

 # Compile shaders to SPIR-V
 if(GLSLANG_VALIDATOR)
     # Vertex shader
     add_custom_command(
         OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vert.spv
         COMMAND ${GLSLANG_VALIDATOR} -V ${CMAKE_SOURCE_DIR}/shaders/vertex_shader.vert -o ${CMAKE_CURRENT_BINARY_DIR}/vert.spv
         DEPENDS ${CMAKE_SOURCE_DIR}/shaders/vertex_shader.vert
         COMMENT "Compiling vertex shader"
     )
     
     # Fragment shader
     add_custom_command(
         OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/frag.spv
         COMMAND ${GLSLANG_VALIDATOR} -V ${CMAKE_SOURCE_DIR}/shaders/fragment_shader.frag -o ${CMAKE_CURRENT_BINARY_DIR}/frag.spv
         DEPENDS ${CMAKE_SOURCE_DIR}/shaders/fragment_shader.frag
         COMMENT "Compiling fragment shader"
     )
     
     # Add custom target for shaders
     add_custom_target(shaders ALL
         DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vert.spv ${CMAKE_CURRENT_BINARY_DIR}/frag.spv
     )
     
     # Make sure shaders are built before the main executable
     add_dependencies(CyberRayne shaders)
     
     # Create shaders directory if it doesn't exist
     add_custom_command(TARGET CyberRayne PRE_BUILD
         COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:CyberRayne>/shaders
     )
     
     # Copy compiled shaders to build directory
     add_custom_command(TARGET CyberRayne POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_if_different
         ${CMAKE_CURRENT_BINARY_DIR}/vert.spv $<TARGET_FILE_DIR:CyberRayne>/shaders/vert.spv
         COMMAND ${CMAKE_COMMAND} -E copy_if_different
         ${CMAKE_CURRENT_BINARY_DIR}/frag.spv $<TARGET_FILE_DIR:CyberRayne>/shaders/frag.spv
     )
 else()
     message(WARNING "glslangValidator not found. Shaders will not be compiled.")
     
     # Copy original shaders to build directory
     file(GLOB SHADERS "shaders/*")
     add_custom_command(TARGET CyberRayne POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_directory
         ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:CyberRayne>/shaders)
 endif()

# Copy assets to build directory
add_custom_command(TARGET CyberRayne POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CyberRayne>/assets
)

# TODO: Add tests and install targets if needed.
