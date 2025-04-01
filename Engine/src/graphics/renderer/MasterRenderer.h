#pragma once

#include <graphics/renderer/renderpass/forward/ForwardProbePass.h>
#include <graphics/renderer/renderpass/forward/ForwardLightingPass.h>
#include <graphics/renderer/renderpass/defferred/DeferredGeometryPass.h>
#include <graphics/renderer/renderpass/PostProcessPass.h>
#include <graphics/renderer/renderpass/ShadowmapPass.h>
#include <scene/Scene3D.h>
#include <utils/Timer.h>

namespace engine
{

	class MasterRenderer
	{
	public:
		MasterRenderer(Scene3D* scene);

		void init();
		void render();
	private:
		Scene3D* m_ActiveScene;

		GLCache* m_GLCache;

		// other passes 
		PostProcessPass m_PostProcessPass;
		ShadowmapPass m_ShadowmapPass;

		// Forward passes
		ForwardLightingPass m_LightingPass;
		ForwardProbePass m_EnvironmentProbePass;

		// Deferred passes
		DeferredGeometryPass m_DeferredGeometryPass;


		Timer m_Timer;
	};

}