#pragma once

#include "graphics/camera/ICamera.h"
#include "graphics/renderer/ModelRenderer.h"
#include "graphics/Skybox.h"
#include "terrain/Terrain.h"
#include "graphics/dynamic_lights/DynamicLightManager.h"
#include "graphics/ibl/ProbeManager.h"

namespace engine {

	class FluidSim;
	class RenderableModel;
	class SceneNode;

	/// @brief 渲染场景数据快照 —— 每帧从 Scene3D 提取的只读渲染数据
	/// RenderPass 通过此结构访问场景数据，而非直接持有 Scene3D*
	/// 这样实现了渲染系统与场景管理的解耦
	struct RenderScene {
		ICamera* camera = nullptr;
		ModelRenderer* modelRenderer = nullptr;
		Skybox* skybox = nullptr;
		Terrain* terrain = nullptr;
		DynamicLightManager* lightManager = nullptr;
		ProbeManager* probeManager = nullptr;
		FluidSim* fluid = nullptr;
		std::vector<RenderableModel*>* renderableModels = nullptr;

		/// @brief 场景节点树根节点（阶段三新增）
		/// 通过遍历节点树可访问所有 SceneNode 及其 Component
		SceneNode* rootNode = nullptr;

		/// @brief 将模型提交到 ModelRenderer 的渲染队列
		void submitModelsToRenderer() const;
	};

}
