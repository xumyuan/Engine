#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/Shader.h>
#include <scene/Scene3D.h>

namespace engine
{

	class ShadowmapPass : public RenderPass
	{
	public:
		ShadowmapPass(Scene3D* scene);
		virtual ~ShadowmapPass() override;

		ShadowmapPassOutput executeRenderPass();
	private:
		Framebuffer m_ShadowmapFramebuffer;
		Shader m_ShadowmapShader;
	};

}