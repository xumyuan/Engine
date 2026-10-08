#pragma once

#include <cstdint>

namespace engine {

// ============================================================================
// 纹理单元编号
// ============================================================================
// 与 shader 中 sampler 的 layout(binding = N) 一一对应；GLSL 没有 #include，修改编号时需同步修改 Source/Shaders/ 下对应的 shader
// 同一个 shader 内编号不能重复，同一编号上也不能同时出现不同类型的 sampler（GL 会在绘制时报 GL_INVALID_OPERATION）
namespace TextureUnit {

    // ----- 光照相关（前向 / 延迟光照 / 地形共用）-----
    static constexpr uint32_t DirLightShadowmap      = 0;
    static constexpr uint32_t IrradianceMap          = 1;
    static constexpr uint32_t PrefilterMap           = 2;
    static constexpr uint32_t BrdfLUT                = 3;
    static constexpr uint32_t SpotLightShadowmap     = 11;
    static constexpr uint32_t PointLightShadowCubemap = 12;

    // ----- 模型材质 -----
    static constexpr uint32_t MaterialAlbedo         = 4;
    static constexpr uint32_t MaterialNormal         = 5;
    static constexpr uint32_t MaterialMetallic       = 6;
    static constexpr uint32_t MaterialRoughness      = 7;
    static constexpr uint32_t MaterialAO             = 8;
    static constexpr uint32_t MaterialDisplacement   = 9;
    static constexpr uint32_t MaterialEmission       = 10;

    // ----- 延迟光照读取的 G-buffer -----
    static constexpr uint32_t GBufferAlbedo          = 6;
    static constexpr uint32_t GBufferNormal          = 7;
    static constexpr uint32_t GBufferMaterialInfo    = 8;
    static constexpr uint32_t GBufferSSAO            = 9;
    static constexpr uint32_t GBufferDepth           = 10;

    // ----- 地形：21 张纹理从 TerrainBase 起连续排列 -----
    // 顺序：albedo1-4, normal1-4, roughness1-4, metallic1-4, AO1-4, blendmap
    static constexpr uint32_t TerrainBase            = 13;
    static constexpr uint32_t TerrainTextureCount    = 21;

    // ----- 单输入 / 少输入的全屏 pass -----
    static constexpr uint32_t PostProcessInput       = 0;  // copy / fxaa / gammaCorrect / ssao_blur
    static constexpr uint32_t SkyboxCubemap          = 0;
    static constexpr uint32_t SceneCaptureCubemap    = 0;  // 光照探针卷积 / 反射探针重要性采样
    static constexpr uint32_t SSAONormal             = 0;
    static constexpr uint32_t SSAODepth              = 1;
    static constexpr uint32_t SSAONoise              = 2;

    // ----- 屏幕空间流体（Source/Shaders/fluid/ssf_*.glsl）-----
    static constexpr uint32_t SSFSceneDepth          = 0;  // ssf_depth / ssf_thickness：遮挡剔除
    static constexpr uint32_t SSFSmoothInput         = 0;  // ssf_smooth
    static constexpr uint32_t SSFSceneColor          = 0;  // ssf_composite
    static constexpr uint32_t SSFFluidDepth          = 1;
    static constexpr uint32_t SSFThickness           = 2;
    static constexpr uint32_t SSFEnvironment         = 3;
}

} // namespace engine
