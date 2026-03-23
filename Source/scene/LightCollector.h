#pragma once

#include "scene/SceneNode.h"
#include "scene/components/LightComponent.h"
#include "graphics/UniformBufferData.h"
#include <vector>

namespace engine {

	/// @brief 灯光收集器 —— 从场景节点树中收集灯光数据
	/// 
	/// 职责：
	/// 1. 每帧遍历节点树，收集所有 LightComponent
	/// 2. 将灯光数据打包填充到 UBOLighting（GPU 通信）
	/// 3. 提供方向光方向查询（供阴影图使用）
	/// 4. 提供按名称查找灯光节点（供运行时控制使用）
	///
	/// 设计理念：
	/// - LightComponent 负责存储灯光属性数据（场景层）
	/// - LightCollector 负责收集 + 打包到 UBO（收集层 + GPU 数据层）
	/// - 渲染通道只需调用 fillLightingUBO() 即可，与场景层解耦
	class LightCollector {
	public:
		LightCollector() = default;

		/// @brief 从节点树收集所有灯光，填充 UBOLighting
		/// @param rootNode 场景根节点
		/// @param ubo 要填充的 UBOLighting 结构体
		void fillLightingUBO(SceneNode* rootNode, UBOLighting& ubo);

		/// @brief 获取第一个激活的方向光方向（供阴影图计算使用）
		/// @param rootNode 场景根节点
		/// @return 方向光方向向量，如果没有激活的方向光则返回默认值
		glm::vec3 getDirectionalLightDirection(SceneNode* rootNode) const;

		/// @brief 按名称查找灯光节点（供运行时控制，如聚光灯跟随相机）
		/// @param rootNode 场景根节点
		/// @param name 节点名称
		/// @return 找到的节点指针，未找到返回 nullptr
		static SceneNode* findNodeByName(SceneNode* rootNode, const std::string& name);

	private:
		/// @brief 递归收集所有 LightComponent
		void collectLights(SceneNode* node, std::vector<std::pair<SceneNode*, LightComponent*>>& outLights) const;
	};

}
