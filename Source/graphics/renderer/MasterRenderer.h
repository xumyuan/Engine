#pragma once

#include <graphics/renderer/renderpass/forward/ForwardProbePass.h>
#include <graphics/renderer/renderpass/forward/ForwardLightingPass.h>
#include <graphics/renderer/renderpass/deferred/DeferredGeometryPass.h>
#include <graphics/renderer/renderpass/deferred/DeferredLightingPass.h>
#include <graphics/renderer/renderpass/PostProcessPass.h>
#include <graphics/renderer/renderpass/ShadowmapPass.h>
#include <graphics/renderer/renderpass/SSAOPass.h>
#include <scene/Scene3D.h>
#include <utils/Timer.h>
#include <graphics/UniformBufferManager.h>
#include <rhi/include/RHICommandQueue.h>

namespace engine
{

	class MasterRenderer
	{
	public:
		MasterRenderer(Scene3D* scene);
		~MasterRenderer();

		void init();
		void render();

		// 获取命令队列（供外部查询调试信息等）
		rhi::CommandQueue& getCommandQueue() { return m_CommandQueue; }

	private:
		Scene3D* m_ActiveScene;
		RenderScene m_RenderScene;

		// UBO 管理器
		UniformBufferManager m_UBOManager;

		// 命令队列（统一管理所有 pass 录制的命令）
		rhi::CommandQueue m_CommandQueue;

		// other passes 
		PostProcessPass m_PostProcessPass;
		ShadowmapPass m_ShadowmapPass;
		SSAOPass m_SSAOPass;

		// Forward passes
		ForwardLightingPass m_LightingPass;
		ForwardProbePass m_EnvironmentProbePass;

		// Deferred passes
		DeferredGeometryPass m_DeferredGeometryPass;
		DeferredLightingPass m_DeferredLightingPass;


		Timer m_Timer;
	};

}
