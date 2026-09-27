#include "ComputePBF.h"
#include "physics/fluid/FluidSim.h"
#include "rhi/include/RHIShaderCompiler.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace engine {

ComputePBF::~ComputePBF() {
    for (size_t i = 1; i < m_buffers.size(); ++i) {
        if (m_buffers[i]) m_device.destroyBuffer(m_buffers[i]);
    }
    if (m_uniforms) m_device.destroyBuffer(m_uniforms);
}

bool ComputePBF::initialize(rhi::BufferHandle positions, const std::vector<glm::vec3>& velocities,
                            const SimParams& params) {
    const auto limits = m_device.getComputeLimits();
    const uint64_t maxElements = std::min<uint64_t>(
        uint64_t(limits.maxGroupCountX) * 128,
        std::min<uint64_t>(limits.maxStorageBufferSize, std::numeric_limits<uint32_t>::max()) / 16);
    if (!positions || velocities.empty() || velocities.size() > maxElements ||
        !(params.dt > 0.0f) || !std::isfinite(params.dt) ||
        !(params.sphRadius > 0.0f) || !std::isfinite(params.sphRadius)) return false;

    glm::uvec3 grid(1);
    uint64_t cellCount = 1;
    for (int axis = 0; axis < 3; ++axis) {
        const double cells = std::ceil(double(params.boundary.max[axis] - params.boundary.min[axis]) / params.sphRadius);
        if (!std::isfinite(cells) || cells < 1 || cells > double(maxElements)) return false;
        grid[axis] = static_cast<uint32_t>(cells);
        cellCount *= grid[axis];
        if (cellCount > maxElements) return false;
    }

    auto compiler = m_device.createShaderCompiler();
    m_program = compiler->loadAndCompile(std::string(PROJECT_ROOT_DIR) + "/Shaders/fluid/pbf_compute.glsl");
    if (!m_program || !m_device.waitProgram(m_program->getProgramHandle())) return false;

    m_params.counts = glm::uvec4(static_cast<uint32_t>(velocities.size()), 0, static_cast<uint32_t>(cellCount), 0);
    m_params.grid = glm::uvec4(grid, 0);
    m_params.boundaryMin = glm::vec4(params.boundary.min, 0);
    m_params.boundaryMax = glm::vec4(params.boundary.max, 0);
    m_params.gravityDt = glm::vec4(params.gravity, params.dt);
    m_params.fluid = glm::vec4(params.sphRadius, params.mass * params.invRestDensity,
                             params.viscosity, params.spacing * 0.5f);

    const uint32_t n = m_params.counts.x;
    const uint32_t sizes[] = { 0, n * 16, n * 16, n * 16, n * 4, m_params.counts.z * 4, n * 4 };
    m_buffers[0] = positions;
    for (size_t i = 1; i < m_buffers.size(); ++i) {
        m_buffers[i] = m_device.createBuffer({ rhi::BufferUsage::Storage, sizes[i], true });
        if (!m_buffers[i]) return false;
    }
    m_uniforms = m_device.createBuffer({ rhi::BufferUsage::Uniform, sizeof(Parameters), true });
    if (!m_uniforms) return false;

    std::vector<glm::vec4> initialVelocity;
    initialVelocity.reserve(n);
    for (const auto& v : velocities) initialVelocity.emplace_back(v, 0.0f);
    rhi::BufferDataDesc data;
    data.data = initialVelocity.data();
    data.size = n * sizeof(glm::vec4);
    data.offset = 0;
    m_device.updateBuffer(m_buffers[2], data);
    return true;
}

void ComputePBF::dispatch(rhi::CommandBuffer& cmd, uint32_t phase, uint32_t count) {
    m_params.counts.y = phase;
    cmd.updateBuffer(m_uniforms, &m_params, sizeof(m_params));
    cmd.dispatchCompute((count + 127) / 128);
    cmd.memoryBarrier(rhi::MemoryBarrier::ShaderStorage);
}

void ComputePBF::buildGrid(rhi::CommandBuffer& cmd) {
    dispatch(cmd, 1, m_params.counts.z);
    dispatch(cmd, 2, m_params.counts.x);
}

void ComputePBF::step(rhi::CommandBuffer& cmd) {
    cmd.pushDebugGroup("Compute PBF");
    cmd.bindComputeProgram(m_program->getProgramHandle());
    cmd.bindUBO(0, m_uniforms, sizeof(m_params));
    for (uint32_t i = 0; i < m_buffers.size(); ++i) cmd.bindStorageBuffer(i, m_buffers[i]);
    const uint32_t count = m_params.counts.x;
    dispatch(cmd, 0, count);
    for (int iteration = 0; iteration < 3; ++iteration) {
        buildGrid(cmd);
        dispatch(cmd, 3, count);
        dispatch(cmd, 4, count);
        dispatch(cmd, 5, count);
    }
    buildGrid(cmd);
    dispatch(cmd, 6, count);
    dispatch(cmd, 7, count);
    dispatch(cmd, 8, count);
    cmd.memoryBarrier(rhi::MemoryBarrier::ShaderStorage | rhi::MemoryBarrier::VertexAttribute);
    cmd.popDebugGroup();
}

}
