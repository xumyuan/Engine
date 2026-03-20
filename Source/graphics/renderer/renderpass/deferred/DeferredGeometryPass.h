#pragma once
#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/RenderTarget.h>
#include <graphics/Shader.h>

namespace engine
{
	class Shader;
	class ICamera;

	class DeferredGeometryPass :public RenderPass
	{
	public:
		DeferredGeometryPass(const RenderScene& renderScene);
		virtual ~DeferredGeometryPass() override;
		GeometryPassOutput ExecuteGeometryPass(ICamera* camera, bool renderOnlyStatic);
	private:
		void initGBuffer();
	private:
		RenderTarget m_GBufferRT;
		// GBuffer 纹理
		// 0 RGBA8  ->  albedo.r     albedo.g        albedo.b             albedo's alpha
		// 1 RGBA32F -> normal.x     normal.y        normal.z
		// 2 RGBA8  ->  metallic     roughness       ambientOcclusion     emissionIntensity
		Texture m_AlbedoTexture;
		Texture m_NormalTexture;
		Texture m_MaterialInfoTexture;

		Shader* m_ModelShader, * m_TerrainShader;
	};
}
