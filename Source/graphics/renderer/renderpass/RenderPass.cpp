#include "pch.h"
#include "RenderPass.h"

namespace engine
{

	RenderPass::RenderPass(const RenderScene& renderScene, RenderPassType passType) : m_RenderScene(renderScene), m_RenderPassType(passType) {
		// 纯录制模式：命令仅记录到 CommandBuffer，不会立即执行。
		// 所有命令在 MasterRenderer::render() 末尾通过 CommandQueue::flush() 统一分发。
	}

	RenderPass::~RenderPass() {}

}