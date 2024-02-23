#include "pch.h"
#include "RenderPass.h"

namespace engine
{

	RenderPass::RenderPass(Scene3D* scene, RenderPassType passType) : m_ActiveScene(scene), m_RenderPassType(passType) {
		m_GLCache = GLCache::getInstance();
	}

	RenderPass::~RenderPass() {}

}