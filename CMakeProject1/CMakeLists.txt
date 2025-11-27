# CMakeLists.txt : CMake project for FF9-style JRPG
#
# This file defines the project structure and build configuration.

cmake_minimum_required(VERSION 3.10)

project(FF9StyleJRPG)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Add executable
add_executable(FF9StyleJRPG
    src/main.cpp
    src/core/Game.cpp
    src/core/GameState.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Item.cpp
    src/entities/Spell.cpp
    src/entities/NPC.cpp
    src/core/Map.cpp
    src/core/Tile.cpp
    src/core/World.cpp
    src/systems/CharacterSelectionSystem.cpp
    src/systems/BattleSystem.cpp
    src/systems/UIManager.cpp
    src/graphics/VulkanRenderer.cpp
    src/ui/MenuSystem.cpp
)

# Add test executable
set(TEST_SOURCES
    src/tests/CharacterSelectionTest.cpp
    src/systems/CharacterSelectionSystem.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Item.cpp
    src/entities/Spell.cpp
)

# Add battle system test executable
set(BATTLE_TEST_SOURCES
    src/tests/BattleSystemTest.cpp
    src/systems/BattleSystem.cpp
    src/systems/UIManager.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/entities/EnemyTypes.cpp
    src/entities/Item.cpp
    src/entities/Spell.cpp
    src/graphics/VulkanRenderer.cpp
)

# Add enemy types test executable
set(ENEMY_TEST_SOURCES
    src/tests/EnemyTypesTest.cpp
    src/entities/Enemy.cpp
    src/entities/EnemyTypes.cpp
)

add_executable(CharacterSelectionTest ${TEST_SOURCES})
add_executable(BattleSystemTest ${BATTLE_TEST_SOURCES})
add_executable(EnemyTypesTest ${ENEMY_TEST_SOURCES})
add_executable(VulkanTest src/tests/VulkanTest.cpp)
target_link_libraries(VulkanTest ${Vulkan_LIBRARIES})
target_include_directories(VulkanTest PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)

target_include_directories(CharacterSelectionTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Exclude Vulkan dependencies for the test
target_compile_definitions(CharacterSelectionTest PRIVATE -DNO_VULKAN)

# Battle system test
target_include_directories(BattleSystemTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${Vulkan_INCLUDE_DIRS}
)

# Exclude Vulkan dependencies for the battle system test
# Exclude Vulkan dependencies for the battle system test
# target_compile_definitions(BattleSystemTest PRIVATE -DNO_VULKAN)
target_link_libraries(BattleSystemTest ${Vulkan_LIBRARIES})

# Include directories
target_include_directories(FF9StyleJRPG PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

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
    include/Spell.h
    include/Item.h
    include/CharacterSelectionSystem.h
    include/MenuSystem.h
)

# Link Vulkan
target_link_libraries(FF9StyleJRPG ${Vulkan_LIBRARIES})

# For the test, we don't need Vulkan
# target_link_libraries(CharacterSelectionTest ${Vulkan_LIBRARIES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET FF9StyleJRPG PROPERTY CXX_STANDARD 20)
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
    add_dependencies(FF9StyleJRPG shaders)
    
    # Create shaders directory if it doesn't exist
    add_custom_command(TARGET FF9StyleJRPG PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:FF9StyleJRPG>/shaders
    )
    
    # Copy compiled shaders to build directory
    add_custom_command(TARGET FF9StyleJRPG POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/vert.spv $<TARGET_FILE_DIR:FF9StyleJRPG>/shaders/vert.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/frag.spv $<TARGET_FILE_DIR:FF9StyleJRPG>/shaders/frag.spv
    )
else()
    message(WARNING "glslangValidator not found. Shaders will not be compiled.")
    
    # Copy original shaders to build directory
    file(GLOB SHADERS "shaders/*")
    add_custom_command(TARGET FF9StyleJRPG POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:FF9StyleJRPG>/shaders)
endif()

# Copy assets to build directory
add_custom_command(TARGET FF9StyleJRPG POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:FF9StyleJRPG>/assets
)

# TODO: Add tests and install targets if needed.
