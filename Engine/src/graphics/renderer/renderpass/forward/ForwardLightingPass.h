#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/Shader.h>
#include <scene/Scene3D.h>

namespace engine
{

	class ForwardLightingPass : public RenderPass
	{
	public:
		ForwardLightingPass(Scene3D* scene);
		ForwardLightingPass(Scene3D* scene, Framebuffer* customFramebuffer);
		virtual ~ForwardLightingPass() override;

		LightingPassOutput executeRenderPass(ShadowmapPassOutput& shadowmapData, ICamera* camera, bool useIBL);
	private:
		void bindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData);
	private:
		Framebuffer* m_Framebuffer = nullptr;

		Shader* m_ModelShader, * m_TerrainShader;
		
	};

}