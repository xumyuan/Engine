#include "pch.h"
#include "LightMarkerRenderer.h"

#include "graphics/UniformBufferManager.h"
#include "utils/loaders/ShaderLoader.h"
#include <algorithm>

namespace engine {

LightMarkerRenderer::LightMarkerRenderer()
    : m_Shader(ShaderLoader::loadShader("Source/Shaders/light_marker.glsl")) {}

void LightMarkerRenderer::draw(rhi::CommandBuffer& cmd, bool multisampled) {
    auto* uniforms = getUBOManager();
    if (!uniforms || !m_Shader || !m_Shader->getProgramHandle()) return;

    const auto& lights = uniforms->getLightingDataConst();
    const int pointCount = std::clamp(lights.numDirPointSpotLights.y, 0, MAX_POINT_LIGHTS);
    const int spotCount = std::clamp(lights.numDirPointSpotLights.z, 0, MAX_SPOT_LIGHTS);
    if (pointCount + spotCount == 0) return;

    cmd.pushDebugGroup("Light Markers");
    rhi::PipelineState pipeline;
    pipeline.program = m_Shader->getProgramHandle();
    pipeline.depthTest = true;
    pipeline.depthWrite = false;
    pipeline.cullMode = rhi::CullMode::Back;
    pipeline.multisample = multisampled;
    cmd.bindPipeline(pipeline);

    cmd.bindUBO(UBOBinding::PerFrame, uniforms->getPerFrameHandle(), sizeof(UBOPerFrame));
    cmd.bindUBO(UBOBinding::Lighting, uniforms->getLightingHandle(), sizeof(UBOLighting));
    // Radius and display brightness are independent of the light's attenuation/intensity.
    const glm::vec4 markerSettings(0.5f, 1.5f, 0.0f, 0.0f);
    cmd.updateBuffer(uniforms->getCustomHandle(), &markerSettings, sizeof(markerSettings));
    cmd.bindUBO(UBOBinding::CustomParams, uniforms->getCustomHandle(), sizeof(markerSettings));
    m_Sphere.Draw(cmd, static_cast<uint32_t>(pointCount + spotCount));
    cmd.popDebugGroup();
}

}
