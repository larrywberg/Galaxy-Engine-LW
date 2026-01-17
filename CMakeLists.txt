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
    UI/brush.cpp
    UI/controls.cpp
    UI/rightClickSettings.cpp
    UI/UI.cpp
    UX/camera.cpp
    UX/saveSystem.cpp
    UX/randNum.cpp)

if(NOT DEFINED GALAXYENGINE_ENABLE_SOUND)
    if(EMSCRIPTEN)
        set(GALAXYENGINE_ENABLE_SOUND OFF)
    else()
        set(GALAXYENGINE_ENABLE_SOUND ON)
    endif()
endif()

if(GALAXYENGINE_ENABLE_SOUND)
    list(APPEND GALAXYENGINE_SOURCES Sound/sound.cpp)
else()
    list(APPEND GALAXYENGINE_SOURCES Sound/sound_stub.cpp)
endif()

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
target_compile_definitions(GalaxyEngine PRIVATE GE_ENABLE_SOUND=$<BOOL:${GALAXYENGINE_ENABLE_SOUND}>)

target_compile_features(GalaxyEngine PRIVATE cxx_std_20)
set_target_properties(
        GalaxyEngine PROPERTIES
        MSVC_RUNTIME_LIBRARY MultiThreadedDLL
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine)

target_link_libraries(GalaxyEngine PUBLIC raylib-lib imgui glm-lib yaml-cpp)
if(NOT EMSCRIPTEN)
    target_link_libraries(GalaxyEngine PUBLIC ffmpeg openmp)
endif()

