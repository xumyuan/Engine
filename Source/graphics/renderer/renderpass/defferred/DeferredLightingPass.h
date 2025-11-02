#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/renderpass/RenderPassType.h>

namespace engine
{
	class Shader;
	class Scene3D;
	class ICamera;

	class DeferredLightingPass : public RenderPass {
	public:
		DeferredLightingPass(Scene3D* scene);
		DeferredLightingPass(Scene3D* scene, Framebuffer* framebuffer);
		virtual ~DeferredLightingPass() override;

		LightingPassOutput ExecuteLightingPass(ShadowmapPassOutput& inputShadowmapData, GBuffer* inputGbuffer, ICamera* camera, bool useIBL);
	private:
		void BindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData);
	private:
		bool m_AllocatedFramebuffer;
		Framebuffer* m_Framebuffer;
		Shader* m_LightingShader;
	};
}
