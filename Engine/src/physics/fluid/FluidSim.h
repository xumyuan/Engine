#pragma once

namespace engine {

	struct Particle 
	{
		glm::vec3 position;
		glm::vec3 velocity;
	};

	class FluidSim
	{
	public:
		FluidSim(size_t pnum,glm::vec3 min,glm::vec3 max);
		~FluidSim();

		void init();
		
		//void startSim();

	private:
		size_t m_maxParticleNum;
		size_t m_particleNum;

		float m_spacing;
		float m_restDensity = 1000.0f;
		
		// boundary
		glm::vec3 m_min, m_max;

		std::vector<Particle> m_particles;


	};

	
}
