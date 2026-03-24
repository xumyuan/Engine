#pragma once

#include "UniformBufferData.h"
#include "rhi/include/RHIDevice.h"
#include "rhi/include/RHIHandle.h"
#include "rhi/include/RHIResources.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace engine {

// UniformBufferManager
// 集中管理所有 UBO 的创建、更新和绑定
// 每个 UBO 对应一个 binding point，与 shader 中的 layout(std140, binding=N) 一致
class UniformBufferManager {
public:
    UniformBufferManager();
    ~UniformBufferManager();

    // 初始化：创建所有 UBO
    void initialize(rhi::RHIDevice* device);

    // 释放所有 UBO
    void shutdown();

    // ===== PerFrame (binding 0) =====
    void updatePerFrame(const glm::mat4& view, const glm::mat4& projection,
                        const glm::vec3& viewPos,
                        const glm::vec2& screenSize = glm::vec2(0.0f));
    void bindPerFrame();

    // ===== PerObject (binding 1) =====
    void updatePerObject(const glm::mat4& model, const glm::mat3& normalMatrix);
    void updatePerObject(const glm::mat4& model);  // 自动计算 normalMatrix
    void bindPerObject();

    // ===== Lighting (binding 2) =====
    UBOLighting& getLightingData() { return m_LightingData; }
    void updateLighting();
    void bindLighting();

    // ===== MaterialParams (binding 3) =====
    UBOMaterialParams& getMaterialParamsData() { return m_MaterialParamsData; }
    void updateMaterialParams();
    void bindMaterialParams();

    // ===== Custom (binding 4) - 用于 SSAO/PostProcess/Fluid =====
    void updateSSAOParams(const UBOSSAOParams& params);
    void updatePostProcessParams(const UBOPostProcessParams& params);
    void updateFluidParams(const UBOFluidParams& params);
    void updateShadowmapPass(const UBOShadowmapPass& params);
    void updateProbeParams(const UBOProbeParams& params);
    void updateClipPlane(const UBOClipPlane& params);
    void updateIBLParams(const UBOIBLParams& params);
    void bindCustom(uint32_t size);

    // 通用绑定接口
    void bind(uint32_t binding, rhi::BufferHandle handle, uint32_t size);

    // ===== 句柄访问器（供命令录制使用）=====
    rhi::BufferHandle getPerFrameHandle() const { return m_PerFrameUBO; }
    rhi::BufferHandle getPerObjectHandle() const { return m_PerObjectUBO; }
    rhi::BufferHandle getLightingHandle() const { return m_LightingUBO; }
    rhi::BufferHandle getMaterialParamsHandle() const { return m_MaterialParamsUBO; }
    rhi::BufferHandle getCustomHandle() const { return m_CustomUBO; }

    // ===== CPU 侧数据访问（供命令录制前填充）=====
    const UBOPerFrame& getPerFrameData() const { return m_PerFrameData; }
    const UBOPerObject& getPerObjectData() const { return m_PerObjectData; }
    const UBOLighting& getLightingDataConst() const { return m_LightingData; }
    const UBOMaterialParams& getMaterialParamsDataConst() const { return m_MaterialParamsData; }

    // ===== 仅更新 CPU 侧数据（不上传 GPU，供命令录制模式使用）=====
    void preparePerFrame(const glm::mat4& view, const glm::mat4& projection,
                         const glm::vec3& viewPos,
                         const glm::vec2& screenSize = glm::vec2(0.0f));
    void preparePerObject(const glm::mat4& model, const glm::mat3& normalMatrix);
    void preparePerObject(const glm::mat4& model);

private:
    rhi::RHIDevice* m_Device = nullptr;

    // UBO handles
    rhi::BufferHandle m_PerFrameUBO;
    rhi::BufferHandle m_PerObjectUBO;
    rhi::BufferHandle m_LightingUBO;
    rhi::BufferHandle m_MaterialParamsUBO;
    rhi::BufferHandle m_CustomUBO;       // 共用 binding 4

    // CPU side data
    UBOPerFrame       m_PerFrameData{};
    UBOPerObject      m_PerObjectData{};
    UBOLighting       m_LightingData{};
    UBOMaterialParams m_MaterialParamsData{};

    // Helper
    rhi::BufferHandle createUBO(uint32_t size);
    void updateUBO(rhi::BufferHandle handle, const void* data, uint32_t size);
};

// 全局访问器（Service Locator，同 getRHIDevice() 模式）
void setUBOManager(UniformBufferManager* mgr);
UniformBufferManager* getUBOManager();

} // namespace engine
