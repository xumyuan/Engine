#include "pch.h"
#include "FluidSim.h"
#include "solvers/pbf.h"

#include <utils/loaders/ShaderLoader.h>
#include <graphics/camera/FPSCamera.h>

namespace engine {

	FluidSim::FluidSim(size_t pnum, glm::vec3 min, glm::vec3 max) :m_maxParticleNum(pnum), m_min(min), m_max(max)
	{
		m_particleShader = ShaderLoader::loadShader("src/shaders/fluid/particle_draw.glsl");
		m_GLCache = GLCache::getInstance();

		m_fluidVBO.load(nullptr, m_maxParticleNum * 3, 3, DrawType::Dynamic);
		m_fluidVAO.addBuffer(&m_fluidVBO, 0, sizeof(glm::vec3));

		m_particleNum = 0;

		m_spacing = 1.0f;
		m_sphKernelRadius = 3 * m_spacing;

		m_positions = std::vector<glm::vec3>(pnum);

		m_mass = m_restDensity * glm::pow(m_spacing, 3.0f);

		init();
		m_velocities.resize(m_particleNum, glm::vec3(0.0f));
		m_pbf = new PBF(this);
	}

	FluidSim::~FluidSim()
	{
	}

	void FluidSim::init()
	{
		glm::vec3& max = m_max;
		glm::vec3& min = m_min;
		float& spacing = m_spacing;

		glm::vec3 delta = max - min;
		int cntx, cntz;

		cntx = (int)std::ceil(delta.x / spacing);
		cntz = (int)std::ceil(delta.z / spacing);

		int cnt = cntx * cntz;

		glm::vec3 pos;

		bool shouldBreak = false;
		for (pos.y = min.y; pos.y < max.y; pos.y += spacing) {
			for (int xz = 0; xz < cnt; xz++) {
				pos.x = min.x + (xz % int(cntx)) * spacing;
				pos.z = min.z + (xz / int(cntx)) * spacing;

				m_positions[m_particleNum++] = pos;
				if (m_particleNum >= m_maxParticleNum) {
					shouldBreak = true;
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
