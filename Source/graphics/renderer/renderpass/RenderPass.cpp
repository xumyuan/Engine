#include "pch.h"
#include "RenderPass.h"

namespace engine
{

	RenderPass::RenderPass(const RenderScene& renderScene, RenderPassType passType) : m_RenderScene(renderScene), m_RenderPassType(passType) {
	}

	RenderPass::~RenderPass() {}

}