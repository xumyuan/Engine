#pragma once
#include <graphics/camera/ICamera.h>
#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/RenderTarget.h>
#include <graphics/Shader.h>
#include <scene/Scene3D.h>

namespace engine
{

	class ShadowmapPass : public RenderPass
	{
	public:
		ShadowmapPass(Scene3D* scene);
		ShadowmapPass(Scene3D* scene, RenderTarget* customRT);
		virtual ~ShadowmapPass() override;

		ShadowmapPassOutput generateShadowmaps(ICamera* camera);
	private:
		RenderTarget* m_RT;
		bool m_OwnsRT;
		Shader* m_ShadowmapShader;
	};

}