if(NOT EMSCRIPTEN)
    # Release-only size/perf opts: function/data sections + dead-strip + LTO.
    target_compile_options(GalaxyEngine PRIVATE
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:AppleClang,Clang,GNU>>:-ffunction-sections>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:AppleClang,Clang,GNU>>:-fdata-sections>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:AppleClang,Clang,GNU>>:-flto>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/GL>
    )
    target_link_options(GalaxyEngine PRIVATE
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:AppleClang,Clang>>:-Wl,-dead_strip>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU>>:-Wl,--gc-sections>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:AppleClang,Clang,GNU>>:-flto>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/LTCG>
    )
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
    target_link_options(GalaxyEngine PRIVATE "-sPTHREAD_POOL_SIZE=4")
    target_link_options(GalaxyEngine PRIVATE "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap")
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        target_compile_options(GalaxyEngine PRIVATE "-Os" "-g0")
        target_link_options(GalaxyEngine PRIVATE "-Os" "-g0")
        target_link_options(GalaxyEngine PRIVATE "-sASSERTIONS=0" "-sSAFE_HEAP=0" "-sDEMANGLE_SUPPORT=0")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(GalaxyEngine PRIVATE "-g0")
        target_link_options(GalaxyEngine PRIVATE "-g0")
        target_link_options(GalaxyEngine PRIVATE "-sASSERTIONS=0" "-sSAFE_HEAP=0" "-sDEMANGLE_SUPPORT=0")
    endif()
    target_link_options(GalaxyEngine PRIVATE "-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_web_save_scene_to_buffer,_web_get_scene_buffer_ptr,_web_load_scene_from_buffer,_web_build_scene_json,_web_get_scene_json_ptr,_web_load_scene_json,_web_set_time_playing,_web_get_time_playing,_web_set_time_factor,_web_get_time_factor,_web_set_target_fps,_web_get_target_fps,_web_set_gravity_multiplier,_web_get_gravity_multiplier,_web_set_gravity_ramp_enabled,_web_get_gravity_ramp_enabled,_web_set_gravity_ramp_start_mult,_web_get_gravity_ramp_start_mult,_web_set_gravity_ramp_seconds,_web_get_gravity_ramp_seconds,_web_set_gravity_ramp_time,_web_get_gravity_ramp_time,_web_set_velocity_damping_enabled,_web_get_velocity_damping_enabled,_web_set_velocity_damping_rate,_web_get_velocity_damping_rate,_web_set_time_step_multiplier,_web_get_time_step_multiplier,_web_set_symplectic_integrator,_web_get_symplectic_integrator,_web_set_window_size,_web_set_pause_after_recording,_web_get_pause_after_recording,_web_set_clean_scene_after_recording,_web_get_clean_scene_after_recording,_web_set_recording_time_limit,_web_get_recording_time_limit,_web_set_show_brush_cursor,_web_get_show_brush_cursor,_web_clear_scene,_web_save_scene,_web_load_scene,_web_set_glow_enabled,_web_get_glow_enabled,_web_set_trails_length,_web_get_trails_length,_web_set_trails_thickness,_web_get_trails_thickness,_web_set_global_trails,_web_get_global_trails,_web_set_selected_trails,_web_get_selected_trails,_web_set_local_trails,_web_get_local_trails,_web_set_white_trails,_web_get_white_trails,_web_set_color_mode,_web_get_color_mode,_web_set_particle_size_multiplier,_web_get_particle_size_multiplier,_web_get_fps,_web_get_frame_time,_web_get_particle_count,_web_get_selected_particle_count,_web_get_total_lights,_web_get_accumulated_rays,_web_set_dark_matter_enabled,_web_get_dark_matter_enabled,_web_set_looping_space_enabled,_web_get_looping_space_enabled,_web_set_fluid_ground_enabled,_web_get_fluid_ground_enabled,_web_set_temperature_enabled,_web_get_temperature_enabled,_web_set_highlight_selected,_web_get_highlight_selected,_web_set_constraints_enabled,_web_get_constraints_enabled,_web_set_unbreakable_constraints,_web_get_unbreakable_constraints,_web_set_constraint_after_drawing,_web_get_constraint_after_drawing,_web_set_draw_constraints,_web_get_draw_constraints,_web_set_visualize_mesh,_web_get_visualize_mesh,_web_set_constraint_stress_color,_web_get_constraint_stress_color,_web_set_gravity_field_enabled,_web_get_gravity_field_enabled,_web_set_gravity_field_dm_particles,_web_get_gravity_field_dm_particles,_web_set_field_res,_web_get_field_res,_web_set_gravity_display_threshold,_web_get_gravity_display_threshold,_web_set_gravity_display_softness,_web_get_gravity_display_softness,_web_set_gravity_display_stretch,_web_get_gravity_display_stretch,_web_set_gravity_custom_colors,_web_get_gravity_custom_colors,_web_set_gravity_exposure,_web_get_gravity_exposure,_web_set_theta,_web_get_theta,_web_set_softening,_web_get_softening,_web_set_optics_enabled,_web_get_optics_enabled,_web_set_light_gain,_web_get_light_gain,_web_set_light_spread,_web_get_light_spread,_web_set_wall_specular_roughness,_web_get_wall_specular_roughness,_web_set_wall_refraction_roughness,_web_get_wall_refraction_roughness,_web_set_wall_refraction_amount,_web_get_wall_refraction_amount,_web_set_wall_ior,_web_get_wall_ior,_web_set_wall_dispersion,_web_get_wall_dispersion,_web_set_wall_emission_gain,_web_get_wall_emission_gain,_web_set_shape_relax_iter,_web_get_shape_relax_iter,_web_set_shape_relax_factor,_web_get_shape_relax_factor,_web_set_max_samples,_web_get_max_samples,_web_set_sample_rays_amount,_web_get_sample_rays_amount,_web_set_max_bounces,_web_get_max_bounces,_web_set_diffuse_enabled,_web_get_diffuse_enabled,_web_set_specular_enabled,_web_get_specular_enabled,_web_set_refraction_enabled,_web_get_refraction_enabled,_web_set_dispersion_enabled,_web_get_dispersion_enabled,_web_set_emission_enabled,_web_get_emission_enabled,_web_set_symmetrical_lens,_web_get_symmetrical_lens,_web_set_draw_normals,_web_get_draw_normals,_web_set_relax_move,_web_get_relax_move,_web_set_light_color,_web_get_light_color,_web_set_wall_base_color,_web_get_wall_base_color,_web_set_wall_specular_color,_web_get_wall_specular_color,_web_set_wall_refraction_color,_web_get_wall_refraction_color,_web_set_wall_emission_color,_web_get_wall_emission_color,_web_get_lighting_samples,_web_reset_lighting_samples,_web_set_threads_amount,_web_get_threads_amount,_web_set_domain_width,_web_get_domain_width,_web_set_domain_height,_web_get_domain_height,_web_set_black_hole_init_mass,_web_get_black_hole_init_mass,_web_set_path_prediction_enabled,_web_get_path_prediction_enabled,_web_set_path_prediction_length,_web_get_path_prediction_length,_web_set_particle_amount_multiplier,_web_get_particle_amount_multiplier,_web_set_dark_matter_amount_multiplier,_web_get_dark_matter_amount_multiplier,_web_set_mass_multiplier_enabled,_web_get_mass_multiplier_enabled,_web_set_ambient_temperature,_web_get_ambient_temperature,_web_set_ambient_heat_rate,_web_get_ambient_heat_rate,_web_set_heat_conductivity,_web_get_heat_conductivity,_web_set_constraint_stiffness,_web_get_constraint_stiffness,_web_set_constraint_resistance,_web_get_constraint_resistance,_web_set_fluid_vertical_gravity,_web_get_fluid_vertical_gravity,_web_set_fluid_mass_multiplier,_web_get_fluid_mass_multiplier,_web_set_fluid_viscosity,_web_get_fluid_viscosity,_web_set_fluid_stiffness,_web_get_fluid_stiffness,_web_set_fluid_cohesion,_web_get_fluid_cohesion,_web_set_fluid_delta,_web_get_fluid_delta,_web_set_fluid_max_velocity,_web_get_fluid_max_velocity,_web_set_ui_hover,_web_set_tool_draw_particles,_web_get_tool_draw_particles,_web_set_tool_black_hole,_web_get_tool_black_hole,_web_set_tool_big_galaxy,_web_get_tool_big_galaxy,_web_set_tool_small_galaxy,_web_get_tool_small_galaxy,_web_set_tool_star,_web_get_tool_star,_web_set_tool_big_bang,_web_get_tool_big_bang,_web_set_tool_point_light,_web_get_tool_point_light,_web_set_tool_area_light,_web_get_tool_area_light,_web_set_tool_cone_light,_web_get_tool_cone_light,_web_set_tool_wall,_web_get_tool_wall,_web_set_tool_circle,_web_get_tool_circle,_web_set_tool_draw_shape,_web_get_tool_draw_shape,_web_set_tool_lens,_web_get_tool_lens,_web_set_tool_move_optics,_web_get_tool_move_optics,_web_set_tool_erase_optics,_web_get_tool_erase_optics,_web_set_tool_select_optics,_web_get_tool_select_optics,_web_set_tool_eraser,_web_get_tool_eraser,_web_set_tool_radial_force,_web_get_tool_radial_force,_web_set_tool_spin,_web_get_tool_spin,_web_set_tool_grab,_web_get_tool_grab,_web_rc_subdivide_all,_web_rc_subdivide_selected,_web_rc_delete_selection,_web_rc_delete_stray,_web_rc_deselect_all,_web_rc_invert_selection,_web_rc_select_clusters,_web_rc_center_camera,_web_rc_pin_selected,_web_rc_unpin_selected,_web_set_draw_quadtree,_web_get_draw_quadtree,_web_set_draw_z_curves,_web_get_draw_z_curves")
    target_link_options(GalaxyEngine PRIVATE "--preload-file=${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/fonts@/fonts")
    target_link_options(GalaxyEngine PRIVATE "--preload-file=${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/Textures@/Textures")
    target_link_options(GalaxyEngine PRIVATE "--preload-file=${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/Shaders@/Shaders")
    if(GALAXYENGINE_ENABLE_SOUND)
        target_link_options(GalaxyEngine PRIVATE "--preload-file=${CMAKE_CURRENT_LIST_DIR}/GalaxyEngine/Sounds@/Sounds")
    endif()
endif()

if(WIN32)
    target_link_libraries(GalaxyEngine PUBLIC winmm)

    add_custom_command(
            TARGET GalaxyEngine
            POST_BUILD
            COMMAND
                ${CMAKE_COMMAND} -E copy_directory ${ffmpeg-fetch_SOURCE_DIR}/bin $<TARGET_FILE_DIR:GalaxyEngine>)
endif()
