// Copyright (C) 2026 SharpEmu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#version 450
layout(location = 0) out vec4 color;
layout(set = 0, binding = 1) uniform sampler2D sourceTexture;
void main() { color = texture(sourceTexture, gl_FragCoord.xy / vec2(64.0)); }
