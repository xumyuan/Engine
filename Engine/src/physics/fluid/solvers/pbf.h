#pragma once

#include "physics/fluid/FluidSim.h"
namespace engine
{
	class PBF {
	public:
		PBF(FluidSim* fluidSim, size_t iterations = 3, float dt = 0.016f) :
			m_fluidSim(fluidSim), m_iterations(iterations), m_dt(dt) {
			m_particleNum = fluidSim->getParticleNum();
			m_deltaP.resize(m_particleNum, glm::vec3(0.0f));
			m_lambda.resize(m_particleNum, 0.0f);
			m_tempPositions.resize(m_particleNum, glm::vec3(0.0f));
		}
		~PBF() {

		}
		void solve();

		std::mutex& getPosMutex() { return m_posMutex; }
	private:
		void predictAdvect();
		void computeLambda();
		void computeDeltaP();
		void neighborSearch();
		void updatePosAndVel();
	private:
		FluidSim* m_fluidSim;
		size_t m_iterations;

		size_t m_particleNum;

		float m_dt = 0.01f; // time step

		std::vector<glm::vec3> m_deltaP;
		std::vector<float> m_lambda;
		std::vector<glm::vec3> m_tempPositions;

		std::mutex m_posMutex;
	};
}

