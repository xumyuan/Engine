#pragma once

#include "platform/OpenGL/IndexBuffer.h"
#include "platform/OpenGL/VertexArray.h"
#include "platform/OpenGL/Buffer.h"
#include "graphics/shader.h"
#include "graphics/renderer/GLCache.h"

namespace engine {

	class FPSCamera;

	class FluidSim
	{
	public:
		FluidSim(size_t pnum,glm::vec3 min,glm::vec3 max);
		~FluidSim();

		std::vector<glm::vec3>& getPositions() { return m_positions; }
		void init();

		void drawParticle(FPSCamera* camera);
		
		//void startSim();

	private:
		GLCache* m_GLCache;

		size_t m_maxParticleNum;
		size_t m_particleNum;

		float m_spacing;
		float m_restDensity = 1000.0f;
		
		// boundary
		glm::vec3 m_min, m_max;

		// particle attribute
		std::vector<glm::vec3> m_positions;

		// buffer
		VertexArray m_fluidVAO;
		Buffer	m_fluidVBO;
		// shader
		Shader* m_particleShader;
	};

	
}
