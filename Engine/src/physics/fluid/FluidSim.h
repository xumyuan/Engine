#pragma once

#include "platform/OpenGL/IndexBuffer.h"
#include "platform/OpenGL/VertexArray.h"
#include "platform/OpenGL/Buffer.h"
#include "graphics/shader.h"
#include "graphics/renderer/GLCache.h"
#include <mutex>

namespace engine {

	class FPSCamera;
	class PBF;

	struct Boundary
	{
		glm::vec3 min;
		glm::vec3 max;
	};

	struct SimParams {
		float spacing;		//粒子的间距
		float sphRadius;	//SPH核半径,一般取值是粒子间距的3倍
		float restDensity;	//静止密度，一般设置为1000.0f
		float mass;			//粒子质量 m = restDensity * spacing^3
		float dt;			//时间步长
		glm::vec3 gravity;	//重力加速度，一般取为(0.0f, -9.81f, 0.0f)
		Boundary boundary;	//模拟边界，目前是规则的立方体
		float viscosity;	// 粘性系数
	};

	class FluidSim
	{
	public:
		FluidSim(size_t pnum, Boundary boundary);
		~FluidSim();

		std::vector<glm::vec3>& getPositions() { return m_positions; }
		std::vector<glm::vec3>& getVelocities() { return m_velocities; }
		std::vector<std::vector<size_t>>& getNeighborList() { return m_neighborList; }

		glm::vec3& getMin() { return m_simParams.boundary.min; }
		glm::vec3& getMax() { return m_simParams.boundary.max; }
		SimParams& getSimParams() { return m_simParams; }

		float getSpacing() const { return m_simParams.spacing; }
		float getSphKernelRadius() const { return m_simParams.sphRadius; }
		float getmass() const { return m_simParams.mass; }
		float getRestDensity() const { return m_simParams.restDensity; }


		void init();

		void drawParticle(FPSCamera* camera);

		void startSim();
		void subPosData();
		size_t getParticleNum() const { return m_particleNum; }
	private:
		GLCache* m_GLCache;

		size_t m_maxParticleNum;
		size_t m_particleNum;

		// particle attribute
		std::vector<glm::vec3> m_positions;
		std::vector<glm::vec3> m_velocities;

		// neighborList
		std::vector<std::vector<size_t>> m_neighborList;

		// buffer
		VertexArray m_fluidVAO;
		Buffer	m_fluidVBO;
		// shader
		Shader* m_particleShader;

		// solver
		PBF* m_pbf = nullptr;

		// mutex
		std::condition_variable m_condVar;
		bool dataReady = false;

		SimParams m_simParams;
	};


}
