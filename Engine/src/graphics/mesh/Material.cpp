#include "pch.h"
#include "Material.h"
#include "graphics/Window.h"

namespace engine {

	Material::Material(Texture* albedoMap, Texture* normalMap, Texture* metallicMap, Texture* roughnessMap, Texture* ambientOcclusionMap, Texture* emissionMap)
		: m_AlbedoMap(albedoMap), m_NormalMap(normalMap), m_MetallicMap(metallicMap), m_RoughnessMap(roughnessMap), m_AmbientOcclusionMap(ambientOcclusionMap), m_EmissionMap(emissionMap) {}


	void Material::BindMaterialInformation(Shader* shader) const {
		// 纹理单元0留给shadowmap
		// 纹理单元 1 保留用于用于间接漫反射 IBL 的 irradianceMap
		// 纹理单元 2 保留给 prefilterMap
		// 纹理单元 3 保留给 brdfLUT
		int currentTextureUnit = 4;

		shader->setUniform("material.albedoColour", m_AlbedoColour);
		shader->setUniform("material.texture_albedo", currentTextureUnit);
		if (m_AlbedoMap) {
			m_AlbedoMap->bind(currentTextureUnit++);
			shader->setUniform("material.hasAlbedoTexture", true);
		}
		else {
			TextureLoader::getDefaultAlbedo()->bind(currentTextureUnit++);
			shader->setUniform("material.hasAlbedoTexture", false);
		}



		shader->setUniform("material.texture_normal", currentTextureUnit);
		if (m_NormalMap) {
			m_NormalMap->bind(currentTextureUnit++);
		}
		else {
			TextureLoader::getDefaultNormal()->bind(currentTextureUnit++);
		}

		shader->setUniform("material.texture_metallic", currentTextureUnit);
		if (m_MetallicMap) {
			m_MetallicMap->bind(currentTextureUnit++);
			shader->setUniform("material.hasMetallicTexture", true);

		}
		else {
			TextureLoader::getDefaultMetallic()->bind(currentTextureUnit++);
			shader->setUniform("material.hasMetallicTexture", false);
		}

		shader->setUniform("material.texture_roughness", currentTextureUnit);
		if (m_RoughnessMap) {
			m_RoughnessMap->bind(currentTextureUnit++);
			shader->setUniform("material.hasRoughnessTexture", true);

		}
		else {
			TextureLoader::getDefaultRoughness()->bind(currentTextureUnit++);
			shader->setUniform("material.hasRoughnessTexture", false);

		}

		shader->setUniform("material.texture_ao", currentTextureUnit);
		if (m_AmbientOcclusionMap) {
			m_AmbientOcclusionMap->bind(currentTextureUnit++);
		}
		else {
			TextureLoader::getDefaultAO()->bind(currentTextureUnit++);
		}

		shader->setUniform("material.texture_emission", currentTextureUnit);
		if (m_EmissionMap) {
			m_EmissionMap->bind(currentTextureUnit++);
		}
		else {
			TextureLoader::getDefaultEmission()->bind(currentTextureUnit++);
		}
	}

	void Material::processMaterial(const SceneInfo::ModelInfo& modelinfo) {
		const std::unordered_map<
			std::string,
			std::function<void(const std::variant<std::string, glm::vec4, float, bool>&)>
		> matHandlers = {
			{"NormalMap",[&](const auto& value) {
				if (auto path = std::get_if<std::string>(&value)) {
					m_NormalMap = TextureLoader::load2DTexture(*path);
				}
			} },
			{ "AlbedoMap",[&](const auto& value) {
				if (auto path = std::get_if<std::string>(&value)) {
					TextureSettings srgbSettings;
					srgbSettings.IsSRGB = true;
					m_AlbedoMap = TextureLoader::load2DTexture(*path, &srgbSettings);
				}
			} },
			{ "MetallicMap",[&](const auto& value) {
				if (auto path = std::get_if<std::string>(&value)) {
					m_MetallicMap = TextureLoader::load2DTexture(*path);
				}
			} },
			{ "RoughnessMap",[&](const auto& value) {
				if (auto path = std::get_if<std::string>(&value)) {
					m_RoughnessMap = TextureLoader::load2DTexture(*path);
				}
			} },
			{ "AmbientOcclusionMap",[&](const auto& value) {
				if (auto path = std::get_if<std::string>(&value)) {
					m_AmbientOcclusionMap = TextureLoader::load2DTexture(*path);
				}
			} },
			{ "EmissionMap",[&](const auto& value) {
				if (auto path = std::get_if<std::string>(&value)) {
					m_EmissionMap = TextureLoader::load2DTexture(*path);
				}
			}},
			{"AlbedoColour",[&](const auto& value) {
				if (auto color = std::get_if<glm::vec4>(&value)) {
					m_AlbedoColour = *color;
				}
			}},
			{"MetallicValue",[&](const auto& value) {
				if (auto metallicValue = std::get_if<float>(&value)) {
					m_MetallicValue = *metallicValue;
				}
			}},
			{"RoughnessValue",[&](const auto& value) {
				if (auto roughnessValue = std::get_if<float>(&value)) {
					m_RoughnessValue = *roughnessValue;
				}
			}},
			{"ParallaxStrength",[&](const auto& value) {
				if (auto parallaxStrength = std::get_if<float>(&value)) {
					m_ParallaxStrength = *parallaxStrength;
				}
			}},
		};
	
		for (const auto& [key, value] : modelinfo.customMatTexList) {
			if (auto handler = matHandlers.find(key); handler != matHandlers.end()) {
				handler->second(value);
			}
		}
	}
}