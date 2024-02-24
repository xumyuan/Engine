#pragma once
#include <platform/OpenGL/Framebuffers/Framebuffer.h>

namespace engine {

	enum RenderPassType {
		ShadowmapPassTpye,
		LightingPassType,
		PostProcessPassType,
		EnvironmentProbePassType
	};

	struct ShadowmapPassOutput
	{
		glm::mat4 directionalLightViewProjMatrix;
		unsigned int shadowmapTexture;
	};

	struct LightingPassOutput
	{
		Framebuffer* outputFramebuffer;
	};

}