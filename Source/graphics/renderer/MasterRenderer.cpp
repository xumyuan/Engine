#include "pch.h"
#include "MasterRenderer.h"

#include <ui/RuntimePane.h>

namespace engine
{

	MasterRenderer::MasterRenderer(Scene3D* scene) : m_ActiveScene(scene),
		m_ShadowmapPass(scene),
		m_LightingPass(scene),
		m_PostProcessPass(scene),
		m_EnvironmentProbePass(scene),
		m_DeferredGeometryPass(scene),
		m_DeferredLightingPass(scene)
	{
		m_GLCache = GLCache::getInstance();
	}

	void MasterRenderer::init() {
		// State that should never change
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		// 环境贴图预计算
		BEGIN_EVENT("GenerateProbes");
		m_EnvironmentProbePass.pregenerateProbes();
		END_EVENT();
	}

	void MasterRenderer::render() {

#if FORWARD_RENDER
		BEGIN_EVENT("Forward render");
#if DEBUG_ENABLED
		glFinish();
		m_Timer.reset();
#endif
		BEGIN_EVENT("Shadowmap")
		// Shadow map pass
		ShadowmapPassOutput shadowmapOutput = m_ShadowmapPass.generateShadowmaps(m_ActiveScene->getCamera());
		END_EVENT();
#if DEBUG_ENABLED
		glFinish();
		RuntimePane::setShadowmapTimer((float)m_Timer.elapsed());
#endif
		BEGIN_EVENT("Light");
		// Lighting Pass
		LightingPassOutput lightingOutput = m_LightingPass.executeRenderPass(shadowmapOutput, m_ActiveScene->getCamera(), true);
		END_EVENT();
		// 后处理 Pass
#if DEBUG_ENABLED
		glFinish();
		m_Timer.reset();
#endif
		BEGIN_EVENT("PostProcess");
		m_PostProcessPass.executeRenderPass(lightingOutput.outputFramebuffer);
		END_EVENT();
#if DEBUG_ENABLED
		glFinish();
		RuntimePane::setPostProcessTimer((float)m_Timer.elapsed());
		END_EVENT();
#endif
#else
		ShadowmapPassOutput shadowmapOutput = m_ShadowmapPass.generateShadowmaps(m_ActiveScene->getCamera());

		GeometryPassOutput geometryOutput = m_DeferredGeometryPass.ExecuteGeometryPass(m_ActiveScene->getCamera(), false);

		LightingPassOutput deferredLightingOutput = m_DeferredLightingPass.ExecuteLightingPass(shadowmapOutput, geometryOutput.outputGBuffer, m_ActiveScene->getCamera(), true);

		m_PostProcessPass.executeRenderPass(deferredLightingOutput.outputFramebuffer);

#endif // FORWARD_RENDER


	}

}

