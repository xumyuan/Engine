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

// 从深度（[0, 1]）和 UV 重建世界空间位置
vec3 reconstructWorldPosition(vec2 uv, float depth)
{
    vec3 viewPosRecon = reconstructViewPosition(uv, depth);
    return (viewInverse * vec4(viewPosRecon, 1.0)).xyz;
}

// 将 [0, 1] 硬件深度转换为相机前方的正值线性距离（-viewZ）
float linearizeDepth(float depth)
{
    float ndcZ = depth * 2.0 - 1.0;
    return projection[3][2] / (ndcZ + projection[2][2]);
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

// 从线性视图距离（-viewZ，正值）重建视图空间位置
vec3 reconstructViewPositionFromViewDist(vec2 uv, float viewDist)
{
    vec2 ndc = uv * 2.0 - 1.0;
    return vec3(ndc * vec2(projectionInverse[0][0], projectionInverse[1][1]) * viewDist,
        -viewDist);
}

// 基于线性视图距离纹理（深度塔 level 0）的 HighQ 法线重建
vec3 reconstructViewSpaceNormalFromViewDist(sampler2D distTex, vec2 uv, float viewDist, vec3 origin)
{
    vec2 dx = vec2(texelSize.x, 0.0);
    vec2 dy = vec2(0.0, texelSize.y);
    float invDist = 1.0 / viewDist;

    vec4 H;
    H.x = textureLod(distTex, uv - dx, 0.0).r;
    H.y = textureLod(distTex, uv + dx, 0.0).r;
    H.z = textureLod(distTex, uv - dx * 2.0, 0.0).r;
    H.w = textureLod(distTex, uv + dx * 2.0, 0.0).r;
    vec4 invH = 1.0 / H;
    vec2 he = abs((2.0 * invH.xy - invH.zw) - invDist);
    vec3 dpdx = (he.x < he.y)
        ? (origin - reconstructViewPositionFromViewDist(uv - dx, H.x))
        : (reconstructViewPositionFromViewDist(uv + dx, H.y) - origin);

    vec4 V;
    V.x = textureLod(distTex, uv - dy, 0.0).r;
    V.y = textureLod(distTex, uv + dy, 0.0).r;
    V.z = textureLod(distTex, uv - dy * 2.0, 0.0).r;
    V.w = textureLod(distTex, uv + dy * 2.0, 0.0).r;
    vec4 invV = 1.0 / V;
    vec2 ve = abs((2.0 * invV.xy - invV.zw) - invDist);
    vec3 dpdy = (ve.x < ve.y)
        ? (origin - reconstructViewPositionFromViewDist(uv - dy, V.x))
        : (reconstructViewPositionFromViewDist(uv + dy, V.y) - origin);

    return normalize(cross(dpdx, dpdy));
}

// 双边滤波深度 key 打包与解包（Filament ssaoUtils.fs）
vec2 packDepthKey(float normalizedDepth)
{
    float z = clamp(normalizedDepth, 0.0, 1.0);
    float t = floor(256.0 * z);
    return vec2(t * (1.0 / 256.0), 256.0 * z - t);
}

float unpackDepthKey(vec2 p)
{
    return p.x * (256.0 / 257.0) + p.y * (1.0 / 257.0);
}

float interleavedGradientNoise(vec2 fragCoord)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(fragCoord, magic.xy)));
}

#endif
