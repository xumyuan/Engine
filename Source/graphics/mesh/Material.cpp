#include "pch.h"
#include "Material.h"
#include "graphics/Window.h"
#include "graphics/TextureBindings.h"
#include "rhi/include/RHICommandBuffer.h"

namespace {
	// 纹理异步加载，上传前或加载失败时 isGenerated() 为 false；绑定它不会生效，纹理单元会残留上一次的绑定
	bool isReady(const engine::Texture* texture) {
		return texture != nullptr && texture->isGenerated();
	}

	engine::rhi::TextureHandle readyOr(const engine::Texture* texture, const engine::Texture* fallback) {
		return (isReady(texture) ? texture : fallback)->getRHIHandle();
	}
}

namespace engine {
	Material::Material(Texture* albedoMap, Texture* normalMap, Texture* metallicMap, Texture* roughnessMap, Texture* ambientOcclusionMap, Texture* emissionMap)
		: m_AlbedoMap(albedoMap), m_NormalMap(normalMap), m_MetallicMap(metallicMap), m_RoughnessMap(roughnessMap), m_AmbientOcclusionMap(ambientOcclusionMap), m_EmissionMap(emissionMap) {
	}

	void Material::fillMaterialUBO(UBOMaterialParams& params) const {
		params.albedoColour = m_AlbedoColour;
		params.emissionColour = glm::vec4(m_EmissionColour, m_EmissionIntensity);
		params.metallicValue = m_MetallicValue;
		params.roughnessValue = m_RoughnessValue;
		params.parallaxStrength = m_ParallaxStrength;
		params.tilingAmount = 1.0f;  // default
		params.hasAlbedoTexture = isReady(m_AlbedoMap) ? 1 : 0;
		params.hasMetallicTexture = isReady(m_MetallicMap) ? 1 : 0;
		params.hasRoughnessTexture = isReady(m_RoughnessMap) ? 1 : 0;
		params.hasEmissionTexture = isReady(m_EmissionMap) ? 1 : 0;
		params.hasDisplacement = 0;
		params.hasEmission = (isReady(m_EmissionMap) || glm::length(m_EmissionColour) > 0.001f) ? 1 : 0;
		params.minMaxDisplacementSteps = glm::vec2(8.0f, 32.0f);
	}

	void Material::bindMaterialTextures(rhi::CommandBuffer& cmd) const {
		// 0-3 号单元留给阴影贴图与 IBL，见 TextureBindings.h
		cmd.bindTextureUnit(readyOr(m_AlbedoMap, TextureLoader::getDefaultAlbedo()), TextureUnit::MaterialAlbedo);
		cmd.bindTextureUnit(readyOr(m_NormalMap, TextureLoader::getDefaultNormal()), TextureUnit::MaterialNormal);
		cmd.bindTextureUnit(readyOr(m_MetallicMap, TextureLoader::getDefaultMetallic()), TextureUnit::MaterialMetallic);
		cmd.bindTextureUnit(readyOr(m_RoughnessMap, TextureLoader::getDefaultRoughness()), TextureUnit::MaterialRoughness);
		cmd.bindTextureUnit(readyOr(m_AmbientOcclusionMap, TextureLoader::getDefaultAO()), TextureUnit::MaterialAO);
		// 位移贴图暂未支持，占位绑定默认法线
		cmd.bindTextureUnit(TextureLoader::getDefaultNormal()->getRHIHandle(), TextureUnit::MaterialDisplacement);
		cmd.bindTextureUnit(readyOr(m_EmissionMap, TextureLoader::getDefaultEmission()), TextureUnit::MaterialEmission);
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
