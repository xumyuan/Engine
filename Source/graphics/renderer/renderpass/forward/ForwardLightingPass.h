#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/RenderTarget.h>
#include <graphics/Shader.h>
#include <scene/Scene3D.h>

namespace engine
{

	class ForwardLightingPass : public RenderPass
	{
	public:
		ForwardLightingPass(Scene3D* scene);
		ForwardLightingPass(Scene3D* scene, RenderTarget* customRT);
		virtual ~ForwardLightingPass() override;

		LightingPassOutput executeRenderPass(ShadowmapPassOutput& shadowmapData, ICamera* camera, bool useIBL);
	private:
		void bindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData);
	private:
		RenderTarget* m_RT = nullptr;
		bool m_OwnsRT = false;

		Shader* m_ModelShader, * m_TerrainShader;
		
	};

}
