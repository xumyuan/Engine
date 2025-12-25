#pragma once
#include <platform/OpenGL/Framebuffers/Framebuffer.h>

#include <platform/OpenGL/Framebuffers/GBuffer.h>

namespace engine {

	enum RenderPassType {
		ShadowmapPassType,
		LightingPassType,
		PostProcessPassType,
		ProbePassType,
		GeometryPassType,
		SSAOPassType
	};

	struct ShadowmapPassOutput
	{
		glm::mat4 directionalLightViewProjMatrix;
		Framebuffer* shadowmapFramebuffer;
	};

	struct LightingPassOutput
	{
		Framebuffer* outputFramebuffer;
	};

	struct GeometryPassOutput
	{
		GBuffer* outputGBuffer = nullptr;
	};

	struct PreLightingPassOutput
	{
		Texture* ssaoTexture = nullptr;
	};

}
