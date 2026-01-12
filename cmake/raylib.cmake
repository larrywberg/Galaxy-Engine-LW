set(RL_VERSION 5.5)
set(RL_URL_BASE https://github.com/raysan5/raylib/releases/download)

if(EMSCRIPTEN)
    set(PLATFORM "Web" CACHE STRING "" FORCE)
    set(GRAPHICS "GRAPHICS_API_OPENGL_ES2" CACHE STRING "" FORCE)
else()
    set(PLATFORM "Desktop" CACHE STRING "" FORCE)
endif()

set(RAYLIB_SHARED OFF CACHE BOOL "" FORCE)
set(RAYLIB_STATIC ON CACHE BOOL "" FORCE)
set(RAYLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAYLIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RAYLIB_BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(RAYLIB_BUILD_UTILS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(raylib
GIT_REPOSITORY https://github.com/raysan5/raylib.git
GIT_TAG master)
FetchContent_MakeAvailable(raylib)

add_library(raylib-lib INTERFACE)

target_include_directories(raylib-lib INTERFACE ${raylib_SOURCE_DIR}/src)
target_link_libraries(raylib-lib INTERFACE raylib)
