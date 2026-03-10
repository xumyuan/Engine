#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/renderpass/RenderPassType.h>
#include <graphics/renderer/RenderTarget.h>

namespace engine
{
	class Shader;
	class Scene3D;
	class ICamera;

	class DeferredLightingPass : public RenderPass {
	public:
		DeferredLightingPass(Scene3D* scene);
		virtual ~DeferredLightingPass() override;

		LightingPassOutput ExecuteLightingPass(ShadowmapPassOutput& inputShadowmapData, GeometryPassOutput& inputGbuffer, PreLightingPassOutput& preLightingOutput, ICamera* camera, bool useIBL);
	private:
		void BindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData);
	private:
		RenderTarget* m_RT;
		Shader* m_LightingShader;
	};
}
