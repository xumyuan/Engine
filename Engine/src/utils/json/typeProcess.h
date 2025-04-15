#pragma once
#include "pch.h"
#include "json_type.h"

namespace glm {
	// Ϊglm::vec3���JSON���л�֧��
	void from_json(const nlohmann::json& j, vec3& v);

	void from_json(const nlohmann::json& j, vec4& v);
}

namespace engine {
	// Ϊ�Զ����������JSON���л�֧��
	void from_json(const nlohmann::json& j, SceneInfo::ModelInfo& m);
	void from_json(const nlohmann::json& j, SceneInfo::SkyboxInfo& s);
	void from_json(const nlohmann::json& j, SceneInfo& s);
}

