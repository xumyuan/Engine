#include "pch.h"
#include "FluidSim.h"

namespace engine {

	FluidSim::FluidSim(size_t pnum, glm::vec3 min, glm::vec3 max):m_maxParticleNum(pnum),m_min(min),m_max(max)
	{
		m_particleNum = 0;
		init();

		m_particles = std::vector<Particle>(pnum);
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

		size_t idx = 0;
		for (pos.y = min.y; pos.y < max.y; pos.y += spacing) {
			for (int xz = 0; xz < cnt; xz++) {
				pos.x = min.x + (xz % int(cntx)) * spacing;
				pos.z = min.z + (xz / int(cntx)) * spacing;

				Particle p;
				p.position = pos;

				m_particles[idx++] = p;
			}
		}
	}

	

}
