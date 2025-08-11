#include "pch.h"
#include "utils/profile/profile.h"
#include "physics/fluid/SPHKernel.h"
#include "omp.h"
#include "pbf.h"
#include "utils/debug_macro.h"

#define UNIFORM_GRID 1

namespace engine {

	const float ploy6_kernel = 315.0f / (64.0f * glm::pi<float>() * glm::pow(3.0f, 9.0f));
	const float spiky_kernel = -45.0f / (glm::pi<float>() * glm::pow(3.0f, 6.0f));
	const float delta_p = 0.1 * 3.0f;
	const float wpoly_con_deltaP = pow(9.0 - delta_p * delta_p, 3) * ploy6_kernel;
	const float pressure_k = 0.1f; // pressure constant, can be adjusted
	const float pressure_n = 4.0f;

	void PBF::solve() {
		PROFILE("pbf solve");
		predictAdvect();
		DEBUG_LINE(spdlog::info("predictAdvect over"));
		size_t iter = 0;

		m_uniformGrid->neighborSearch();

		while (iter++ < m_iterations) {
			computeLambda();
			computeDeltaP();
		}
		DEBUG_LINE(spdlog::info("iterate over"));
		updatePosAndVel();

		applyXSPHViscosity();

		DEBUG_LINE(spdlog::info("updatePosAndVel over"));
	}

	void PBF::predictAdvect() {
		PROFILE("pbf predictAdvect");
#pragma omp parallel for
		for (int i = 0; i < m_particleNum; ++i) {
			auto& position = m_fluidSim->getPositions()[i];
			glm::vec3 vel = m_fluidSim->getVelocities()[i] + m_dt * m_simParams.gravity;
			m_tempPositions[i] = position + vel * m_dt;

			//confine the particle to the boundary
			float spacing = m_fluidSim->getSpacing();

			glm::vec3& min = m_fluidSim->getMin();
			glm::vec3& max = m_fluidSim->getMax();

			glm::vec3 moveDir = m_tempPositions[i] - position;
			moveDir *= 0.5f;

			if (m_tempPositions[i].x < min.x + spacing * 0.5f) {
				m_tempPositions[i].x = min.x + spacing * 0.5f;
				moveDir.x = -moveDir.x;
				m_tempPositions[i].x += moveDir.x;
			}
			else if (m_tempPositions[i].x > max.x - spacing * 0.5f) {
				m_tempPositions[i].x = max.x - spacing * 0.5f;
				moveDir.x = -moveDir.x;
				m_tempPositions[i].x += moveDir.x;
			}

			if (m_tempPositions[i].y < min.y + spacing * 0.5f) {
				m_tempPositions[i].y = min.y + spacing * 0.5f;
				moveDir.y = -moveDir.y;
				m_tempPositions[i].y += moveDir.y;
			}
			else if (m_tempPositions[i].y > max.y - spacing * 0.5f) {
				m_tempPositions[i].y = max.y - spacing * 0.5f;
				moveDir.y = -moveDir.y;
				m_tempPositions[i].y += moveDir.y;
			}

			if (m_tempPositions[i].z < min.z + spacing * 0.5f) {
				m_tempPositions[i].z = min.z + spacing * 0.5f;
				moveDir.z = -moveDir.z;
				m_tempPositions[i].z += moveDir.z;
			}
			else if (m_tempPositions[i].z > max.z - spacing * 0.5f) {
				m_tempPositions[i].z = max.z - spacing * 0.5f;
				moveDir.z = -moveDir.z;
				m_tempPositions[i].z += moveDir.z;
			}
		}
	}

