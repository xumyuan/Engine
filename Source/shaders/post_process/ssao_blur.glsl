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

out vec3 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D ssaoInput;

#include "../common/depth_utils.glsl"

// 与 UBOBilateralBlurParams 的 std140 布局一致
layout (std140, binding = 4) uniform BilateralBlurParams {
    vec2 axis; // 每步的 UV 偏移（含 stride 缩放，分两遍分别传水平/竖直）
    int sampleCount; // 高斯核元素个数（含中心）
    float farPlaneOverEdgeDistance; // farPlane / bilateralThreshold
    float sigma; // 高斯标准差（以 tap 步长为单位）
    float _pad0;
    vec2 _pad1;
};

// 深度差越接近，权重越接近空间权重；跨深度断层时迅速衰减到 0
float bilateralWeight(float depth, float sampleDepth)
{
    float diff = (sampleDepth - depth) * farPlaneOverEdgeDistance;
    return max(0.0, 1.0 - diff * diff);
}

void tap(inout float sum, inout float totalWeight, float weight, float depth, vec2 position)
{
    vec3 data = textureLod(ssaoInput, position, 0.0).rgb;
    float bilateral = weight * bilateralWeight(depth, unpackDepthKey(data.gb));
    sum += data.r * bilateral;
    totalWeight += bilateral;
}

// 通用 AO 可分离双边模糊（对齐 Filament bilateralBlur.mat）：
// Crytek 与 SAO 共用；C++ 按水平、竖直各调用一遍，每遍是高斯空间权重 x 深度双边权重，
// 中心像素的 bilateral key 原样透传给下一遍继续使用
void main()
{
    vec3 data = textureLod(ssaoInput, TexCoords, 0.0).rgb;
    if (data.g * data.b == 1.0)
    {
        // 天空哨兵值（AO pass 对天空输出 vec3(1)），跳过过滤
        FragColor = data;
        return;
    }

    float depth = unpackDepthKey(data.gb);

    // 中心 tap 的高斯权重 exp(0) = 1
    float totalWeight = 1.0;
    float sum = data.r * totalWeight;

    float invTwoSigma2 = 1.0 / (2.0 * sigma * sigma);
    for (int i = 1; i < sampleCount; ++i)
    {
        float fi = float(i);
        float weight = exp(-fi * fi * invTwoSigma2);
        vec2 offset = axis * fi;
        tap(sum, totalWeight, weight, depth, TexCoords + offset);
        tap(sum, totalWeight, weight, depth, TexCoords - offset);
    }

    float ao = sum * (1.0 / totalWeight);

    // 1/255 抖动，消除 8-bit 输出的渐变条带（Filament 同款）
    ao += (interleavedGradientNoise(gl_FragCoord.xy) - 0.5) / 255.0;

    FragColor = vec3(ao, data.gb);
}
