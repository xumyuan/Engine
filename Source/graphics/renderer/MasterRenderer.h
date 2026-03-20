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

namespace engine
{

	class MasterRenderer
	{
	public:
		MasterRenderer(Scene3D* scene);
		~MasterRenderer();

		void init();
		void render();
	private:
		Scene3D* m_ActiveScene;
		RenderScene m_RenderScene;

		// UBO 管理器
		UniformBufferManager m_UBOManager;

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
