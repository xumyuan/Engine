#pragma once

#include "RenderPassType.h"
#include "scene/Scene3D.h"
#include "rhi/include/RHIContext.h"
#include "rhi/include/RHIResources.h"

namespace engine {

	class RenderPass
	{
	public:
		RenderPass(Scene3D* scene, RenderPassType passType);
		virtual ~RenderPass();
	protected:
		// 获取 RHI 设备（便捷方法）
		rhi::RHIDevice* getRHIDevice() const { return engine::getRHIDevice(); }

		// 绑定管线状态到 RHI 设备
		void bindPipelineState(const rhi::PipelineState& state) {
			if (auto* dev = getRHIDevice()) {
				dev->bindPipeline(state);
			}
		}

		Scene3D* m_ActiveScene;
		RenderPassType m_RenderPassType;
	};

}
