#include "pch.h"
#include "omp.h"
#include "pbf.h"

namespace engine {

	const float ploy6_kernel = 315.0f / (64.0f * glm::pi<float>() * glm::pow(3.0f, 9.0f));
	const float spiky_kernel = -45.0f / (glm::pi<float>() * glm::pow(3.0f, 6.0f));
	const float delta_p = 0.1 * 9.0f;
	const float wpoly_con_deltaP = pow(9.0 - delta_p * delta_p, 3) * ploy6_kernel;
	const float pressure_k = 0.1f; // pressure constant, can be adjusted
	const float pressure_n = 4.0f;

	float Wpoly6(const glm::vec3& dist) {
		const float len = glm::length(dist);
		const float len2 = len * len;

		const float r2 = 9.0f;

		float item = r2 - len2;

		if (item < 0.0f) return 0.0f;

		float res = ploy6_kernel * glm::pow(item, 3.0f);

		return res;
	}

	glm::vec3 Wspiky_grad(const glm::vec3& dist) {
		const float len = glm::length(dist);
		const float len2 = len * len;
		const float r = 3.0f;
		const float r2 = 9.0f;
		if (len < 1e-6f) return glm::vec3(0.0f);
		if (len2 > r2 * r2) return glm::vec3(0.0f);

		float item = r - len;

		float res = spiky_kernel * glm::pow(item, 2.0f);

		glm::vec3 grad = glm::normalize(dist) * res;
		return grad;
	}

	void PBF::solve() {
		predictAdvect();
		spdlog::info("predictAdvect over");
		size_t iter = 0;
		while (iter++ < m_iterations) {
			computeLambda();
			computeDeltaP();
		}
		spdlog::info("iterate over");
		updatePosAndVel();
		spdlog::info("updatePosAndVel over");
	}

	void PBF::predictAdvect() {
#pragma omp parallel for
		for (int i = 0; i < m_particleNum; ++i) {
			auto& position = m_fluidSim->getPositions()[i];
			glm::vec3 vel = m_fluidSim->getVelocities()[i] + m_dt * m_fluidSim->m_gravity;
			m_tempPositions[i] = position + vel * m_dt;

			//confine the particle to the boundary
			float spacing = m_fluidSim->getSpacing();

			glm::vec3& min = m_fluidSim->getMin();
			glm::vec3& max = m_fluidSim->getMax();

			glm::vec3 moveDir = m_tempPositions[i] - position;
			moveDir *= 0.5f;

			if (m_tempPositions[i].x < min.x + spacing) {
				m_tempPositions[i].x = position.x + spacing;
				m_tempPositions[i].x -= moveDir.x;
			}
			else if (m_tempPositions[i].x > max.x - spacing) {
				m_tempPositions[i].x = position.x - spacing;
				m_tempPositions[i].x -= moveDir.x;
			}

			if (m_tempPositions[i].y < min.y + spacing) {
				m_tempPositions[i].y = position.y + spacing;
				m_tempPositions[i].y -= moveDir.y;
			}
			else if (m_tempPositions[i].y > max.y - spacing) {
				m_tempPositions[i].y = position.y - spacing;
				m_tempPositions[i].y -= moveDir.y;
			}

			if (m_tempPositions[i].z < min.z + spacing) {
				m_tempPositions[i].z = position.z + spacing;
				m_tempPositions[i].z -= moveDir.z;
			}
			else if (m_tempPositions[i].z > max.z - spacing) {
				m_tempPositions[i].z = position.z - spacing;
				m_tempPositions[i].z -= moveDir.z;
			}
		}
	}

	void PBF::computeLambda() {
#pragma omp parallel for
		for (int i = 0; i < m_particleNum; ++i) {
			auto& pos = m_tempPositions[i];
			float& lambda = m_lambda[i];

			glm::vec3 grad_pi_C = glm::vec3(0.0f);
			float grad_C_sum = 0.0f;
			float density = 0.0f;
			float mass = m_fluidSim->getmass();
			float restDensity = m_fluidSim->getRestDensity();

			for (size_t j = 0; j < m_particleNum; ++j) {
				if (i == j) continue;
				auto& pos_j = m_tempPositions[j];
				glm::vec3 diff = pos - pos_j;
				float dist = glm::length(diff);
				float sphRadius = m_fluidSim->getSphKernelRadius();

				if (dist < sphRadius) {
					density += Wpoly6(diff);
					glm::vec3 grad_pj_C = Wspiky_grad(diff) * mass / restDensity;
					grad_pi_C -= grad_pj_C;
					grad_C_sum += glm::dot(grad_pj_C, grad_pj_C);
				}
			}
			density *= mass;
			float constraint = density / restDensity - 1;
			float grad_i_sum = glm::dot(grad_pi_C, grad_pi_C);
			lambda = -constraint / (grad_i_sum + 1000.0f);
		}
	}

	void PBF::computeDeltaP() {
#pragma omp parallel for
		for (int i = 0; i < m_particleNum; ++i) {
			auto& pos = m_tempPositions[i];
			auto& deltaP = m_deltaP[i];

			for (size_t j = 0; j < m_particleNum; ++j) {
				if (i == j) continue;
				auto& pos_j = m_tempPositions[j];
				glm::vec3 diff = pos - pos_j;
				float dsq = glm::dot(diff, diff);
				float sphRadius = m_fluidSim->getSphKernelRadius();
				if (dsq < sphRadius * sphRadius) {
					float corr = Wpoly6(diff) / wpoly_con_deltaP;

					float s_corr = -pressure_k * glm::pow(corr, pressure_n);

					float coff = (m_lambda[i] + m_lambda[j] + s_corr);

					glm::vec3 grad = Wspiky_grad(diff);
					deltaP += coff * grad;
				}
			}
			deltaP /= m_fluidSim->getRestDensity();
			deltaP *= m_fluidSim->getmass();
		}
	}

	void PBF::neighborSearch() {}

	void PBF::updatePosAndVel() {
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
