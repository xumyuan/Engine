#include "pch.h"
#include "MasterRenderer.h"

#include <ui/RuntimePane.h>

namespace engine
{

	MasterRenderer::MasterRenderer(Scene3D* scene) : m_ActiveScene(scene),
		m_ShadowmapPass(scene), m_LightingPass(scene), m_PostProcessPass(scene)
	{
		m_GLCache = GLCache::getInstance();
	}

	void MasterRenderer::render() {
		// Shadow map pass
#if DEBUG_ENABLED
		glFinish();
		m_Timer.reset();
#endif
		ShadowmapPassOutput shadowmapOutput = m_ShadowmapPass.executeRenderPass();
#if DEBUG_ENABLED
		glFinish();
		RuntimePane::setShadowmapTimer((float)m_Timer.elapsed());
#endif

		// Lighting Pass
		LightingPassOutput lightingOutput = m_LightingPass.executeRenderPass(shadowmapOutput);

		// Post Process Pass
#if DEBUG_ENABLED
		glFinish();
		m_Timer.reset();
#endif
		m_PostProcessPass.executeRenderPass(lightingOutput.outputFramebuffer);
#if DEBUG_ENABLED
		glFinish();
		RuntimePane::setPostProcessTimer((float)m_Timer.elapsed());
#endif
	}

}

