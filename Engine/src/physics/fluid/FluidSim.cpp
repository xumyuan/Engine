#include "pch.h"
#include "SPHKernel.h"
#include "FluidSim.h"
#include "solvers/pbf.h"

#include <utils/loaders/ShaderLoader.h>
#include <graphics/camera/FPSCamera.h>

namespace engine {

	FluidSim::FluidSim(size_t pnum, Boundary boundary) :m_maxParticleNum(pnum)
	{
		m_particleShader = ShaderLoader::loadShader("src/shaders/fluid/particle_draw.glsl");
		m_GLCache = GLCache::getInstance();

		m_fluidVBO.load(nullptr, m_maxParticleNum * 3, 3, DrawType::Dynamic);
		m_fluidVAO.addBuffer(&m_fluidVBO, 0, sizeof(glm::vec3));

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

		m_fluidVBO.subData((float*)m_positions.data());
	}

	void FluidSim::subPosData() {
		m_fluidVBO.subData((float*)m_positions.data());
	}

	void FluidSim::drawParticle(FPSCamera* camera) {
		{
			std::unique_lock<std::mutex> lock(m_pbf->getPosMutex());
			if (m_condVar.wait_for(lock, std::chrono::milliseconds(1), [this] { return dataReady; })) {
				subPosData();
				dataReady = false;
			}
		}


		float aspect = static_cast<float>(WINDOW_X_RESOLUTION) / WINDOW_Y_RESOLUTION;
		float fov = camera->getFOV();
		glm::vec3 cameraPos = camera->getPosition();
		glm::mat4 projectMat = camera->getProjectionMatrix();

		glm::vec3 waterPos(0.0);

		glm::mat4 modelMat = glm::translate(glm::mat4(1), waterPos);

		const glm::vec3 lightPos = { 0.0f,1000.f,0.f };
		const glm::vec3 lightColor = { 1.f,1.f,1.f };
		glm::vec3 objectColor = { 0.267, 0.447, 0.769 };

		float pointScale = 1.0f * 768.f / glm::tan(glm::radians(fov) * 0.5f);
		float pointSize = 0.5f;

		m_GLCache->switchShader(m_particleShader);

		m_particleShader->setUniform("pointScale", pointScale);
		m_particleShader->setUniform("pointSize", pointSize);
		m_particleShader->setUniform("lightPos", lightPos);
		m_particleShader->setUniform("lightColor", lightColor);
		m_particleShader->setUniform("objectColor", objectColor);

		m_particleShader->setUniform("projection", projectMat);
		m_particleShader->setUniform("model", modelMat);
		m_particleShader->setUniform("view", camera->getViewMatrix());
		m_particleShader->setUniform("viewPos", cameraPos);

		m_GLCache->setDepthTest(true);

		glEnable(GL_PROGRAM_POINT_SIZE);
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
		//glEnable(GL_POINT_SPRITE);

		m_fluidVAO.bind();
		glDrawArrays(GL_POINTS, 0, m_particleNum);
		m_fluidVAO.unbind();

		glDisable(GL_PROGRAM_POINT_SIZE);
		glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
		//glDisable(GL_POINT_SPRITE);
	}

	void FluidSim::startSim() {
		while (true) {
			m_pbf->solve();
			{
				std::lock_guard<std::mutex> lock(m_pbf->getPosMutex());
				dataReady = true;
			}
			m_condVar.notify_all();
		}

	}


}
