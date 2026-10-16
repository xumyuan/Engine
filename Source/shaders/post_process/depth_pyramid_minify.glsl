#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;

void main()
{
    gl_Position = vec4(position, 1.0);
}

#shader-type fragment
#version 450 core

out float FragColor;

layout(binding = 1) uniform sampler2D gDepthSource;

#include "../common/depth_utils.glsl"

// 与 UBODepthPyramidParams 的 std140 布局一致
layout (std140, binding = 4) uniform DepthPyramidParams {
    int isBaseLevel; // 1 = level 0（硬件深度 [0,1] -> 线性视图距离），0 = level 1..N（Rotated-Grid 子采样）
    int _pad0;
    int _pad1;
    int _pad2;
};

void main()
{
    ivec2 icoord = ivec2(gl_FragCoord.xy);
    if (isBaseLevel != 0)
    {
        float depth = texelFetch(gDepthSource, icoord, 0).r;
        FragColor = linearizeDepth(depth);
        return;
    }

    // 深度塔逐级 minify：Rotated-Grid 子采样（Filament mipmapDepth.mat / McGuire 2012 Table 1）
    // C++ 端已通过 setTextureMipRange 将采样器锁定在上一级 [srcLevel, srcLevel]，相对 lod 恒为 0
    ivec2 srcSize = textureSize(gDepthSource, 0);
    ivec2 srcCoord = min(2 * icoord + ivec2(icoord.y & 1, icoord.x & 1), srcSize - 1);
    FragColor = texelFetch(gDepthSource, srcCoord, 0).r;
}
