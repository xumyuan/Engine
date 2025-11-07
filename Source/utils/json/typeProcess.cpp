#include "pch.h"
#include "typeProcess.h"

namespace glm {
	using json = nlohmann::json;
	// 为glm::vec3添加JSON序列化支持
	void from_json(const json& j, vec3& v) {
		v.x = j[0].get<float>();
		v.y = j[1].get<float>();
		v.z = j[2].get<float>();
	}

	void from_json(const json& j, vec4& v) {
		v.x = j[0].get<float>();
		v.y = j[1].get<float>();
		v.z = j[2].get<float>();
		v.w = j[3].get<float>();
	}
}

namespace engine {
	using json = nlohmann::json;
	void from_json(const json& j, SceneInfo::ModelInfo& m) {
		j.at("modelPath").get_to(m.modelPath);
		j.at("position").get_to(m.position);
		j.at("scale").get_to(m.scale);
		j.at("rotationAxis").get_to(m.rotationAxis);
		j.at("radianRotation").get_to(m.radianRotation);
		j.at("isStatic").get_to(m.isStatic);
		j.at("isTransparent").get_to(m.isTransparent);

		// 解析自定义材质纹理列表
		if (j.contains("customMatTexList"))
		{
			for (auto& [key, value] : j.at("customMatTexList").items()) {
				if (value.is_string()) {
					m.customMatTexList[key] = value.get<std::string>();
				}
				else if (value.is_array() && value.size() == 4) {
					m.customMatTexList[key] = value.get<glm::vec4>();
				}
				else if (value.is_boolean()) {
					m.customMatTexList[key] = value.get<bool>();
				}
				else if (value.is_number()) {
					m.customMatTexList[key] = value.get<float>();
				}
				else {
					// 处理不支持的类型
					spdlog::error("Unsupported type in customMatTexList for key:{}", key);
					//throw json::type_error::create(302,"Unsupported type in customMatTexList for key: " + key);
				}
			}
		}
	}

	void from_json(const json& j, SceneInfo::SkyboxInfo& s) {
		// 确保天空盒图片顺序与渲染器要求一致
		s.skyboxFilePaths = {
			j.at("right").get<std::string>(),
			j.at("left").get<std::string>(),
			j.at("top").get<std::string>(),
			j.at("bottom").get<std::string>(),
			j.at("back").get<std::string>(),
			j.at("front").get<std::string>()
		};
	}

	void from_json(const json& j, SceneInfo::LightsInfo::DirectionalLight& d) {
		j.at("isActive").get_to(d.isActive);
		j.at("direction").get_to(d.direction);
		j.at("lightColor").get_to(d.lightColor);
	}

	void from_json(const json& j, SceneInfo::LightsInfo::SpotLight& s) {
		j.at("isActive").get_to(s.isActive);
		j.at("lightColor").get_to(s.lightColor);
		j.at("position").get_to(s.position);
		j.at("direction").get_to(s.direction);
		j.at("cutOff").get_to(s.cutOff);
		j.at("outerCutOff").get_to(s.outerCutOff);
	}

	void from_json(const json& j, SceneInfo::LightsInfo::PointLight& p) {
		j.at("isActive").get_to(p.isActive);
		j.at("position").get_to(p.position);
		j.at("lightColor").get_to(p.lightColor);
	}

	void from_json(const json& j, SceneInfo::LightsInfo& l) {
		if (j.contains("directionalLight")) {
			j.at("directionalLight").get_to(l.directionalLight);
		}
		if (j.contains("spotLight")) {
			j.at("spotLight").get_to(l.spotLight);
		}
		if (j.contains("pointLight")) {
			j.at("pointLight").get_to(l.pointLightList);
		}
	}

	void from_json(const json& j, SceneInfo& s) {
		// 解析模型列表，可选字段
		if (j.contains("modelList")) {
			j.at("modelList").get_to(s.modelInfoList);
		}
		
		// 解析天空盒信息，可选字段
		if (j.contains("skybox")) {
			j.at("skybox").get_to(s.skyboxInfo);
		}
		
		// 解析灯光信息，可选字段
		if (j.contains("lights")) {
			j.at("lights").get_to(s.lightsInfo);
		}
	}
}