	void PBF::computeLambda() {
		PROFILE("pbf computeLambda");
#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < m_particleNum; ++i) {
			auto& pos = m_tempPositions[i];
			float& lambda = m_lambda[i];
			lambda = 0.0f;
			glm::vec3 grad_pi_C = glm::vec3(0.0f);
			float grad_C_sum = 0.0f;
			float density = Poly6Kernel::m_W_zero;
			float mass = m_simParams.mass;
			float restDensity = m_simParams.restDensity;

			auto& curParticleNeighbors = m_neighborList[i];

			for (auto j : curParticleNeighbors) {
				if (i == j) continue;
				auto& pos_j = m_tempPositions[j];
				glm::vec3 diff = pos - pos_j;
				float dist = glm::length(diff);
				float sphRadius = m_simParams.sphRadius;

				if (dist < sphRadius) {

					density += Poly6Kernel::W(dist);
					glm::vec3 grad_pj_C = SpikyKernel::gradW(diff) * mass / restDensity;
					grad_pi_C -= grad_pj_C;
					grad_C_sum += glm::length2(grad_pj_C);
				}
			}
			density *= mass;
			float constraint = density / restDensity - 1;
			float grad_i_sum = glm::length2(grad_pi_C);
			lambda = -constraint / (grad_i_sum + grad_C_sum + 1000.0f);
		}
	}

	void PBF::computeDeltaP() {
		PROFILE("pbf computeDeltaP");

#pragma omp parallel for
		for (int i = 0; i < m_particleNum; ++i) {
			auto& pos = m_tempPositions[i];
			auto& deltaP = m_deltaP[i];
			deltaP = glm::vec3(0.0f);

			auto& curParticleNeighbors = m_neighborList[i];
			for (auto& j : curParticleNeighbors) {
				if (i == j) continue;

				auto& pos_j = m_tempPositions[j];
				glm::vec3 diff = pos - pos_j;
				float dsq = glm::length2(diff);
				float sphRadius = m_simParams.sphRadius;

				if (dsq < sphRadius * sphRadius) {
					float corr = Poly6Kernel::W(diff) / wpoly_con_deltaP;

					float s_corr = -pressure_k * corr * corr * corr * corr;

					float coff = (m_lambda[i] + m_lambda[j] + s_corr);

					glm::vec3 grad = SpikyKernel::gradW(diff);

					deltaP += coff * grad;
				}
			}

			deltaP /= m_simParams.restDensity;
			deltaP *= m_simParams.mass;
		}
	}

	void PBF::applyXSPHViscosity() {
		PROFILE("pbf applyXSPHViscosity");

#pragma omp parallel for
		for (int i = 0; i < m_particleNum; ++i) {
			auto& pos = m_tempPositions[i];

			glm::vec3& vel_i = m_fluidSim->getVelocities()[i];

			glm::vec3& newVel = m_newVel[i];
			newVel = glm::vec3(0.0f);

			auto& curParticleNeighbors = m_neighborList[i];
			for (auto& j : curParticleNeighbors) {
				if (i == j) continue;

				auto& pos_j = m_tempPositions[j];
				auto& vel_j = m_fluidSim->getVelocities()[j];

				glm::vec3 diff = pos - pos_j;

				newVel += Poly6Kernel::W(diff) * (vel_j - vel_i);
			}
			newVel = newVel * m_simParams.viscosity * m_simParams.mass / m_simParams.restDensity;
			newVel += vel_i; // add the original velocity
		}

		// update the velocities with the new velocities
#pragma omp parallel for
		for (int i = 0; i < m_particleNum; ++i) {
			auto& newVel = m_newVel[i];
			auto& vel_i = m_fluidSim->getVelocities()[i];
			vel_i = newVel;
		}
	}

	void PBF::updatePosAndVel() {
		PROFILE("pbf updatePosAndVel");
		std::lock_guard<std::mutex> lock(m_posMutex);
		auto& positions = m_fluidSim->getPositions();
		auto& velocities = m_fluidSim->getVelocities();
#pragma omp parallel for
		for (int i = 0; i < m_particleNum; ++i) {
			auto& pos = m_tempPositions[i];
			auto& deltaP = m_deltaP[i];
			// update position
			pos += deltaP;
			// update velocity
			glm::vec3 vel = (pos - positions[i]) / m_dt;
			velocities[i] = vel;
			// update position in the original positions vector
			positions[i] = pos;
		}
	}
}
