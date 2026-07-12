# CMakeLists.txt : CMake project for Cyber Rayne
#
# This file defines the project structure and build configuration.

cmake_minimum_required(VERSION 3.10)

project(CyberRayne)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    src/entities/Spell.cpp
    src/entities/NPC.cpp
    src/entities/Combatant.cpp
    src/entities/Party.cpp
    src/entities/StatusEffect.cpp
    src/core/Map.cpp
    src/core/Tile.cpp
    src/core/World.cpp
    src/systems/CharacterSelectionSystem.cpp
    src/systems/BattleSystem.cpp
    src/systems/UIManager.cpp
    src/graphics/VulkanRenderer.cpp
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
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/entities/Item.cpp
    src/entities/Spell.cpp
    src/entities/NPC.cpp
    src/entities/Combatant.cpp
    src/entities/Party.cpp
    src/entities/StatusEffect.cpp
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

foreach(test_target CharacterSelectionTest BattleSystemTest EnemyTypesTest BitmapFontTest)
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
    include/Spell.h
    include/Item.h
    include/CharacterSelectionSystem.h
    include/MenuSystem.h
    include/Combatant.h
    include/Party.h
    include/StatusEffect.h
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
