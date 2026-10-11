#include "pch.h"
#include "TypeProcess.h"
#include <stdexcept>

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
	void from_json(const json& j, SceneInfo::CameraInfo& c) {
		if (!j.is_object()) {
			throw std::invalid_argument("camera must be an object");
		}
		if (j.contains("position")) {
			const auto& position = j.at("position");
			if (!position.is_array() || position.size() != 3) {
				throw std::invalid_argument("camera.position must contain exactly 3 numbers");
			}
			for (const auto& coordinate : position) {
				if (!coordinate.is_number()) {
					throw std::invalid_argument("camera.position must contain exactly 3 numbers");
				}
			}
			position.get_to(c.position);
		}
		for (const char* angle : { "yaw", "pitch" }) {
			if (j.contains(angle) && !j.at(angle).is_number()) {
				throw std::invalid_argument(std::string("camera.") + angle + " must be a number");
			}
		}
		c.yaw = j.value("yaw", c.yaw);
		c.pitch = j.value("pitch", c.pitch);
		if (!std::isfinite(c.position.x) || !std::isfinite(c.position.y) ||
			!std::isfinite(c.position.z) || !std::isfinite(c.yaw) || !std::isfinite(c.pitch)) {
			throw std::invalid_argument("camera position and angles must be finite");
		}
		if (c.pitch < -89.0f || c.pitch > 89.0f) {
			throw std::invalid_argument("camera.pitch must be between -89 and 89 degrees");
		}
	}

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

	void from_json(const json& j, SceneInfo::FluidInfo& f) {
		// 出现 "fluid" 字段即视为启用，可用 isActive=false 临时关闭
		f.isActive = j.value("isActive", true);
		j.at("maxParticles").get_to(f.maxParticles);
		const auto& boundary = j.at("boundary");
		boundary.at("min").get_to(f.boundaryMin);
		boundary.at("max").get_to(f.boundaryMax);
		f.autoStart = j.value("autoStart", true);
		f.backend = j.value("backend", std::string("cpu"));
		if (f.backend != "cpu" && f.backend != "compute") {
			throw std::invalid_argument("fluid.backend must be 'cpu' or 'compute'");
		}
	}

	void from_json(const json& j, SceneInfo::EnvironmentInfo& e) {
		if (j.contains("skybox")) {
			j.at("skybox").get_to(e.skybox);
		}
	}

	void from_json(const json& j, SceneInfo::SystemsInfo& s) {
		if (j.contains("fluid")) {
			j.at("fluid").get_to(s.fluid);
		}
	}

	void from_json(const json& j, SceneInfo& s) {
		s.version = j.value("version", 1);

		if (j.contains("camera")) {
			j.at("camera").get_to(s.camera);
		}

		// 新格式：按场景职责分组。
		if (j.contains("entities")) {
			j.at("entities").get_to(s.entities);
		}
		if (j.contains("environment")) {
			j.at("environment").get_to(s.environment);
		}
		if (j.contains("lighting")) {
			j.at("lighting").get_to(s.lighting);
		}
		if (j.contains("systems")) {
			j.at("systems").get_to(s.systems);
		}

		// 旧格式兼容：允许旧场景逐步迁移到新结构。
		if (!j.contains("entities") && j.contains("modelList")) {
			j.at("modelList").get_to(s.entities);
		}
		if (!j.contains("environment") && j.contains("skybox")) {
			j.at("skybox").get_to(s.environment.skybox);
		}
		if (!j.contains("lighting") && j.contains("lights")) {
			j.at("lights").get_to(s.lighting);
		}
		if (!j.contains("systems") && j.contains("fluid")) {
			j.at("fluid").get_to(s.systems.fluid);
		}
	}
}
