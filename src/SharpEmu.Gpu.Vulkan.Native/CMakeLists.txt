# Copyright (C) 2026 SharpEmu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.25)
project(SharpEmuGpuVulkan VERSION 0.1.0 LANGUAGES CXX)

find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG QUIET)
if(TARGET SDL3::SDL3)
    set(SE_GPU_SDL_TARGET SDL3::SDL3)
elseif(TARGET SDL3::SDL3-shared)
    set(SE_GPU_SDL_TARGET SDL3::SDL3-shared)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3>=3.2)
    set(SE_GPU_SDL_TARGET PkgConfig::SDL3)
endif()

add_library(sharpemu_gpu_vulkan SHARED src/backend.cpp)
target_compile_features(sharpemu_gpu_vulkan PRIVATE cxx_std_20)
target_compile_definitions(sharpemu_gpu_vulkan PRIVATE SE_GPU_BUILD)
target_include_directories(sharpemu_gpu_vulkan PUBLIC include)
target_link_libraries(sharpemu_gpu_vulkan PRIVATE Vulkan::Vulkan ${SE_GPU_SDL_TARGET})
if(MSVC)
    target_compile_options(sharpemu_gpu_vulkan PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(sharpemu_gpu_vulkan PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
endif()
set_target_properties(sharpemu_gpu_vulkan PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Visual Studio is a multi-config generator and otherwise inserts an extra
# Debug/Release directory. Keep the ABI library at the path consumed by the
# managed project on every generator.
foreach(SE_GPU_CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${SE_GPU_CONFIG}" SE_GPU_CONFIG_UPPER)
    set_target_properties(sharpemu_gpu_vulkan PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${SE_GPU_CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY_${SE_GPU_CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin")
endforeach()

if(WIN32)
    add_custom_command(TARGET sharpemu_gpu_vulkan POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:sharpemu_gpu_vulkan>
            $<TARGET_FILE_DIR:sharpemu_gpu_vulkan>
        COMMAND_EXPAND_LISTS)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(sharpemu_gpu_vulkan_exports_test tests/exports_test.cpp)
    target_link_libraries(sharpemu_gpu_vulkan_exports_test PRIVATE sharpemu_gpu_vulkan)
    set_target_properties(sharpemu_gpu_vulkan_exports_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    foreach(SE_GPU_CONFIG Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${SE_GPU_CONFIG}" SE_GPU_CONFIG_UPPER)
        set_target_properties(sharpemu_gpu_vulkan_exports_test PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${SE_GPU_CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin")
    endforeach()
    add_test(NAME sharpemu_gpu_vulkan_exports_test COMMAND sharpemu_gpu_vulkan_exports_test)

    if(TARGET Vulkan::glslc)
        set(SMOKE_COMPUTE_SPV "${CMAKE_CURRENT_BINARY_DIR}/smoke.comp.spv")
        set(SMOKE_VERTEX_SPV "${CMAKE_CURRENT_BINARY_DIR}/smoke.vert.spv")
        set(SMOKE_FRAGMENT_SPV "${CMAKE_CURRENT_BINARY_DIR}/smoke.frag.spv")
        add_custom_command(OUTPUT "${SMOKE_COMPUTE_SPV}"
            COMMAND Vulkan::glslc "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.comp" -o "${SMOKE_COMPUTE_SPV}"
            DEPENDS tests/smoke.comp VERBATIM)
        add_custom_command(OUTPUT "${SMOKE_VERTEX_SPV}"
            COMMAND Vulkan::glslc "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.vert" -o "${SMOKE_VERTEX_SPV}"
            DEPENDS tests/smoke.vert VERBATIM)
        add_custom_command(OUTPUT "${SMOKE_FRAGMENT_SPV}"
            COMMAND Vulkan::glslc "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.frag" -o "${SMOKE_FRAGMENT_SPV}"
            DEPENDS tests/smoke.frag VERBATIM)
        add_custom_target(sharpemu_gpu_vulkan_test_shaders
            DEPENDS "${SMOKE_COMPUTE_SPV}" "${SMOKE_VERTEX_SPV}" "${SMOKE_FRAGMENT_SPV}")
        add_executable(sharpemu_gpu_vulkan_abi_test tests/abi_test.cpp)
        target_link_libraries(sharpemu_gpu_vulkan_abi_test PRIVATE sharpemu_gpu_vulkan)
        add_dependencies(sharpemu_gpu_vulkan_abi_test sharpemu_gpu_vulkan_test_shaders)
        add_test(NAME sharpemu_gpu_vulkan_abi_test
            COMMAND sharpemu_gpu_vulkan_abi_test
                "${SMOKE_COMPUTE_SPV}" "${SMOKE_VERTEX_SPV}" "${SMOKE_FRAGMENT_SPV}")
    endif()
endif()
