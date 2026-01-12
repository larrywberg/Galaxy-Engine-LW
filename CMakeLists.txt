cmake_minimum_required(VERSION 3.26..3.30 FATAL_ERROR)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
cmake_policy(SET CMP0077 NEW)

cmake_policy(SET CMP0135 NEW)

project(GalaxyEngine)

include(FetchContent)

if(NOT DEFINED EMSCRIPTEN)
    if((CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "emscripten") OR DEFINED ENV{EMSDK})
        set(EMSCRIPTEN ON)
    else()
        set(EMSCRIPTEN OFF)
    endif()
endif()

if(EMSCRIPTEN)
    # Prevent macOS defaults from injecting -arch flags that emcc does not understand.
    set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS "" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS "" CACHE STRING "" FORCE)
    set(CMAKE_MODULE_LINKER_FLAGS "" CACHE STRING "" FORCE)
    set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES> -o <TARGET>" CACHE STRING "" FORCE)
    set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES> -o <TARGET>" CACHE STRING "" FORCE)
endif()

if(NOT EMSCRIPTEN)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/ffmpeg.cmake)
endif()
include(${CMAKE_CURRENT_LIST_DIR}/cmake/glm.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/openmp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/raylib.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/imgui.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/yaml.cmake)

set(
    GALAXYENGINE_SOURCES
    main.cpp
    globalLogic.cpp
    parameters.cpp
    Particles/particleSelection.cpp
    Particles/particlesSpawning.cpp
    Particles/particleSubdivision.cpp
    Particles/particleTrails.cpp
    Physics/morton.cpp
    Physics/physics.cpp
    Physics/quadtree.cpp
    Physics/slingshot.cpp
    Physics/SPH.cpp
    Physics/light.cpp
    Sound/sound.cpp
    UI/brush.cpp
    UI/controls.cpp
    UI/rightClickSettings.cpp
    UI/UI.cpp
    UX/camera.cpp
    UX/saveSystem.cpp
    UX/randNum.cpp)

    if(EMSCRIPTEN)
        list(APPEND GALAXYENGINE_SOURCES UX/screenCapture_stub.cpp)
        list(APPEND GALAXYENGINE_SOURCES rlImGui_wasm.cpp)
    else()
        list(APPEND GALAXYENGINE_SOURCES UX/screenCapture.cpp)
    endif()

if(WIN32)
    list(APPEND GALAXYENGINE_SOURCES resources.rc)
endif()

list(TRANSFORM GALAXYENGINE_SOURCES PREPEND ${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/src/)

add_executable(GalaxyEngine ${GALAXYENGINE_SOURCES})

target_precompile_headers(GalaxyEngine PUBLIC ${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/include/pch.h)

target_include_directories(GalaxyEngine PUBLIC ${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/include)

target_compile_features(GalaxyEngine PRIVATE cxx_std_20)
set_target_properties(
        GalaxyEngine PROPERTIES
        MSVC_RUNTIME_LIBRARY MultiThreadedDLL
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine)

target_link_libraries(GalaxyEngine PUBLIC raylib-lib imgui glm-lib yaml-cpp)
if(NOT EMSCRIPTEN)
    target_link_libraries(GalaxyEngine PUBLIC ffmpeg openmp)
endif()

if(EMSCRIPTEN)
    target_compile_definitions(GalaxyEngine PRIVATE EMSCRIPTEN=1)
    target_link_options(GalaxyEngine PRIVATE "-sUSE_GLFW=3")
    target_link_options(GalaxyEngine PRIVATE "-sASYNCIFY")
    target_link_options(GalaxyEngine PRIVATE "-sSTACK_SIZE=4194304")
    target_link_options(GalaxyEngine PRIVATE "-sINITIAL_MEMORY=268435456")
    target_link_options(GalaxyEngine PRIVATE "-sALLOW_MEMORY_GROWTH=1")
    target_link_options(GalaxyEngine PRIVATE "--preload-file=${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/fonts@/fonts")
    target_link_options(GalaxyEngine PRIVATE "--preload-file=${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/Textures@/Textures")
    target_link_options(GalaxyEngine PRIVATE "--preload-file=${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/Shaders@/Shaders")
    target_link_options(GalaxyEngine PRIVATE "--preload-file=${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/Sounds@/Sounds")
endif()

if(WIN32)
    target_link_libraries(GalaxyEngine PUBLIC winmm)

    add_custom_command(
            TARGET GalaxyEngine
            POST_BUILD
            COMMAND
                ${CMAKE_COMMAND} -E copy_directory ${ffmpeg-fetch_SOURCE_DIR}/bin $<TARGET_FILE_DIR:GalaxyEngine>)
endif()
