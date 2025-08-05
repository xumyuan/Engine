#pragma once

#include "platform/OpenGL/IndexBuffer.h"
#include "platform/OpenGL/VertexArray.h"
#include "platform/OpenGL/Buffer.h"
#include "graphics/shader.h"
#include "graphics/renderer/GLCache.h"
#include <mutex>

namespace engine {

	class FPSCamera;
	class PBF;

	class FluidSim
	{
	public:
		FluidSim(size_t pnum, glm::vec3 min, glm::vec3 max);
		~FluidSim();

		std::vector<glm::vec3>& getPositions() { return m_positions; }
		std::vector<glm::vec3>& getVelocities() { return m_velocities; }

		glm::vec3& getMin() { return m_min; }
		glm::vec3& getMax() { return m_max; }

		float getSpacing() const { return m_spacing; }
		float getSphKernelRadius() const { return m_sphKernelRadius; }
		float getmass() const { return m_mass; }
		float getRestDensity() const { return m_restDensity; }

		void init();

		void drawParticle(FPSCamera* camera);

		void startSim();
		void subPosData();
		size_t getParticleNum() const { return m_particleNum; }
	private:
		GLCache* m_GLCache;

		size_t m_maxParticleNum;
		size_t m_particleNum;
		// simulation parameters
		float m_spacing;
		float m_sphKernelRadius;
		float m_restDensity = 1000.0f;
		float m_mass;

		// boundary
		glm::vec3 m_min, m_max;

		// particle attribute
		std::vector<glm::vec3> m_positions;
		std::vector<glm::vec3> m_velocities;

		// buffer
		VertexArray m_fluidVAO;
		Buffer	m_fluidVBO;
		// shader
		Shader* m_particleShader;

		// solver
		PBF* m_pbf = nullptr;

		// mutex
		std::condition_variable m_condVar;
		bool dataReady = false;

	public:
		// constants
		const glm::vec3 m_gravity = glm::vec3(0.0f, -9.81f, 0.0f);
	};


}
