#pragma once
#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/Shader.h>
#include <scene/Scene3D.h>
namespace engine
{

	/*enum DeferredStencilValue :int {
		ModelStencilValue = 0x01,
		LightStencilValue = 0x02,
	};*/

	class Shader;
	class Scene3D;
	class ICamera;
	class GBuffer;

	class DeferredGeometryPass :public RenderPass
	{
	public:
		DeferredGeometryPass(Scene3D* scene);
		DeferredGeometryPass(Scene3D* scene, GBuffer* customGBuffer);
		virtual ~DeferredGeometryPass() override;
		GeometryPassOutput ExecuteGeometryPass(ICamera* camera, bool renderOnlyStatic);
	private:
		bool m_AllocatedGBuffer;
		GBuffer* m_GBuffer;
		Shader* m_ModelShader, /** m_SkinnedModelShader,*/* m_TerrainShader;
	};
}




