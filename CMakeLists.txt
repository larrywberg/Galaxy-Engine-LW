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
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread" CACHE STRING "" FORCE)
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
        list(APPEND GALAXYENGINE_SOURCES web_api.cpp)
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
    target_compile_options(GalaxyEngine PRIVATE "-sUSE_PTHREADS=1")
    target_link_options(GalaxyEngine PRIVATE "-sUSE_GLFW=3")
    target_link_options(GalaxyEngine PRIVATE "-sASYNCIFY")
    target_link_options(GalaxyEngine PRIVATE "-sSTACK_SIZE=4194304")
    target_link_options(GalaxyEngine PRIVATE "-sINITIAL_MEMORY=268435456")
    target_link_options(GalaxyEngine PRIVATE "-sALLOW_MEMORY_GROWTH=1")
    target_link_options(GalaxyEngine PRIVATE "-sUSE_PTHREADS=1")
    target_link_options(GalaxyEngine PRIVATE "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency")
    target_link_options(GalaxyEngine PRIVATE "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap")
    target_link_options(GalaxyEngine PRIVATE "-sEXPORTED_FUNCTIONS=_main,_web_set_time_playing,_web_get_time_playing,_web_set_time_factor,_web_get_time_factor,_web_set_target_fps,_web_get_target_fps,_web_set_gravity_multiplier,_web_get_gravity_multiplier,_web_set_time_step_multiplier,_web_get_time_step_multiplier,_web_set_window_size,_web_set_pause_after_recording,_web_get_pause_after_recording,_web_set_clean_scene_after_recording,_web_get_clean_scene_after_recording,_web_set_recording_time_limit,_web_get_recording_time_limit,_web_clear_scene,_web_set_glow_enabled,_web_get_glow_enabled,_web_set_trails_length,_web_get_trails_length,_web_set_trails_thickness,_web_get_trails_thickness,_web_set_global_trails,_web_get_global_trails,_web_set_selected_trails,_web_get_selected_trails,_web_set_local_trails,_web_get_local_trails,_web_set_white_trails,_web_get_white_trails,_web_set_color_mode,_web_get_color_mode,_web_set_ui_hover,_web_set_tool_draw_particles,_web_get_tool_draw_particles,_web_set_tool_black_hole,_web_get_tool_black_hole,_web_set_tool_big_galaxy,_web_get_tool_big_galaxy,_web_set_tool_small_galaxy,_web_get_tool_small_galaxy,_web_set_tool_star,_web_get_tool_star,_web_set_tool_big_bang,_web_get_tool_big_bang,_web_set_tool_point_light,_web_get_tool_point_light,_web_set_tool_area_light,_web_get_tool_area_light,_web_set_tool_cone_light,_web_get_tool_cone_light,_web_set_tool_wall,_web_get_tool_wall,_web_set_tool_circle,_web_get_tool_circle,_web_set_tool_draw_shape,_web_get_tool_draw_shape,_web_set_tool_lens,_web_get_tool_lens,_web_set_tool_move_optics,_web_get_tool_move_optics,_web_set_tool_erase_optics,_web_get_tool_erase_optics,_web_set_tool_select_optics,_web_get_tool_select_optics,_web_set_tool_eraser,_web_get_tool_eraser,_web_set_tool_radial_force,_web_get_tool_radial_force,_web_set_tool_spin,_web_get_tool_spin,_web_set_tool_grab,_web_get_tool_grab,_web_rc_subdivide_all,_web_rc_subdivide_selected,_web_rc_delete_selection,_web_rc_delete_stray,_web_rc_deselect_all,_web_rc_invert_selection,_web_rc_select_clusters,_web_rc_center_camera,_web_rc_pin_selected,_web_rc_unpin_selected,_web_set_draw_quadtree,_web_get_draw_quadtree,_web_set_draw_z_curves,_web_get_draw_z_curves")
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
