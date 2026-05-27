cmake_minimum_required(VERSION 3.20)

project(dice LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/third-party" CACHE PATH "" FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ----------------------------
# SFML
# ----------------------------
set(SFML_BUILD_GRAPHICS      ON  CACHE BOOL "" FORCE)
set(SFML_BUILD_WINDOW        ON  CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK       ON  CACHE BOOL "" FORCE)
set(SFML_BUILD_SYSTEM        ON  CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO         OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES      OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC           OFF CACHE BOOL "" FORCE)
set(SFML_USE_STATIC_STD_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG        2.6.1
)

# ----------------------------
# Dear ImGui
# ----------------------------
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.90.4
)

# ----------------------------
# nlohmann/json
# ----------------------------
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)

# ----------------------------
# sol2
# ----------------------------
FetchContent_Declare(
  sol2
  GIT_REPOSITORY https://github.com/ThePhD/sol2.git
  GIT_TAG        v3.3.0
)

# ----------------------------
# spdlog
# ----------------------------
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH   OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.14.1
)

# ----------------------------
# GoogleTest
# ----------------------------
set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(SFML imgui nlohmann_json sol2 spdlog googletest)

# ----------------------------
# Dear ImGui target
# ----------------------------
add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR})
target_compile_features(imgui_lib PUBLIC cxx_std_20)

# ----------------------------
# Core library (for tests & app)
# ----------------------------
add_library(dice_core STATIC
  src/core/GameObject.cpp
  src/components/Chip.cpp
  src/components/Card.cpp
  src/ui/View.cpp
  src/core/Model.cpp
  src/core/Action.cpp
  src/core/ActionManager.cpp
  src/core/SceneValidator.cpp
)

target_include_directories(dice_core
  PUBLIC
    ${CMAKE_SOURCE_DIR}       
    ${CMAKE_SOURCE_DIR}/include 
)

target_link_libraries(dice_core
  PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)


find_package(Lua 5.3 REQUIRED)

# ----------------------------
# Scripting library
# ----------------------------
add_library(dice_scripting STATIC
  src/scripting/LuaScriptEngine.cpp
  src/scripting/LuaScript.cpp
)

target_include_directories(dice_scripting
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(dice_scripting
  PUBLIC
    dice_core
    ${LUA_LIBRARY}
    spdlog::spdlog
)

if(TARGET sol2::sol2)
  target_link_libraries(dice_scripting PUBLIC sol2::sol2)
else()
  target_include_directories(dice_scripting PUBLIC ${sol2_SOURCE_DIR}/include)
endif()

# ----------------------------
# Controller library
# ----------------------------
add_library(dice_controller STATIC
  src/controller/Controller.cpp
  src/app/Application.cpp
  src/app/ConfigLoader.cpp
)

target_include_directories(dice_controller
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dice_controller
  PUBLIC
    dice_scripting
)

# ----------------------------
# Application
# ----------------------------
add_executable(${PROJECT_NAME}
  src/main.cpp
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    dice_controller
    sfml-network
    imgui_lib
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

set(RESOURCE_DIRS assets)

foreach(dir IN LISTS RESOURCE_DIRS)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/${dir}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/${dir}
  )
endforeach()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/samples/scripts
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/scripts
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/samples/scenes
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/scenes
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_SOURCE_DIR}/samples/game.json
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/game.json
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_SOURCE_DIR}/samples/assets/presets.json
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/presets.json
)

# ----------------------------
# Tests
# ----------------------------
enable_testing()

add_executable(dice_tests
  tests/core/test_GameObject.cpp
  tests/components/test_Chip.cpp
  tests/ui/test_View.cpp
  tests/core/test_Model.cpp
  tests/core/test_ResourceManager.cpp
  tests/core/test_Action.cpp
  tests/core/test_ActionManager.cpp
  tests/core/test_SceneValidator.cpp
)

target_link_libraries(dice_tests
  PRIVATE
    dice_core
    GTest::gtest_main
)

add_executable(dice_scripting_tests
  tests/scripting/test_LuaScriptEngine.cpp
  tests/scripting/test_BehaviorPresets.cpp
)

target_link_libraries(dice_scripting_tests
  PRIVATE
    dice_scripting
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(dice_tests)
gtest_discover_tests(dice_scripting_tests)

add_custom_command(TARGET dice_scripting_tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/tests/fixtures
  $<TARGET_FILE_DIR:dice_scripting_tests>/tests/fixtures
)

option(ENABLE_SANITIZERS "Enable sanitizers" OFF)

if (ENABLE_SANITIZERS AND UNIX AND NOT APPLE)
  message(STATUS "Sanitizers enabled")

  set(SAN_FLAGS -fsanitize=address,undefined)

  add_compile_options(
    ${SAN_FLAGS}
    -fno-omit-frame-pointer
    -g
  )

  add_link_options(${SAN_FLAGS})
endif()

# ----------------------------
# Application compile options
# ----------------------------

#if (MSVC)
#  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /D_CRT_SECURE_NO_WARNINGS)
#
#elseif(UNIX)
#  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
#
#  if(CMAKE_BUILD_TYPE STREQUAL "Asan")
#    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined
#            -fno-sanitize-recover=all -fsanitize=address)
#    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined
#            -fno-sanitize-recover=all -fsanitize=address)
#
#  elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
#    target_compile_options(${PROJECT_NAME} PRIVATE -g)
#  endif()
#endif()
