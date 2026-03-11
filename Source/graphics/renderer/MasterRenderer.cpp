#include "pch.h"
#include "MasterRenderer.h"

#include <gl/glew.h>
#include <ui/RuntimePane.h>
#include "rhi/include/RHIContext.h"

namespace engine
{

	MasterRenderer::MasterRenderer(Scene3D* scene) : m_ActiveScene(scene),
		m_ShadowmapPass(scene),
		m_LightingPass(scene),
		m_PostProcessPass(scene),
		m_EnvironmentProbePass(scene),
		m_DeferredGeometryPass(scene),
		m_DeferredLightingPass(scene),
		m_SSAOPass(scene)
	{
		// 初始化 UBO 管理器
		m_UBOManager.initialize(getRHIDevice());
		setUBOManager(&m_UBOManager);
	}

	MasterRenderer::~MasterRenderer() {
		setUBOManager(nullptr);
		m_UBOManager.shutdown();
	}

	void MasterRenderer::init() {
		// GL_TEXTURE_CUBE_MAP_SEAMLESS 已在 OpenGLDevice::initialize() 中设置
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
		BEGIN_EVENT("Shadowmap");
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
		m_PostProcessPass.executeRenderPass(lightingOutput);
		END_EVENT();
#if DEBUG_ENABLED
		glFinish();
		RuntimePane::setPostProcessTimer((float)m_Timer.elapsed());
		END_EVENT();
#endif
#else
		BEGIN_EVENT("Shadowmap");
		ShadowmapPassOutput shadowmapOutput = m_ShadowmapPass.generateShadowmaps(m_ActiveScene->getCamera());
		END_EVENT();
		BEGIN_EVENT("GeometryPass");
		GeometryPassOutput geometryOutput = m_DeferredGeometryPass.ExecuteGeometryPass(m_ActiveScene->getCamera(), false);
		END_EVENT();
		BEGIN_EVENT("SSAOPass");
		PreLightingPassOutput ssaoOutput = m_SSAOPass.executeSSAOPass(m_ActiveScene->getCamera(), geometryOutput);
		END_EVENT();
		BEGIN_EVENT("LightingPass");
		LightingPassOutput deferredLightingOutput = m_DeferredLightingPass.ExecuteLightingPass(shadowmapOutput, geometryOutput, ssaoOutput, m_ActiveScene->getCamera(), true);
		END_EVENT();
		m_PostProcessPass.executeRenderPass(deferredLightingOutput);

#endif // FORWARD_RENDER


	}

}

