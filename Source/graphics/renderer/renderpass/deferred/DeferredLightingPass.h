#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/renderpass/RenderPassType.h>
#include <graphics/renderer/RenderTarget.h>
#include <rhi/include/RHICommandBuffer.h>

namespace engine
{
	class Shader;
	class ICamera;
	class LightMarkerRenderer;

	class DeferredLightingPass : public RenderPass {
	public:
		DeferredLightingPass(const RenderScene& renderScene);
		virtual ~DeferredLightingPass() override;

		LightingPassOutput ExecuteLightingPass(ShadowmapPassOutput& inputShadowmapData, GeometryPassOutput& inputGbuffer, PreLightingPassOutput& preLightingOutput, ICamera* camera, bool useIBL, LightMarkerRenderer* markers = nullptr);
	private:
		void BindShadowmap(rhi::CommandBuffer& cmdBuf, ShadowmapPassOutput& shadowmapData);
	private:
		RenderTarget* m_RT;
		Shader* m_LightingShader;
	};
}
