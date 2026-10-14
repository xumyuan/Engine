#ifndef ENGINE_COMMON_DEPTH_UTILS_GLSL
#define ENGINE_COMMON_DEPTH_UTILS_GLSL

#include "per_frame.glsl"

// 从深度（[0, 1]）和 UV 重建视图空间位置（右手系，相机朝 -Z）
vec3 reconstructViewPosition(vec2 uv, float depth)
{
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPosRecon = projectionInverse * clipPos;
    return viewPosRecon.xyz / viewPosRecon.w;
}

// 单侧差分重建几何法线（MediumQ，2 次额外采样）
vec3 reconstructViewSpaceNormalMediumQ(sampler2D depthTex, vec2 uv, vec3 origin)
{
    vec2 uvdx = uv + vec2(texelSize.x, 0.0);
    vec2 uvdy = uv + vec2(0.0, texelSize.y);
    vec3 px = reconstructViewPosition(uvdx, texture(depthTex, uvdx).r);
    vec3 py = reconstructViewPosition(uvdy, texture(depthTex, uvdy).r);
    return normalize(cross(px - origin, py - origin));
}

// 二阶外推双侧差分重建几何法线（Filament HighQ，自动避开深度断层边缘）
vec3 reconstructViewSpaceNormalHighQ(sampler2D depthTex, vec2 uv, float depth, vec3 origin)
{
    vec2 dx = vec2(texelSize.x, 0.0);
    vec2 dy = vec2(0.0, texelSize.y);

    vec4 H;
    H.x = texture(depthTex, uv - dx).r;
    H.y = texture(depthTex, uv + dx).r;
    H.z = texture(depthTex, uv - dx * 2.0).r;
    H.w = texture(depthTex, uv + dx * 2.0).r;
    vec2 he = abs((2.0 * H.xy - H.zw) - depth);
    vec3 dpdx = (he.x < he.y)
        ? (origin - reconstructViewPosition(uv - dx, H.x))
        : (reconstructViewPosition(uv + dx, H.y) - origin);

    vec4 V;
    V.x = texture(depthTex, uv - dy).r;
    V.y = texture(depthTex, uv + dy).r;
    V.z = texture(depthTex, uv - dy * 2.0).r;
    V.w = texture(depthTex, uv + dy * 2.0).r;
    vec2 ve = abs((2.0 * V.xy - V.zw) - depth);
    vec3 dpdy = (ve.x < ve.y)
        ? (origin - reconstructViewPosition(uv - dy, V.x))
        : (reconstructViewPosition(uv + dy, V.y) - origin);

    return normalize(cross(dpdx, dpdy));
}

float interleavedGradientNoise(vec2 fragCoord)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(fragCoord, magic.xy)));
}

#endif
