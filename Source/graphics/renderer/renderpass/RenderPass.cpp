#include "pch.h"
#include "RenderPass.h"

namespace engine
{

	RenderPass::RenderPass(const RenderScene& renderScene, RenderPassType passType) : m_RenderScene(renderScene), m_RenderPassType(passType) {
		// 过渡阶段：启用即时执行模式，使录制的命令在录制时立即分发到 RHIDevice。
		// 这保证了 cmd().bindPipeline() 等命令在 Shader::setUniform() 之前生效。
		// 后续所有高层操作都迁移到命令缓冲后，可移除此行切换为纯录制模式。
		m_CommandBuffer.setImmediateDevice(getRHIDevice());
	}

	RenderPass::~RenderPass() {}

}