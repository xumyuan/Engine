#pragma once
#include "physics/fluid/FluidSim.h"

namespace engine {

	struct GridResolution {
		int x;
		int y;
		int z;
	};

	class UniformGrid {
	public:
		UniformGrid(float spacing, float sphRadius, Boundary boundary, FluidSim* fluidSim) :
			m_spacing(spacing), m_sphKernelRadius(sphRadius), m_boundary(boundary),
			m_positions(fluidSim->getPositions()),
			m_particleNum(fluidSim->getParticleNum()),
			m_velocities(fluidSim->getVelocities()),
			m_neighborList(fluidSim->getNeighborList()) {

			m_gridRes.x = static_cast<int>(std::ceil((m_boundary.max.x - m_boundary.min.x) / m_sphKernelRadius));
			m_gridRes.y = static_cast<int>(std::ceil((m_boundary.max.y - m_boundary.min.y) / m_sphKernelRadius));
			m_gridRes.z = static_cast<int>(std::ceil((m_boundary.max.z - m_boundary.min.z) / m_sphKernelRadius));

			m_gridCellCount = m_gridRes.x * m_gridRes.y * m_gridRes.z;

			m_fluidSim = fluidSim;

			m_gridParticleIndices.resize(m_gridCellCount);
			m_particleGridIndex.resize(fluidSim->getParticleNum(), -1);

			numThread = std::thread::hardware_concurrency();
		}

		void insertParticles();

		void particleReorder();

		void neighborSearch();

		~UniformGrid() = default;
	public:
		int numThread = 0;
	private:
		Boundary m_boundary;

		float m_spacing;
		float m_sphKernelRadius;

		GridResolution m_gridRes;

		// 每个网格单元的粒子索引
		std::vector<std::vector<size_t>> m_gridParticleIndices;
		// 每个粒子所在的网格单元索引
		std::vector<size_t> m_particleGridIndex;

		size_t m_gridCellCount = 0;

		std::vector<glm::vec3> m_tempPositions;

		FluidSim* m_fluidSim;

		// 从fluidsim获取的粒子数据
		std::vector<glm::vec3>& m_positions;
		std::vector<glm::vec3>& m_velocities;
		size_t m_particleNum;
		std::vector<std::vector<size_t>>& m_neighborList;
	};
}
