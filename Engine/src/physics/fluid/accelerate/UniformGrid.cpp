#include "pch.h"
#include "UniformGrid.h"
#include "utils/profile/profile.h"
#include "omp.h"

namespace engine {
	void UniformGrid::insertParticles() {
		PROFILE("pbf insertParticles");

		for (auto& v : m_gridParticleIndices) {
			v.clear();
		}
		for (int i = 0; i < m_particleNum; ++i) {
			glm::vec3& pos = m_positions[i];
			int gridX = static_cast<int>((pos.x - m_boundary.min.x) / m_sphKernelRadius);
			int gridY = static_cast<int>((pos.y - m_boundary.min.y) / m_sphKernelRadius);
			int gridZ = static_cast<int>((pos.z - m_boundary.min.z) / m_sphKernelRadius);
			if (gridX < 0 || gridX >= m_gridRes.x ||
				gridY < 0 || gridY >= m_gridRes.y ||
				gridZ < 0 || gridZ >= m_gridRes.z) {
				continue; // Skip particles outside the boundary
			}
			size_t gridIndex = gridX + gridY * m_gridRes.x + gridZ * m_gridRes.x * m_gridRes.y;
			m_particleGridIndex[i] = gridIndex;
			m_gridParticleIndices[gridIndex].push_back(i);
		}
	}

	void UniformGrid::particleReorder() {
		//todo : reorder particles based on grid indices
	}

	void UniformGrid::neighborSearch() {
		PROFILE("pbf neighborSearch");

		insertParticles();

		for (auto& nlist : m_neighborList) nlist.clear();

#pragma omp parallel for
		for (int i = 0; i < m_particleNum; ++i) {
			auto& pos = m_positions[i];
			size_t pGridIndex = m_particleGridIndex[i];

			int gx = pGridIndex % m_gridRes.x;
			int gy = (pGridIndex / m_gridRes.x) % m_gridRes.y;
			int gz = pGridIndex / (m_gridRes.x * m_gridRes.y);

			for (int dz = -1; dz <= 1; dz++) {
				for (int dy = -1; dy <= 1; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						int nx = gx + dx;
						int ny = gy + dy;
						int nz = gz + dz;
						if (nx < 0 || nx >= m_gridRes.x || ny < 0 || ny >= m_gridRes.y || nz < 0 || nz >= m_gridRes.z) {
							continue; // Skip out of bounds neighbors
						}
						int neighborGridIndex = nx + ny * m_gridRes.x + nz * m_gridRes.x * m_gridRes.y;
						for (auto& j : m_gridParticleIndices[neighborGridIndex]) {
							if (i == j) continue;
							auto& nPos = m_positions[j];
							float dist2 = glm::length2(pos - nPos);
							if (dist2 < m_sphKernelRadius * m_sphKernelRadius) {
								m_neighborList[i].emplace_back(j);
							}
						}
					}
				}
			}
		}

	}
}
