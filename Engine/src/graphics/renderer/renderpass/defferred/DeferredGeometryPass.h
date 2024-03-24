#pragma once
#include <graphics/renderer/renderpass/RenderPass.h>
namespace engine
{

	enum DeferredStencilValue :int {
		ModelStencilValue = 0x01,
		LightStencilValue = 0x02,
	};
	class DeferredGeometryPass :public RenderPass
	{

	};
}




