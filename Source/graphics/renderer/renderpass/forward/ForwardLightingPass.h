#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/RenderTarget.h>
#include <graphics/Shader.h>
#include <rhi/include/RHICommandBuffer.h>

namespace engine
{
	class LightMarkerRenderer;

	class ForwardLightingPass : public RenderPass
	{
	public:
		ForwardLightingPass(const RenderScene& renderScene);
		ForwardLightingPass(const RenderScene& renderScene, RenderTarget* customRT);
		virtual ~ForwardLightingPass() override;

		LightingPassOutput executeRenderPass(ShadowmapPassOutput& shadowmapData, ICamera* camera, bool useIBL, LightMarkerRenderer* markers = nullptr);
	private:
		void bindShadowmap(rhi::CommandBuffer& cmdBuf, ShadowmapPassOutput& shadowmapData);
	private:
		RenderTarget* m_RT = nullptr;
		bool m_OwnsRT = false;

		Shader* m_ModelShader, * m_TerrainShader;
		
	};

}
