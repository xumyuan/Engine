#include "pch.h"
#include "MasterRenderer.h"

#include <gl/glew.h>
#include <ui/RuntimePane.h>
#include "rhi/include/RHIContext.h"

namespace engine
{

	MasterRenderer::MasterRenderer(Scene3D* scene) : m_ActiveScene(scene),
		m_RenderScene(scene->extractRenderScene()),
		m_CommandQueue(getRHIDevice()),
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

		// 应用默认命令模式到所有 pass
		applyCommandModeToAllPasses();
	}

	MasterRenderer::~MasterRenderer() {
		setUBOManager(nullptr);
		m_UBOManager.shutdown();
	}

	void MasterRenderer::setCommandMode(CommandMode mode) {
		m_CommandMode = mode;
		applyCommandModeToAllPasses();
	}

	void MasterRenderer::applyCommandModeToAllPasses() {
		auto* device = (m_CommandMode == CommandMode::Immediate) ? getRHIDevice() : nullptr;

		m_ShadowmapPass.enableImmediateMode(device);
		m_LightingPass.enableImmediateMode(device);
		m_PostProcessPass.enableImmediateMode(device);
		m_EnvironmentProbePass.enableImmediateMode(device);
		m_DeferredGeometryPass.enableImmediateMode(device);
		m_DeferredLightingPass.enableImmediateMode(device);
		m_SSAOPass.enableImmediateMode(device);
	}

	void MasterRenderer::init() {
		// GL_TEXTURE_CUBE_MAP_SEAMLESS 已在 OpenGLDevice::initialize() 中设置
		// 环境贴图预计算
		BEGIN_EVENT("GenerateProbes");
		m_EnvironmentProbePass.pregenerateProbes();
		END_EVENT();

		// 延迟模式：提交到命令队列后统一执行
		// 即时模式：命令已在录制时执行，submit/flush 仅为清理
		m_CommandQueue.submit(m_EnvironmentProbePass.getCommandBuffer());
		if (m_CommandMode == CommandMode::Deferred) {
			m_CommandQueue.flush();
		} else {
			m_CommandQueue.clear();
		}
		m_EnvironmentProbePass.resetCommandBuffer();
	}

	void MasterRenderer::render() {
		// 每帧更新渲染场景快照（场景数据可能在帧间变化）
		m_RenderScene = m_ActiveScene->extractRenderScene();

#if FORWARD_RENDER
		BEGIN_EVENT("Forward render");
		BEGIN_EVENT("Shadowmap");
		// Shadow map pass（仅录制命令）
		ShadowmapPassOutput shadowmapOutput = m_ShadowmapPass.generateShadowmaps(m_RenderScene.camera);
		END_EVENT();
		BEGIN_EVENT("Light");
		// Lighting Pass（仅录制命令）
		LightingPassOutput lightingOutput = m_LightingPass.executeRenderPass(shadowmapOutput, m_RenderScene.camera, true);
		END_EVENT();
		BEGIN_EVENT("PostProcess");
		// 后处理 Pass（仅录制命令）
		m_PostProcessPass.executeRenderPass(lightingOutput);
		END_EVENT();
		END_EVENT();
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

		// ===== 命令执行 =====
		if (m_CommandMode == CommandMode::Deferred) {
			// 延迟模式：提交所有 pass 的命令缓冲到队列，然后统一执行
#if FORWARD_RENDER
			m_CommandQueue.submit(m_ShadowmapPass.getCommandBuffer());
			m_CommandQueue.submit(m_LightingPass.getCommandBuffer());
			m_CommandQueue.submit(m_PostProcessPass.getCommandBuffer());
#else
			m_CommandQueue.submit(m_ShadowmapPass.getCommandBuffer());
			m_CommandQueue.submit(m_DeferredGeometryPass.getCommandBuffer());
			m_CommandQueue.submit(m_SSAOPass.getCommandBuffer());
			m_CommandQueue.submit(m_DeferredLightingPass.getCommandBuffer());
			m_CommandQueue.submit(m_PostProcessPass.getCommandBuffer());
#endif
			// 统一执行所有已提交的命令
			m_CommandQueue.flush();
		}
		// 即时模式：命令已在录制时执行，无需 submit/flush

		// 重置所有 pass 的命令缓冲，为下一帧做准备
		m_ShadowmapPass.resetCommandBuffer();
		m_LightingPass.resetCommandBuffer();
		m_PostProcessPass.resetCommandBuffer();
		m_SSAOPass.resetCommandBuffer();
		m_DeferredGeometryPass.resetCommandBuffer();
		m_DeferredLightingPass.resetCommandBuffer();
	}

}

