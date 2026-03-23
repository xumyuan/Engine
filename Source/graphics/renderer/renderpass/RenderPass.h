#pragma once

#include "RenderPassType.h"
#include "scene/RenderScene.h"
#include "rhi/include/RHIContext.h"
#include "rhi/include/RHIResources.h"
#include "rhi/include/RHICommandBuffer.h"

namespace engine {

	class RenderPass
	{
	public:
		RenderPass(const RenderScene& renderScene, RenderPassType passType);
		virtual ~RenderPass();

		// 获取当前 pass 的命令缓冲（供 CommandQueue 提交）
		rhi::CommandBuffer& getCommandBuffer() { return m_CommandBuffer; }
		const rhi::CommandBuffer& getCommandBuffer() const { return m_CommandBuffer; }

		// 重置命令缓冲（每帧结束后调用）
		void resetCommandBuffer() { m_CommandBuffer.reset(); }

	protected:
		// 获取 RHI 设备（便捷方法，直接调用模式 - 向后兼容）
		rhi::RHIDevice* getRHIDevice() const { return engine::getRHIDevice(); }

		// 绑定管线状态到 RHI 设备（直接调用模式 - 向后兼容）
		void bindPipelineState(const rhi::PipelineState& state) {
			if (auto* dev = getRHIDevice()) {
				dev->bindPipeline(state);
			}
		}

		// 获取命令缓冲引用（子类录制命令的便捷方法）
		rhi::CommandBuffer& cmd() { return m_CommandBuffer; }

		RenderScene m_RenderScene;
		RenderPassType m_RenderPassType;

	private:
		rhi::CommandBuffer m_CommandBuffer;
	};

}
