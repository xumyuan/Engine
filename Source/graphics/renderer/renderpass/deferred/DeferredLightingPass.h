#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/renderpass/RenderPassType.h>
#include <graphics/renderer/RenderTarget.h>
#include <rhi/include/RHICommandBuffer.h>

namespace engine
{
	class Shader;
	class ICamera;

	class DeferredLightingPass : public RenderPass {
	public:
		DeferredLightingPass(const RenderScene& renderScene);
		virtual ~DeferredLightingPass() override;

		LightingPassOutput ExecuteLightingPass(ShadowmapPassOutput& inputShadowmapData, GeometryPassOutput& inputGbuffer, PreLightingPassOutput& preLightingOutput, ICamera* camera, bool useIBL);
	private:
		void BindShadowmap(rhi::CommandBuffer& cmdBuf, rhi::ProgramHandle program, ShadowmapPassOutput& shadowmapData);
	private:
		RenderTarget* m_RT;
		Shader* m_LightingShader;
	};
}
