#pragma once

#include "rhi/include/RHIDevice.h"
#include "rhi/include/RHICommandBuffer.h"
#include "graphics/Shader.h"
#include <atomic>
#include <mutex>
#include <thread>

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
		float invRestDensity; // 静止密度的倒数
		float mass;			//粒子质量 m = restDensity * spacing^3
		float dt;			//时间步长
		float invDt;		//时间步长的倒数
		float volume;		//体积，通常为spacing^3
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

		// 前向渲染用：Blinn-Phong 着色的球形点精灵
		void drawParticle(rhi::CommandBuffer& cmd, FPSCamera* camera);

		// 渲染线程每帧调用一次：模拟线程有新结果时上传到顶点缓冲
		void uploadLatestPositions();
		// 以点图元绘制全部粒子，shader 与管线状态由调用方负责
		void drawPoints(rhi::CommandBuffer& cmd) const;
		float getParticleRadius() const { return m_simParams.spacing * 0.5f; }

		// 在后台线程上持续求解；重复调用无效果。析构时自动停止并等待线程退出
		void startSimulation();
		void stopSimulation();
		bool isSimulating() const { return m_simThread.joinable(); }

		size_t getParticleNum() const { return m_particleNum; }
	private:
		void simulationLoop();
		// 调用方需持有 m_pbf->getPosMutex()（构造期间模拟线程尚未启动时除外）
		void subPosData();

		size_t m_maxParticleNum;
		size_t m_particleNum;

		// particle attribute：m_positions 只由求解器（模拟线程）读写
		std::vector<glm::vec3> m_positions;
		std::vector<glm::vec3> m_velocities;
		// 每步求解完成后在 posMutex 保护下拷贝，渲染线程只读这份快照
		std::vector<glm::vec3> m_renderPositions;

		// neighborList
		std::vector<std::vector<size_t>> m_neighborList;

		// RHI buffer
		rhi::RHIDevice*            m_Device = nullptr;
		rhi::BufferHandle          m_VertexBuffer;
		rhi::RenderPrimitiveHandle m_RenderPrimitive;

		// shader
		Shader* m_particleShader;

		// solver
		PBF* m_pbf = nullptr;

		bool dataReady = false;  // 由 m_pbf->getPosMutex() 保护

		SimParams m_simParams;

		// 析构函数体中先 stopSimulation()，再释放 m_pbf 与 GPU 资源
		std::thread m_simThread;
		std::atomic<bool> m_stopRequested{ false };
	};


}
