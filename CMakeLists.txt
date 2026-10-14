cmake_minimum_required(VERSION 3.20)
project(MetroidZeroMissionRecomp C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GBARECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/reference/gbarecomp" CACHE PATH "Pinned framework checkout")
set(GBARECOMP_GENERATED_BIOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated/bios" CACHE PATH "Local BIOS translation")
set(GBARECOMP_TOMLPP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/framework/_deps/tomlplusplus-src" CACHE PATH "Pinned toml++ headers")

if(NOT RECOMP_UI_ROOT)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/reference/recomp-ui/recomp_ui.cmake")
        set(RECOMP_UI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/reference/recomp-ui" CACHE PATH "Pinned player launcher")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/recomp-ui/recomp_ui.cmake")
        set(RECOMP_UI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/recomp-ui" CACHE PATH "Pinned player launcher")
    endif()
endif()

if(NOT EXISTS "${GBARECOMP_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "Missing framework checkout; see docs/FRAMEWORK_PIN.json")
endif()

add_subdirectory("${GBARECOMP_ROOT}" framework EXCLUDE_FROM_ALL)

if(RECOMP_UI_ROOT AND EXISTS "${RECOMP_UI_ROOT}/recomp_ui.cmake")
    include("${RECOMP_UI_ROOT}/recomp_ui.cmake")
    add_library(game_launcher_ui STATIC src/game_launcher_boot.cpp)
    target_include_directories(game_launcher_ui PRIVATE
        "${GBARECOMP_ROOT}/src/runtime"
        "${GBARECOMP_ROOT}/src/gba"
        ${SDL2_INCLUDE_DIR}
    )
    recomp_target_launcher_ui(game_launcher_ui CONSOLE gba)
    target_link_libraries(game_launcher_ui PUBLIC ${SDL2_LIBRARY})

    add_executable(launcher_contract EXCLUDE_FROM_ALL tests/launcher_contract.cpp)
    target_compile_definitions(launcher_contract PRIVATE RECOMP_LAUNCHER)
    target_include_directories(launcher_contract PRIVATE
        "${GBARECOMP_ROOT}/src/runtime"
        "${GBARECOMP_ROOT}/src/gba"
        "${RECOMP_UI_ROOT}/src"
        "${RECOMP_UI_ROOT}/src/common"
        ${SDL2_INCLUDE_DIR}
    )
    target_link_libraries(launcher_contract PRIVATE game_launcher_ui gbarecomp_gba)
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/cart/dispatch_table.cpp" OR
   NOT EXISTS "${GBARECOMP_GENERATED_BIOS_DIR}/bios_recompiled.cpp")
    message(FATAL_ERROR "Generate cartridge and BIOS source with tools/regen.ps1 first")
endif()

file(GLOB CART_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/generated/cart/*.cpp")
add_executable(stack_variant_generator EXCLUDE_FROM_ALL tools/generate_stack_variants.cpp)
target_include_directories(stack_variant_generator PRIVATE src)
target_link_libraries(stack_variant_generator PRIVATE gbarecomp_recompile_core gbarecomp_gba)

file(GLOB STACK_ROM "${CMAKE_CURRENT_SOURCE_DIR}/*.gba")
list(LENGTH STACK_ROM STACK_ROM_COUNT)
if(STACK_ROM_COUNT EQUAL 1)
    set(STACK_GENERATED "${CMAKE_CURRENT_SOURCE_DIR}/generated/stack_variants.cpp")
    add_custom_command(OUTPUT "${STACK_GENERATED}"
        COMMAND $<TARGET_FILE:stack_variant_generator> "${STACK_ROM}" "${STACK_GENERATED}"
        DEPENDS stack_variant_generator "${STACK_ROM}" src/copied_code.h
        VERBATIM)
endif()
if(NOT STACK_ROM_COUNT EQUAL 1)
    message(FATAL_ERROR "Native RAM generation requires exactly one verified local USA ROM")
endif()

add_executable(MetroidZeroMissionRecomp src/main.cpp src/copied_code.cpp ${CART_SOURCES} "${STACK_GENERATED}")
target_include_directories(MetroidZeroMissionRecomp PRIVATE src)
target_link_gbarecomp_runtime_stack(MetroidZeroMissionRecomp)
if(TARGET game_launcher_ui)
    target_link_libraries(MetroidZeroMissionRecomp PRIVATE game_launcher_ui)
    recomp_stage_launcher_assets(MetroidZeroMissionRecomp CONSOLE gba)
endif()

if(MINGW)
    target_link_options(MetroidZeroMissionRecomp PRIVATE "-Wl,--stack,67108864")
endif()

add_executable(copied_code_guard_tests tests/copied_code_guard.cpp)
target_include_directories(copied_code_guard_tests PRIVATE src)
enable_testing()
add_test(NAME copied_code_guard_tests COMMAND copied_code_guard_tests)

add_executable(stack_tail_dispatch_tests tests/stack_tail_dispatch.cpp)
target_include_directories(stack_tail_dispatch_tests PRIVATE src)
add_test(NAME stack_tail_dispatch_tests COMMAND stack_tail_dispatch_tests)

add_executable(frame_yield_scope_tests tests/frame_yield_scope.cpp
    "${GBARECOMP_ROOT}/tools/bios_smoke/dispatch_stub.cpp")
target_link_gbarecomp_runtime_stack(frame_yield_scope_tests)
add_test(NAME frame_yield_scope_tests COMMAND frame_yield_scope_tests)

set(SHARDS ${CART_SOURCES})
list(FILTER SHARDS INCLUDE REGEX "/recompiled_[0-9]+\\.cpp$")
set_source_files_properties(${SHARDS} PROPERTIES COMPILE_OPTIONS "-O1;-g0")
gbarecomp_stage_mingw_runtime(MetroidZeroMissionRecomp)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/bin/SDL2.dll")
    add_custom_command(TARGET MetroidZeroMissionRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/bin/SDL2.dll"
        "$<TARGET_FILE_DIR:MetroidZeroMissionRecomp>")
endif()

add_custom_command(TARGET MetroidZeroMissionRecomp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/game.toml"
    "$<TARGET_FILE_DIR:MetroidZeroMissionRecomp>/game.toml"
    VERBATIM)
