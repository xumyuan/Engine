#pragma once
#pragma once

namespace engine {

	enum RenderPassType {
		ShadowmapPass,
		LightingPass,
		PostProcessPass
	};

	struct ShadowmapPassOutput
	{
		glm::mat4 directionalLightViewProjMatrix;
		unsigned int shadowmapTexture;
	};

	struct LightingPassOutput
	{
		unsigned int outputTexture;
	};

}