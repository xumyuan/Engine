#include "pch.h"
#include "UniformBufferManager.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>

namespace engine {

UniformBufferManager::UniformBufferManager() {}

UniformBufferManager::~UniformBufferManager() {
    shutdown();
}

void UniformBufferManager::initialize(rhi::RHIDevice* device) {
    m_Device = device;
    if (!m_Device) return;

    m_PerFrameUBO      = createUBO(sizeof(UBOPerFrame));
    m_PerObjectUBO     = createUBO(sizeof(UBOPerObject));
    m_LightingUBO      = createUBO(sizeof(UBOLighting));
    m_MaterialParamsUBO = createUBO(sizeof(UBOMaterialParams));
    // Custom UBO: 使用最大可能的 size（SSAO 最大 1040 bytes）
    m_CustomUBO        = createUBO(1040);
}

void UniformBufferManager::shutdown() {
    if (!m_Device) return;

    auto safeDestroy = [this](rhi::BufferHandle& h) {
        if (static_cast<bool>(h)) {
            m_Device->destroyBuffer(h);
            h.clear();
        }
    };
    safeDestroy(m_PerFrameUBO);
    safeDestroy(m_PerObjectUBO);
    safeDestroy(m_LightingUBO);
    safeDestroy(m_MaterialParamsUBO);
    safeDestroy(m_CustomUBO);
    m_Device = nullptr;
}

rhi::BufferHandle UniformBufferManager::createUBO(uint32_t size) {
    rhi::BufferDesc desc;
    desc.usage = rhi::BufferUsage::Uniform;
    desc.size = size;
    desc.dynamic = true;
    return m_Device->createBuffer(desc);
}

void UniformBufferManager::updateUBO(rhi::BufferHandle handle, const void* data, uint32_t size) {
    rhi::BufferDataDesc bufData;
    bufData.data = data;
    bufData.size = size;
    bufData.offset = 0;
    m_Device->updateBuffer(handle, bufData);
}

void UniformBufferManager::bind(uint32_t binding, rhi::BufferHandle handle, uint32_t size) {
    m_Device->bindUniformBuffer(0, binding, handle, 0, size);
}

// ===== PerFrame =====
void UniformBufferManager::updatePerFrame(const glm::mat4& view, const glm::mat4& projection,
                                          const glm::vec3& viewPos, const glm::vec2& screenSize) {
    m_PerFrameData.view = view;
    m_PerFrameData.projection = projection;
    m_PerFrameData.viewInverse = glm::inverse(view);
    m_PerFrameData.projectionInverse = glm::inverse(projection);
    m_PerFrameData.viewPos = glm::vec4(viewPos, 1.0f);
    m_PerFrameData.screenSize = screenSize;
    if (screenSize.x > 0.0f && screenSize.y > 0.0f) {
        m_PerFrameData.texelSize = glm::vec2(1.0f / screenSize.x, 1.0f / screenSize.y);
    }
    updateUBO(m_PerFrameUBO, &m_PerFrameData, sizeof(UBOPerFrame));
}

void UniformBufferManager::bindPerFrame() {
    bind(UBOBinding::PerFrame, m_PerFrameUBO, sizeof(UBOPerFrame));
}

// ===== PerObject =====
void UniformBufferManager::updatePerObject(const glm::mat4& model, const glm::mat3& normalMatrix) {
    m_PerObjectData.model = model;
    setNormalMatrix(m_PerObjectData, normalMatrix);
    updateUBO(m_PerObjectUBO, &m_PerObjectData, sizeof(UBOPerObject));
}

void UniformBufferManager::updatePerObject(const glm::mat4& model) {
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
    updatePerObject(model, normalMatrix);
}

void UniformBufferManager::bindPerObject() {
    bind(UBOBinding::PerObject, m_PerObjectUBO, sizeof(UBOPerObject));
}

// ===== 仅更新 CPU 侧数据（不上传 GPU）=====
void UniformBufferManager::preparePerFrame(const glm::mat4& view, const glm::mat4& projection,
                                           const glm::vec3& viewPos, const glm::vec2& screenSize) {
    m_PerFrameData.view = view;
    m_PerFrameData.projection = projection;
    m_PerFrameData.viewInverse = glm::inverse(view);
    m_PerFrameData.projectionInverse = glm::inverse(projection);
    m_PerFrameData.viewPos = glm::vec4(viewPos, 1.0f);
    m_PerFrameData.screenSize = screenSize;
    if (screenSize.x > 0.0f && screenSize.y > 0.0f) {
        m_PerFrameData.texelSize = glm::vec2(1.0f / screenSize.x, 1.0f / screenSize.y);
    }
}

void UniformBufferManager::preparePerObject(const glm::mat4& model, const glm::mat3& normalMatrix) {
    m_PerObjectData.model = model;
    setNormalMatrix(m_PerObjectData, normalMatrix);
}

void UniformBufferManager::preparePerObject(const glm::mat4& model) {
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
    preparePerObject(model, normalMatrix);
}

// ===== Lighting =====
void UniformBufferManager::updateLighting() {
    updateUBO(m_LightingUBO, &m_LightingData, sizeof(UBOLighting));
}

void UniformBufferManager::bindLighting() {
    bind(UBOBinding::Lighting, m_LightingUBO, sizeof(UBOLighting));
}

// ===== MaterialParams =====
void UniformBufferManager::updateMaterialParams() {
    updateUBO(m_MaterialParamsUBO, &m_MaterialParamsData, sizeof(UBOMaterialParams));
}

void UniformBufferManager::bindMaterialParams() {
    bind(UBOBinding::MaterialParams, m_MaterialParamsUBO, sizeof(UBOMaterialParams));
}

// ===== Custom =====
void UniformBufferManager::updateSSAOParams(const UBOSSAOParams& params) {
    updateUBO(m_CustomUBO, &params, sizeof(UBOSSAOParams));
}

void UniformBufferManager::updatePostProcessParams(const UBOPostProcessParams& params) {
    updateUBO(m_CustomUBO, &params, sizeof(UBOPostProcessParams));
}

void UniformBufferManager::updateFluidParams(const UBOFluidParams& params) {
    updateUBO(m_CustomUBO, &params, sizeof(UBOFluidParams));
}

void UniformBufferManager::updateShadowmapPass(const UBOShadowmapPass& params) {
    updateUBO(m_CustomUBO, &params, sizeof(UBOShadowmapPass));
}

void UniformBufferManager::updateProbeParams(const UBOProbeParams& params) {
    updateUBO(m_CustomUBO, &params, sizeof(UBOProbeParams));
}

void UniformBufferManager::updateClipPlane(const UBOClipPlane& params) {
    updateUBO(m_CustomUBO, &params, sizeof(UBOClipPlane));
}

void UniformBufferManager::updateIBLParams(const UBOIBLParams& params) {
    updateUBO(m_CustomUBO, &params, sizeof(UBOIBLParams));
}

void UniformBufferManager::bindCustom(uint32_t size) {
    bind(UBOBinding::CustomParams, m_CustomUBO, size);
}

} // namespace engine

// ===== 全局访问器 =====
namespace engine {

static UniformBufferManager* gUBOManager = nullptr;

void setUBOManager(UniformBufferManager* mgr) {
    gUBOManager = mgr;
}

UniformBufferManager* getUBOManager() {
    return gUBOManager;
}

} // namespace engine
