#pragma once

#include "rhi/include/RHICommandBuffer.h"
#include "rhi/include/RHIDevice.h"
#include "rhi/include/RHIShaderProgram.h"
#include <array>
#include <memory>
#include <vector>

namespace engine {

struct SimParams;

// All state stays on the GPU. The packed position buffer is also the render VBO.
class ComputePBF {
public:
    explicit ComputePBF(rhi::RHIDevice& device) : m_device(device) {}
    ~ComputePBF();
    ComputePBF(const ComputePBF&) = delete;
    ComputePBF& operator=(const ComputePBF&) = delete;

    bool initialize(rhi::BufferHandle positions, const std::vector<glm::vec3>& velocities,
                    const SimParams& params);
    void step(rhi::CommandBuffer& cmd);

private:
    // Matches std140 in pbf_compute.glsl; only 16-byte fields cross the API.
    struct alignas(16) Parameters {
        glm::uvec4 counts; // particles, phase, cells, unused
        glm::uvec4 grid;
        glm::vec4 boundaryMin;
        glm::vec4 boundaryMax;
        glm::vec4 gravityDt;
        glm::vec4 fluid; // radius, mass/restDensity, viscosity, particle radius
    } m_params{};
    static_assert(sizeof(Parameters) == 96);

    void dispatch(rhi::CommandBuffer& cmd, uint32_t phase, uint32_t count);
    void buildGrid(rhi::CommandBuffer& cmd);
    rhi::RHIDevice& m_device;
    std::unique_ptr<rhi::RHIShaderProgram> m_program;
    // Slot zero is borrowed from FluidSim; the remaining buffers are owned here.
    std::array<rhi::BufferHandle, 7> m_buffers{};
    rhi::BufferHandle m_uniforms;
};

}
