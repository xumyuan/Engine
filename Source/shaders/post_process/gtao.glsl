/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Adapted from Filament's gtaoImpl.fs and gtao.mat. Changes: Engine UBOs,
 * positive linear depth pyramid, exact acos, screen/sky rejection, guarded
 * normalization and sector shifts, and a radius bound for visibility masks.
 */

#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;
out vec2 TexCoords;

void main()
{
    TexCoords = texCoord;
    gl_Position = vec4(position, 1.0);
}

#shader-type fragment
#version 450 core

in vec2 TexCoords;
out vec3 FragColor;
layout(binding = 1) uniform sampler2D gDepthPyramid;

#include "../common/depth_utils.glsl"

// Matches UBOGTAOParams (48 bytes, std140).
layout (std140, binding = 4) uniform GTAOParams {
    int sliceCount;
    int stepsPerSlice;
    float invRadiusSquared;
    float projectionScaleRadius;
    float intensity;
    float power;
    float thicknessHeuristic;
    float constThickness;
    float farPlane;
    int maxLevel;
    int useVisibilityBitmasks;
    int linearThickness;
};

const float kPi = 3.14159265359;
const float kHalfPi = 0.5 * kPi;
const float kLog2LodRate = 3.0;
const uint kSectorCount = 32u;

float integrateArcCosWeight(float h, float n)
{
    return 0.25 * (-cos(2.0 * h - n) + cos(n) + 2.0 * h * sin(n));
}

float updateHorizon(vec3 delta, vec3 viewDir, float horizonCos)
{
    float distanceSquared = dot(delta, delta);
    float sampleCos = clamp(dot(delta, viewDir) * inversesqrt(distanceSquared), -1.0, 1.0);
    float falloff = clamp(distanceSquared * invRadiusSquared * 2.0, 0.0, 1.0);
    return sampleCos > horizonCos
        ? mix(sampleCos, horizonCos, falloff)
        : mix(horizonCos, sampleCos, thicknessHeuristic);
}

uint visibilityMask(vec3 delta, vec3 viewDir, float side, float n, float viewDist)
{
    if (dot(delta, delta) * invRadiusSquared >= 1.0)
        return 0u;

    float thicknessScale = linearThickness != 0 ? clamp(viewDist / farPlane, 0.0, 1.0) * 100.0 : 1.0;
    vec3 backDelta = delta - viewDir * constThickness * thicknessScale;
    vec2 cosFrontBack = vec2(
        dot(delta, viewDir) * inversesqrt(dot(delta, delta)),
        dot(backDelta, viewDir) * inversesqrt(max(dot(backDelta, backDelta), 1e-10)));
    vec2 angles = acos(clamp(cosFrontBack, -1.0, 1.0));
    vec2 sectors = clamp((-side * angles + n + kHalfPi) / kPi, 0.0, 1.0);
    sectors = side > 0.0 ? sectors.yx : sectors.xy;

    uint first = uint(sectors.x * float(kSectorCount));
    uint count = uint(ceil(clamp(sectors.y - sectors.x, 0.0, 1.0) * float(kSectorCount)));
    // GLSL shifts by 32 are undefined, including for an otherwise empty mask.
    if (first >= kSectorCount || count == 0u)
        return 0u;
    count = min(count, kSectorCount - first);
    return (0xFFFFFFFFu >> (kSectorCount - count)) << first;
}

void sampleSlice(vec2 uv, float level, vec3 origin, vec3 viewDir, float side, float n,
                 inout float horizon, inout uint mask)
{
    // Clamp-to-edge would otherwise invent geometry beyond the viewport.
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThanEqual(uv, vec2(1.0))))
        return;
    float distance = textureLod(gDepthPyramid, uv, level).r;
    if (distance <= 0.0 || distance >= farPlane * 0.999)
        return;
    vec3 delta = reconstructViewPositionFromViewDist(uv, distance) - origin;
    if (dot(delta, delta) <= 1e-10)
        return;

    if (useVisibilityBitmasks != 0)
        mask |= visibilityMask(delta, viewDir, side, n, -origin.z);
    else
        horizon = updateHorizon(delta, viewDir, horizon);
}

