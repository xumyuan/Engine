#pragma once

#include <graphics/renderer/renderpass/forward/ForwardProbePass.h>
#include <graphics/renderer/renderpass/forward/ForwardLightingPass.h>
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

		// Render passes
		ShadowmapPass m_ShadowmapPass;
		ForwardLightingPass m_LightingPass;
		PostProcessPass m_PostProcessPass;
		ForwardProbePass m_EnvironmentProbePass;


		Timer m_Timer;
	};

}