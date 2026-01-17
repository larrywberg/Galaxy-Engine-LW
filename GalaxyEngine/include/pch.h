#pragma once

// C++ stdlib
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <filesystem>
#include <array>
#include <algorithm>
#include <memory>
#include <limits> 
#include <chrono>
#include <regex>
#include <variant>
#include <thread>
#include <bitset>
#include <random>
#include <stack>
#include <execution>

// C stdlib
#include <cmath>
#include <cstdint>
#include <cstdio>

// Runtime
#if defined(_OPENMP)
#if __has_include(<omp.h>)
#include <omp.h>
#elif defined(EMSCRIPTEN)
extern "C" {
void omp_set_num_threads(int);
int omp_get_max_threads(void);
}
#else
#error "OpenMP requested but <omp.h> not found."
#endif
#endif

// Vendor
#include <glm.hpp>

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#if defined(EMSCRIPTEN)
#include <GLES3/gl3.h>
#else
#include <external/glad.h>
#endif

#include <imgui.h>

#include <implot.h>
#include <implot_internal.h>

#if defined(EMSCRIPTEN)
#include "rlImGui_stub.h"
#include "rlImGuiColors_stub.h"
#else
#include <rlImGui.h>
#include <rlImGuiColors.h>
#endif

#include <yaml-cpp/yaml.h>
