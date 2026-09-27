#include "pch.h"
#include "SPHKernel.h"
#include "FluidSim.h"
#include "solvers/PBF.h"

#include <gl/glew.h>

#include <algorithm>

#include <utils/loaders/ShaderLoader.h>
#include <graphics/camera/FPSCamera.h>
#include "rhi/include/RHIContext.h"
#include "graphics/UniformBufferManager.h"

namespace engine {

	FluidSim::FluidSim(size_t pnum, Boundary boundary) :m_maxParticleNum(pnum)
	{
		m_Device = getRHIDevice();
		m_particleShader = ShaderLoader::loadShader("Shaders/fluid/particle_draw.glsl");

		// ── 创建 dynamic Vertex Buffer ──
		rhi::BufferDesc vbDesc;
		vbDesc.usage = rhi::BufferUsage::Vertex;
		vbDesc.size = static_cast<uint32_t>(m_maxParticleNum * 3 * sizeof(float));
		vbDesc.dynamic = true;
		m_VertexBuffer = m_Device->createBuffer(vbDesc);

		// ── 构建 VertexLayout ──
		rhi::VertexLayout layout = {};
		layout.attributes[0].bufferIndex = 0;
		layout.attributes[0].offset = 0;
		layout.attributes[0].type = rhi::VertexAttribType::Float3;
		layout.strides[0] = sizeof(glm::vec3);
		layout.attributeCount = 1;
		layout.bufferCount = 1;

		// ── 创建 RenderPrimitive (无索引) ──
		rhi::BufferHandle vbs[] = { m_VertexBuffer };
		m_RenderPrimitive = m_Device->createRenderPrimitive(
			layout, vbs, 1, rhi::BufferHandle(), rhi::IndexType::UInt32);

		m_simParams.spacing = 1.0f; // 粒子间距
		m_simParams.sphRadius = 3.0f * m_simParams.spacing; // SPH核半径
		m_simParams.restDensity = 1000.0f; // 静止密度
		m_simParams.invRestDensity = 1.0f / m_simParams.restDensity; // 静止密度的倒数
		m_simParams.boundary = boundary; // 模拟边界
		m_simParams.mass = m_simParams.restDensity * glm::pow(m_simParams.spacing, 3.0f); // 粒子质量
		m_simParams.gravity = glm::vec3(0.0f, -9.81f, 0.0f); // 重力加速度
		m_simParams.viscosity = 0.01f; // 粘性系数
		m_simParams.dt = 0.01f; // 时间步长
		m_simParams.invDt = 1.0f / m_simParams.dt; // 时间步长的倒数
		m_simParams.volume = glm::pow(m_simParams.spacing, 3.0f); // 体积

		Poly6Kernel::setRadius(m_simParams.sphRadius);
		SpikyKernel::setRadius(m_simParams.sphRadius);

		m_positions = std::vector<glm::vec3>(pnum);

		m_particleNum = 0;
		init();
		m_velocities.resize(m_particleNum, glm::vec3(0.0f));
		m_neighborList.resize(m_particleNum);

		m_pbf = new PBF(this);
	}

	FluidSim::~FluidSim()
	{
		stopSimulation();
		delete m_pbf;
		m_pbf = nullptr;

		if (m_Device) {
			if (static_cast<bool>(m_RenderPrimitive))
				m_Device->destroyRenderPrimitive(m_RenderPrimitive);
			if (static_cast<bool>(m_VertexBuffer))
				m_Device->destroyBuffer(m_VertexBuffer);
		}
	}

	void FluidSim::init()
	{
		auto& boundary = m_simParams.boundary;
		glm::vec3& max = boundary.max;
		glm::vec3& min = boundary.min;
		float& spacing = m_simParams.spacing;

		glm::vec3 delta = max - min;
		int cntx, cntz;

		cntx = (int)std::ceil(delta.x / spacing);
		cntz = (int)std::ceil(delta.z / spacing) / 3;

		int cnt = cntx * cntz;

		glm::vec3 pos;

		// 对粒子初始位置的随机扰动
		static std::mt19937 rng(std::random_device{}());
		std::uniform_real_distribution<float> dist(-0.2f, 0.2f);

		bool shouldBreak = false;
		for (pos.y = min.y + 0.4f; pos.y < max.y; pos.y += spacing) {
			for (int xz = 0; xz < cnt; xz++) {
				float dx = dist(rng);
				float dz = dist(rng);


				pos.x = min.x + 1.0f + (xz % int(cntx)) * spacing + dx;
				pos.z = min.z + 1.0f + (xz / int(cntx)) * spacing + dz;

				m_positions[m_particleNum++] = pos;
				if (m_particleNum >= m_maxParticleNum) {
					shouldBreak = true;
					break;
				}
			}
			if (shouldBreak) break;
		}

		m_renderPositions.assign(m_positions.begin(), m_positions.begin() + m_particleNum);
		subPosData();
	}

