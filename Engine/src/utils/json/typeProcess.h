#pragma once
#include "pch.h"
#include "json_type.h"

namespace glm {
	// 为glm::vec3添加JSON序列化支持
	void from_json(const nlohmann::json& j, vec3& v);

	void from_json(const nlohmann::json& j, vec4& v);
}

namespace engine {
	// 为自定义类型添加JSON序列化支持
	void from_json(const nlohmann::json& j, SceneInfo::ModelInfo& m);
	void from_json(const nlohmann::json& j, SceneInfo::SkyboxInfo& s);
	void from_json(const nlohmann::json& j, SceneInfo& s);
}

