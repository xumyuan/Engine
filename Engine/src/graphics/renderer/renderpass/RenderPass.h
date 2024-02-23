#pragma once

#include "RenderPassType.h"
#include "scene/Scene3D.h"

namespace engine {

	class RenderPass
	{
	public:
		RenderPass(Scene3D* scene, RenderPassType passType);
		virtual ~RenderPass();
	protected:
		GLCache* m_GLCache;

		Scene3D* m_ActiveScene;
		RenderPassType m_RenderPassType;
	};

}