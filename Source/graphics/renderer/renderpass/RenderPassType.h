#pragma once
#include <graphics/texture/Texture.h>

namespace engine {

	// 深度/模板附件格式
	enum class DepthStencilFormat : uint8_t {
		DepthOnly,          // Depth24
		DepthStencil,       // Depth24Stencil8
		DepthStencilFloat,  // Depth32FStencil8
	};

	enum StencilValue : int
	{
		ModelStencilValue = 0x01,
		TerrainStencilValue = 0x02
	};

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
		rhi::RenderTargetHandle renderTarget;
		Texture* depthTexture = nullptr;
	};

	struct LightingPassOutput
	{
		rhi::RenderTargetHandle renderTarget;
		Texture* colorTexture = nullptr;
		uint32_t width = 0;
		uint32_t height = 0;
		bool isMultisampled = false;
	};

	struct GeometryPassOutput
	{
		rhi::RenderTargetHandle renderTarget;
		Texture* albedoTexture = nullptr;
		Texture* normalTexture = nullptr;
		Texture* materialInfoTexture = nullptr;
		Texture* depthStencilTexture = nullptr;
		uint32_t width = 0;
		uint32_t height = 0;
	};

	struct PreLightingPassOutput
	{
		Texture* ssaoTexture = nullptr;
	};

}
