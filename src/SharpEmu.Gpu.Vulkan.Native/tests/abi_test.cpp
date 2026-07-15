/* Copyright (C) 2026 SharpEmu Emulator Project
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "sharpemu_gpu_vulkan.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>

std::vector<char> read_file(const char* path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

int main(int argc, char** argv) {
    if (se_gpu_abi_version() != SE_GPU_ABI_VERSION) return 1;
    uint32_t output_kind{};
    if (se_gpu_render_target_output_kind(10, 4, &output_kind) != SE_GPU_OK || output_kind != 1) return 5;
    if (se_gpu_render_target_output_kind(0xffffffffu, 0, &output_kind) != SE_GPU_NOT_FOUND) return 6;
    se_gpu_create_info info{}; info.struct_size = sizeof(info); info.abi_version = SE_GPU_ABI_VERSION;
    info.width = 64; info.height = 64; info.title_utf8 = "SharpEmu native GPU test";
    se_gpu_backend* backend{};
    if (se_gpu_create(&info, &backend) != SE_GPU_OK) return 2;
    if (argc != 4) { se_gpu_destroy(backend); return 8; }
    std::vector<char> shader = read_file(argv[1]);
    se_gpu_compute compute{}; compute.struct_size = sizeof(compute);
    compute.spirv = {shader.data(), shader.size()}; compute.groups_x = 1; compute.groups_y = 1; compute.groups_z = 1;
    if (shader.empty() || se_gpu_submit_compute(backend, &compute) != SE_GPU_OK) {
        se_gpu_destroy(backend); return 9;
    }
    std::vector<char> vertex = read_file(argv[2]);
    std::vector<char> fragment = read_file(argv[3]);
    se_gpu_draw draw{}; draw.struct_size = sizeof(draw); draw.width = 64; draw.height = 64;
    draw.vertex_spirv = {vertex.data(), vertex.size()}; draw.pixel_spirv = {fragment.data(), fragment.size()};
    std::vector<uint32_t> padded_texture(8, 0xff804020u);
    se_gpu_texture texture{}; texture.struct_size = sizeof(texture); texture.width = 2; texture.height = 2;
    texture.format = 10; texture.pitch = 4; texture.dst_select = 0x324;
    texture.rgba_pixels = {padded_texture.data(), padded_texture.size() * sizeof(uint32_t)};
    draw.textures = &texture; draw.texture_count = 1;
    draw.vertex_count = 3; draw.instance_count = 1; draw.primitive_type = 4;
    if (vertex.empty() || fragment.empty() || se_gpu_submit_draw(backend, &draw) != SE_GPU_OK) {
        se_gpu_destroy(backend); return 10;
    }
    std::vector<uint32_t> pixels(64 * 64);
    for (uint32_t frame = 0; frame < 3; ++frame) {
        std::fill(pixels.begin(), pixels.end(), 0xff000000u | frame * 0x00303030u);
        if (se_gpu_present_bgra(backend, pixels.data(), pixels.size() * sizeof(uint32_t), 64, 64, 256) != SE_GPU_OK) {
            se_gpu_destroy(backend); return 3;
        }
        uint32_t close{}; if (se_gpu_poll(backend, &close) != SE_GPU_OK || close) {
            se_gpu_destroy(backend); return 4;
        }
        se_gpu_input input{}; input.struct_size = sizeof(input);
        if (se_gpu_input_snapshot(backend, &input) != SE_GPU_OK) { se_gpu_destroy(backend); return 7; }
    }
    se_gpu_destroy(backend); return 0;
}