float groundTruthVisibility(vec3 origin, vec3 normal)
{
    vec3 viewDir = normalize(-origin);
    // 将采样半径转换为屏幕中的像素数
    float screenRadius = projectionScaleRadius / -origin.z;
    // 采样步长
    float stepRadius = screenRadius / (float(stepsPerSlice) + 1.0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    float noiseDirection = float((((pixel.x + pixel.y) & 3) << 2) + (pixel.x & 3)) / 16.0;
    float noiseOffset = float((pixel.y - pixel.x) & 3) * 0.25;
    
    float visibility = 0.0;

    for (int i = 0; i < sliceCount; ++i)
    {
        float slice = (float(i) + noiseDirection) / float(sliceCount);
        float phi = slice * kPi;
        
        vec2 omega = vec2(cos(phi), sin(phi));
        // 计算当前切片的方向，屏幕空间单位向量
        vec3 direction = vec3(omega, 0.0);
        // 获取垂直于视线的切片方向
        // 正交化之后得到 viewdir和 orthoDirection 的正交基，切片中的任一方向可以表示为这两个向量的线性组合
        // \omega(\theta)=\cos\theta\,viewDir+\sin\theta\,orthoDirection
        vec3 orthoDirection = normalize(direction - dot(direction, viewDir) * viewDir);
        
        vec3 axis = cross(orthoDirection, viewDir);
        // 计算法线在切片平面上的投影
        vec3 projectedNormal = normal - axis * dot(normal, axis);
        
        // 这里n求的是法线在切片内与viewDir的夹角
        float normalLength = length(projectedNormal);
        float cosNormal = clamp(dot(projectedNormal, viewDir) / max(normalLength, 1e-6), 0.0, 1.0);
        float n = sign(dot(orthoDirection, projectedNormal)) * acos(cosNormal);

        float horizon0 = -1.0;
        float horizon1 = -1.0;
        uint mask = 0u;

        for (int j = 0; j < stepsPerSlice; ++j)
        {
            float offsetLength = max((float(j) + noiseOffset) * stepRadius, 1.0 + float(j));
            vec2 offset = omega * offsetLength * texelSize;
            float level = clamp(floor(log2(offsetLength)) - kLog2LodRate, 0.0, float(maxLevel));
            sampleSlice(TexCoords + offset, level, origin, viewDir, 1.0, n, horizon0, mask);
            sampleSlice(TexCoords - offset, level, origin, viewDir, -1.0, n, horizon1, mask);
        }

        if (useVisibilityBitmasks != 0)
        {
            visibility += 1.0 - float(bitCount(mask)) / float(kSectorCount);
        }
        else
        {
            float h0 = -acos(clamp(horizon1, -1.0, 1.0));
            float h1 = acos(clamp(horizon0, -1.0, 1.0));
            h0 = n + clamp(h0 - n, -kHalfPi, kHalfPi);
            h1 = n + clamp(h1 - n, -kHalfPi, kHalfPi);
            visibility += normalLength * (integrateArcCosWeight(h0, n) + integrateArcCosWeight(h1, n));
        }
    }
    return clamp(visibility / float(sliceCount), 0.0, 1.0);
}

void main()
{
    float viewDist = textureLod(gDepthPyramid, TexCoords, 0.0).r;
    if (viewDist <= 0.0 || viewDist >= farPlane * 0.999)
    {
        FragColor = vec3(1.0);
        return;
    }

    float visibility = 1.0;
    if (intensity > 0.0)
    {
        vec3 origin = reconstructViewPositionFromViewDist(TexCoords, viewDist);
        vec3 normal = reconstructViewSpaceNormalFromViewDist(gDepthPyramid, TexCoords, viewDist, origin);
        visibility = groundTruthVisibility(origin, normal);
        visibility = clamp(mix(1.0, pow(visibility, power), intensity), 0.0, 1.0);
    }
    FragColor = vec3(visibility, packDepthKey(viewDist / farPlane));
}
