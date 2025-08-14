#pragma once

#include "physics/fluid/FluidSim.h"
#include "physics/fluid/accelerate/UniformGrid.h"
#include <CompactNSearch.h>

#define USE_UNIFORM_GRID 0

namespace engine
{
	class PBF {
	public:
		PBF(FluidSim* fluidSim, size_t iterations = 3, float dt = 0.016f);
		~PBF() {

		}
		void solve();

		std::mutex& getPosMutex() { return m_posMutex; }
	private:
		void neighborSearch();
		void predictAdvect();
		void computeLambda();
		void computeDeltaP();
		void applyXSPHViscosity();
		void updatePosAndVel();
	private:
		// fluid simulation parameters
		FluidSim* m_fluidSim;
		size_t m_particleNum;
		float m_dt = 0.016f; // time step
		SimParams& m_simParams;
		std::vector<std::vector<size_t>>& m_neighborList;	// neighbor list from FluidSim
		std::vector<glm::vec3>& m_positions;				// particle positions
		std::vector<glm::vec3>& m_velocities;				// particle velocities


		// accelerate structure
#if USE_UNIFORM_GRID
		UniformGrid* m_uniformGrid = nullptr;	// uniform grid acceleration
#else
		CompactNSearch::NeighborhoodSearch* m_nsearch;			// from SPlisHSPlasH
#endif // USE_UNIFORM_GRID
		// accelerate structure

		// pbf parameters
		size_t m_iterations;
		std::vector<glm::vec3> m_deltaP;
		std::vector<float> m_lambda;
		std::vector<glm::vec3> m_tempPositions;
		std::vector<glm::vec3> m_newVel;

		// thread safety
		std::mutex m_posMutex;
	};
}

