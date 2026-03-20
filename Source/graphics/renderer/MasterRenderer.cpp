#include "pch.h"
#include "MasterRenderer.h"

#include <gl/glew.h>
#include <ui/RuntimePane.h>
#include "rhi/include/RHIContext.h"

namespace engine
{

	MasterRenderer::MasterRenderer(Scene3D* scene) : m_ActiveScene(scene),
		m_RenderScene(scene->extractRenderScene()),
		m_ShadowmapPass(m_RenderScene),
		m_LightingPass(m_RenderScene),
		m_PostProcessPass(m_RenderScene),
		m_EnvironmentProbePass(m_RenderScene),
		m_DeferredGeometryPass(m_RenderScene),
		m_DeferredLightingPass(m_RenderScene),
		m_SSAOPass(m_RenderScene)
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
		// 每帧更新渲染场景快照（场景数据可能在帧间变化）
		m_RenderScene = m_ActiveScene->extractRenderScene();

#if FORWARD_RENDER
		BEGIN_EVENT("Forward render");
#if DEBUG_ENABLED
		glFinish();
		m_Timer.reset();
#endif
		BEGIN_EVENT("Shadowmap");
		// Shadow map pass
		ShadowmapPassOutput shadowmapOutput = m_ShadowmapPass.generateShadowmaps(m_RenderScene.camera);
		END_EVENT();
#if DEBUG_ENABLED
		glFinish();
		RuntimePane::setShadowmapTimer((float)m_Timer.elapsed());
#endif
		BEGIN_EVENT("Light");
		// Lighting Pass
		LightingPassOutput lightingOutput = m_LightingPass.executeRenderPass(shadowmapOutput, m_RenderScene.camera, true);
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
		ShadowmapPassOutput shadowmapOutput = m_ShadowmapPass.generateShadowmaps(m_RenderScene.camera);
		END_EVENT();
		BEGIN_EVENT("GeometryPass");
		GeometryPassOutput geometryOutput = m_DeferredGeometryPass.ExecuteGeometryPass(m_RenderScene.camera, false);
		END_EVENT();
		BEGIN_EVENT("SSAOPass");
		PreLightingPassOutput ssaoOutput = m_SSAOPass.executeSSAOPass(m_RenderScene.camera, geometryOutput);
		END_EVENT();
		BEGIN_EVENT("LightingPass");
		LightingPassOutput deferredLightingOutput = m_DeferredLightingPass.ExecuteLightingPass(shadowmapOutput, geometryOutput, ssaoOutput, m_RenderScene.camera, true);
		END_EVENT();
		m_PostProcessPass.executeRenderPass(deferredLightingOutput);

#endif // FORWARD_RENDER


	}

}

