cmake_minimum_required(VERSION 3.20)
project(DreamcastMac VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization flags
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    add_compile_options(-Wall -Wextra -O3)
    if (APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        add_compile_options(-arch arm64)
    endif()
endif()

# Include directories
include_directories(
    runtime/include
    tools/recompiler/include
)

# -----------------------------------------------------------------------------
# Runtime Library (dc-runtime)
# -----------------------------------------------------------------------------
set(RUNTIME_SOURCES
    runtime/src/memory.cpp
    runtime/src/powervr_metal.cpp
)

if (APPLE)
    enable_language(OBJCXX)
    set_source_files_properties(runtime/src/powervr_metal.cpp PROPERTIES LANGUAGE OBJCXX)
    find_library(METAL_LIB Metal REQUIRED)
    find_library(QUARTZCORE_LIB QuartzCore REQUIRED)
    find_library(METALFX_LIB MetalFX)
    find_library(GAMECONTROLLER_LIB GameController REQUIRED)
endif()

add_library(dc-runtime STATIC ${RUNTIME_SOURCES})
target_include_directories(dc-runtime PUBLIC runtime/include)

if (APPLE)
    target_link_libraries(dc-runtime PUBLIC
        ${METAL_LIB}
        ${QUARTZCORE_LIB}
        ${GAMECONTROLLER_LIB}
    )
    if (METALFX_LIB)
        target_link_libraries(dc-runtime PUBLIC ${METALFX_LIB})
    endif()
endif()

# -----------------------------------------------------------------------------
# Recompiler Tool (dc-recomp)
# -----------------------------------------------------------------------------
set(RECOMPILER_SOURCES
    tools/recompiler/src/main.cpp
    tools/recompiler/src/sh4_disasm.cpp
    tools/recompiler/src/ir_emitter.cpp
)

add_executable(dc-recomp ${RECOMPILER_SOURCES})
target_include_directories(dc-recomp PRIVATE tools/recompiler/include runtime/include)

# -----------------------------------------------------------------------------
# Samples & Verification
# -----------------------------------------------------------------------------
add_executable(generate_sample samples/sh4_test/generate_sample.cpp)

add_executable(verify_execution 
    samples/sh4_test/verify_execution.cpp
    samples/sh4_test/sample_recompiled.cpp
)
target_link_libraries(verify_execution PRIVATE dc-runtime)
