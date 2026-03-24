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

		// 即时/录制模式切换
		void enableImmediateMode(rhi::RHIDevice* device) { m_CommandBuffer.setImmediateDevice(device); }
		void disableImmediateMode() { m_CommandBuffer.setImmediateDevice(nullptr); }
		bool isImmediateMode() const { return m_CommandBuffer.getImmediateDevice() != nullptr; }

		// 设置外部命令缓冲（让临时 pass 将命令录制到父 pass 的 CommandBuffer 中）
		// 当设置了外部 CommandBuffer 后，cmd() 返回外部缓冲而非内部缓冲。
		void setExternalCommandBuffer(rhi::CommandBuffer* externalCmdBuf) { m_ExternalCommandBuffer = externalCmdBuf; }

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
		// 如果设置了外部 CommandBuffer，则返回外部缓冲
		rhi::CommandBuffer& cmd() { return m_ExternalCommandBuffer ? *m_ExternalCommandBuffer : m_CommandBuffer; }

		RenderScene m_RenderScene;
		RenderPassType m_RenderPassType;

	private:
		rhi::CommandBuffer m_CommandBuffer;
		rhi::CommandBuffer* m_ExternalCommandBuffer = nullptr;  // 外部命令缓冲（不拥有所有权）
	};

}
