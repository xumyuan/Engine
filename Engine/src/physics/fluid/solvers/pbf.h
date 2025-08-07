#pragma once

#include "physics/fluid/FluidSim.h"
#include "physics/fluid/accelerate/UniformGrid.h"

namespace engine
{
	class PBF {
	public:
		PBF(FluidSim* fluidSim, size_t iterations = 3, float dt = 0.016f) :
			m_fluidSim(fluidSim), m_iterations(iterations), m_dt(dt),
			m_neighborList(fluidSim->getNeighborList()),
			m_simParams(fluidSim->getSimParams()){

			m_particleNum = fluidSim->getParticleNum();
			m_deltaP.resize(m_particleNum, glm::vec3(0.0f));
			m_lambda.resize(m_particleNum, 0.0f);
			m_tempPositions.resize(m_particleNum, glm::vec3(0.0f));

			m_uniformGrid = new UniformGrid(fluidSim->getSpacing(), fluidSim->getSphKernelRadius(),
				{ fluidSim->getMin(), fluidSim->getMax() }, fluidSim);
		}
		~PBF() {

		}
		void solve();

		std::mutex& getPosMutex() { return m_posMutex; }
	private:
		void predictAdvect();
		void computeLambda();
		void computeDeltaP();
		void updatePosAndVel();
	private:
		FluidSim* m_fluidSim;
		size_t m_iterations;

		UniformGrid* m_uniformGrid = nullptr;

		size_t m_particleNum;

		float m_dt = 0.016f; // time step

		std::vector<glm::vec3> m_deltaP;
		std::vector<float> m_lambda;
		std::vector<glm::vec3> m_tempPositions;

		std::mutex m_posMutex;

		SimParams& m_simParams;

		// 从 FluidSim 中获取的邻居列表
		std::vector<std::vector<size_t>>& m_neighborList;
	};
}

