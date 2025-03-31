#include "pch.h"
#include "DeferredGeometryPass.h"

#include <scene/Scene3D.h>
#include <utils/loaders/ShaderLoader.h>
#include <platform/OpenGL/Framebuffers/GBuffer.h>
#include <graphics/renderer/renderpass/RenderPassType.h>

namespace engine {

	DeferredGeometryPass::DeferredGeometryPass(Scene3D* scene) : RenderPass(scene, RenderPassType::GeometryPassType),m_AllocatedGBuffer(true)
	{
		m_ModelShader = ShaderLoader::loadShader("deferred/PBR_Model_GeometryPass.glsl");
		m_SkinnedModelShader = ShaderLoader::loadShader("deferred/PBR_Skinned_Model_GeometryPass.glsl");
		m_TerrainShader = ShaderLoader::loadShader("deferred/PBR_Terrain_GeometryPass.glsl");

		//m_GBuffer = new GBuffer(Window::GetRenderResolutionWidth(), Window::GetRenderResolutionHeight());
	}

	DeferredGeometryPass::~DeferredGeometryPass()
	{
		if (m_AllocatedGBuffer) {
			delete m_GBuffer;
		}
	}

}