	void FluidSim::subPosData() {
		if (!m_Device) return;

		rhi::BufferDataDesc bufData;
		bufData.data = m_renderPositions.data();
		bufData.size = static_cast<uint32_t>(m_particleNum * sizeof(glm::vec3));
		bufData.offset = 0;
		m_Device->updateBuffer(m_VertexBuffer, bufData);
	}

	void FluidSim::drawParticle(rhi::CommandBuffer& cmd, FPSCamera* camera) {
		// 不等待模拟线程：有新结果就上传，否则沿用上一帧
		{
			std::lock_guard<std::mutex> lock(m_pbf->getPosMutex());
			if (dataReady) {
				subPosData();
				dataReady = false;
			}
		}

		float fov = camera->getFOV();
		glm::vec3 waterPos(0.0);
		glm::mat4 modelMat = glm::translate(glm::mat4(1), waterPos);

		const glm::vec3 lightPos = { 0.0f,1000.f,0.f };
		const glm::vec3 lightColor = { 1.f,1.f,1.f };
		glm::vec3 objectColor = { 0.267, 0.447, 0.769 };

		float pointScale = 1.0f * 768.f / glm::tan(glm::radians(fov) * 0.5f);
		float pointSize = 0.5f;

		rhi::ProgramHandle program = m_particleShader->getProgramHandle();

		rhi::PipelineState pipeline;
		pipeline.program = program;
		pipeline.depthTest = true;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd.bindPipeline(pipeline);

		// PerFrame（view / projection / viewPos）由调用方的 pass 写入
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->preparePerObject(modelMat);
			cmd.updateBuffer(uboMgr->getPerObjectHandle(), &uboMgr->getPerObjectData(), sizeof(UBOPerObject));
			cmd.bindUBO(UBOBinding::PerObject, uboMgr->getPerObjectHandle(), sizeof(UBOPerObject));

			UBOFluidParams fluidParams{};
			fluidParams.lightPos = glm::vec4(lightPos, pointScale);
			fluidParams.lightColor = glm::vec4(lightColor, pointSize);
			fluidParams.objectColor = glm::vec4(objectColor, 0.0f);
			cmd.updateBuffer(uboMgr->getCustomHandle(), &fluidParams, sizeof(UBOFluidParams));
			cmd.bindUBO(UBOBinding::CustomParams, uboMgr->getCustomHandle(), sizeof(UBOFluidParams));
		}

		// 注意：GL_PROGRAM_POINT_SIZE 是 GL 特有状态，暂时保留直接调用
		glEnable(GL_PROGRAM_POINT_SIZE);
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

		cmd.bindRenderPrimitive(m_RenderPrimitive);
		cmd.drawArrays(rhi::PrimitiveType::Points,
			static_cast<uint32_t>(m_particleNum));

		glDisable(GL_PROGRAM_POINT_SIZE);
		glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
	}

	void FluidSim::startSimulation() {
		if (m_simThread.joinable()) return;
		m_stopRequested = false;
		m_simThread = std::thread([this] { simulationLoop(); });
	}

	void FluidSim::stopSimulation() {
		if (!m_simThread.joinable()) return;
		m_stopRequested = true;
		m_simThread.join();
	}

	void FluidSim::simulationLoop() {
		while (!m_stopRequested) {
			m_pbf->solve();
			std::lock_guard<std::mutex> lock(m_pbf->getPosMutex());
			std::copy_n(m_positions.begin(), m_particleNum, m_renderPositions.begin());
			dataReady = true;
		}
	}


}
