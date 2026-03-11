#pragma once

#include <glm/glm.hpp>

namespace engine {

// ============================================================================
// UBO Binding Points
// ============================================================================
// 这些 binding 点与 shader 中的 layout(std140, binding=N) 一一对应
namespace UBOBinding {
    static constexpr uint32_t PerFrame      = 0;
    static constexpr uint32_t PerObject     = 1;
    static constexpr uint32_t Lighting      = 2;
    static constexpr uint32_t MaterialParams = 3;
    static constexpr uint32_t CustomParams  = 4;  // SSAO, PostProcess, Fluid 等
}

// ============================================================================
// std140 布局规则速查:
//   - float/int/bool(as int): 4 bytes, align 4
//   - vec2:   8 bytes, align 8
//   - vec3:  12 bytes, align 16 (padded to 16)
//   - vec4:  16 bytes, align 16
//   - mat3:  3 * vec4 = 48 bytes, align 16 (每列填充为 vec4)
//   - mat4:  4 * vec4 = 64 bytes, align 16
//   - 数组元素: 每个 align 16 (即 float[N] 每个元素占 16 bytes)
//   - struct: align 为其最大成员的 align, 并 round up 到 16
// ============================================================================

// ============================================================================
// UBO 0: PerFrame — 每帧设置一次的相机/视图数据
// ============================================================================
struct alignas(16) UBOPerFrame {
    glm::mat4 view;                // offset 0    (64 bytes)
    glm::mat4 projection;          // offset 64   (64 bytes)
    glm::mat4 viewInverse;         // offset 128  (64 bytes)
    glm::mat4 projectionInverse;   // offset 192  (64 bytes)
    glm::vec4 viewPos;             // offset 256  (16 bytes) — xyz = viewPos, w = padding
    glm::vec2 screenSize;          // offset 272  (8 bytes)
    glm::vec2 texelSize;           // offset 280  (8 bytes)
    // Total: 288 bytes
};
static_assert(sizeof(UBOPerFrame) == 288, "UBOPerFrame size mismatch with std140 layout");

// ============================================================================
// UBO 1: PerObject — 每个绘制调用更新的物体变换
// ============================================================================
struct alignas(16) UBOPerObject {
    glm::mat4 model;               // offset 0    (64 bytes)
    // mat3 in std140 = 3 x vec4 (each column padded to vec4)
    glm::vec4 normalMatrix_col0;   // offset 64   (16 bytes) — normalMatrix column 0
    glm::vec4 normalMatrix_col1;   // offset 80   (16 bytes) — normalMatrix column 1
    glm::vec4 normalMatrix_col2;   // offset 96   (16 bytes) — normalMatrix column 2
    // Total: 112 bytes
};
static_assert(sizeof(UBOPerObject) == 112, "UBOPerObject size mismatch with std140 layout");

// Helper to set normalMatrix from glm::mat3
inline void setNormalMatrix(UBOPerObject& ubo, const glm::mat3& normalMatrix) {
    ubo.normalMatrix_col0 = glm::vec4(normalMatrix[0], 0.0f);
    ubo.normalMatrix_col1 = glm::vec4(normalMatrix[1], 0.0f);
    ubo.normalMatrix_col2 = glm::vec4(normalMatrix[2], 0.0f);
}

// ============================================================================
// UBO 2: Lighting — 灯光数据
// ============================================================================
#define MAX_DIR_LIGHTS 3
#define MAX_POINT_LIGHTS 6
#define MAX_SPOT_LIGHTS 6

struct alignas(16) UBODirLight {
    glm::vec4 direction;           // xyz = direction, w = intensity
    glm::vec4 lightColour;         // xyz = colour, w = padding
    // Total: 32 bytes
};

struct alignas(16) UBOPointLight {
    glm::vec4 position;            // xyz = position, w = intensity
    glm::vec4 lightColour;         // xyz = colour, w = attenuationRadius
    // Total: 32 bytes
};

struct alignas(16) UBOSpotLight {
    glm::vec4 position;            // xyz = position, w = intensity
    glm::vec4 direction;           // xyz = direction, w = attenuationRadius
    glm::vec4 lightColour;         // xyz = colour, w = cutOff
    glm::vec4 params;              // x = outerCutOff, y/z/w = padding
    // Total: 64 bytes
};

struct alignas(16) UBOShadowData {
    glm::mat4 lightSpaceViewProjectionMatrix;  // 64 bytes
    float shadowBias;              // 4 bytes
    int lightShadowIndex;          // 4 bytes
    float _pad0;                   // 4 bytes
    float _pad1;                   // 4 bytes
    // Total: 80 bytes
};

struct alignas(16) UBOShadowDataPointLight {
    float farPlane;                // 4 bytes
    float shadowBias;              // 4 bytes
    int lightShadowIndex;          // 4 bytes
    float _pad0;                   // 4 bytes
    // Total: 16 bytes
};

struct alignas(16) UBOLighting {
    glm::ivec4 numDirPointSpotLights;                // offset 0   (16 bytes)
    UBODirLight dirLights[MAX_DIR_LIGHTS];            // offset 16  (96 bytes)
    UBOPointLight pointLights[MAX_POINT_LIGHTS];      // offset 112 (192 bytes)
    UBOSpotLight spotLights[MAX_SPOT_LIGHTS];          // offset 304 (384 bytes)
    UBOShadowData dirLightShadowData;                 // offset 688 (80 bytes)
    UBOShadowData spotLightShadowData;                // offset 768 (80 bytes)
    UBOShadowDataPointLight pointLightShadowData;     // offset 848 (16 bytes)
    // Total: 864 bytes
};

// ============================================================================
// UBO 3: MaterialParams — 材质标量参数 (纹理 sampler 保持独立 uniform)
// ============================================================================
struct alignas(16) UBOMaterialParams {
    glm::vec4 albedoColour;        // offset 0   (16 bytes)
    glm::vec4 emissionColour;      // offset 16  (16 bytes) — xyz = emission, w = emissionIntensity
    float metallicValue;           // offset 32  (4 bytes)
    float roughnessValue;          // offset 36  (4 bytes)
    float parallaxStrength;        // offset 40  (4 bytes)
    float tilingAmount;            // offset 44  (4 bytes)
    int hasAlbedoTexture;          // offset 48  (4 bytes) — bool as int for std140
    int hasMetallicTexture;        // offset 52  (4 bytes)
    int hasRoughnessTexture;       // offset 56  (4 bytes)
    int hasEmissionTexture;        // offset 60  (4 bytes)
    int hasDisplacement;           // offset 64  (4 bytes)
    int hasEmission;               // offset 68  (4 bytes)
    glm::vec2 minMaxDisplacementSteps;  // offset 72 (8 bytes)
    // Total: 80 bytes
};
static_assert(sizeof(UBOMaterialParams) == 80, "UBOMaterialParams size mismatch with std140 layout");

// ============================================================================
// UBO 4: SSAOParams — SSAO 特有参数
// ============================================================================
struct alignas(16) UBOSSAOParams {
    glm::vec4 samples[64];         // offset 0   (1024 bytes) — vec3 packed as vec4
    int kernelSize;                // offset 1024 (4 bytes)
    float radius;                  // offset 1028 (4 bytes)
    float bias;                    // offset 1032 (4 bytes)
    float power;                   // offset 1036 (4 bytes)
    // Total: 1040 bytes
};
static_assert(sizeof(UBOSSAOParams) == 1040, "UBOSSAOParams size mismatch with std140 layout");

// ============================================================================
// UBO 4: PostProcessParams — 后处理参数（与 SSAOParams 共用 binding 4，不同 shader）
// ============================================================================
struct alignas(16) UBOPostProcessParams {
    float gamma_inverse;           // 4 bytes
    float exposure;                // 4 bytes
    glm::vec2 texel_size;          // 8 bytes
    // Total: 16 bytes
};
static_assert(sizeof(UBOPostProcessParams) == 16, "UBOPostProcessParams size mismatch");

// ============================================================================
// UBO 4: FluidParams — 流体渲染特有参数
// ============================================================================
struct alignas(16) UBOFluidParams {
    glm::vec4 lightPos;            // xyz = lightPos, w = pointScale
    glm::vec4 lightColor;          // xyz = lightColor, w = pointSize
    glm::vec4 objectColor;         // xyz = objectColor, w = padding
    // Total: 48 bytes
};
static_assert(sizeof(UBOFluidParams) == 48, "UBOFluidParams size mismatch");

// ============================================================================
// Shadowmap UBO — lightSpaceViewProjectionMatrix 用于 shadowmap pass
// ============================================================================
struct alignas(16) UBOShadowmapPass {
    glm::mat4 lightSpaceViewProjectionMatrix;  // offset 0 (64 bytes)
    // Total: 64 bytes
};

// ============================================================================
// ForwardProbe UBO — roughness 参数
// ============================================================================
struct alignas(16) UBOProbeParams {
    float roughness;               // 4 bytes
    float _pad0;                   // 4 bytes
    float _pad1;                   // 4 bytes
    float _pad2;                   // 4 bytes
    // Total: 16 bytes
};

// ============================================================================
// Terrain ClipPlane UBO
// ============================================================================
struct alignas(16) UBOClipPlane {
    glm::vec4 clipPlane;           // 16 bytes
    int usesClipPlane;             // 4 bytes
    float _pad0;                   // 4 bytes
    float _pad1;                   // 4 bytes
    float _pad2;                   // 4 bytes
    // Total: 32 bytes
};

// IBL params (shared by deferred lighting and forward)
struct alignas(16) UBOIBLParams {
    int reflectionProbeMipCount;   // 4 bytes
    int computeIBL;                // 4 bytes
    int useSSAO;                   // 4 bytes
    int _pad0;                     // 4 bytes
    // Total: 16 bytes
};

} // namespace engine
