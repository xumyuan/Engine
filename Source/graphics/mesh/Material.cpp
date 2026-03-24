#include "pch.h"
#include "Material.h"
#include "graphics/Window.h"
#include "rhi/include/RHICommandBuffer.h"

#include <unordered_set>

namespace {
	constexpr int kMaterialTextureUnitBase = 4;
	constexpr int kMaterialTextureAlbedoUnit = 4;
	constexpr int kMaterialTextureNormalUnit = 5;
	constexpr int kMaterialTextureMetallicUnit = 6;
	constexpr int kMaterialTextureRoughnessUnit = 7;
	constexpr int kMaterialTextureAOUnit = 8;
	constexpr int kMaterialTextureDisplacementUnit = 9;
	constexpr int kMaterialTextureEmissionUnit = 10;

	void setLegacyMaterialSamplerBindings(engine::Shader* shader) {
		shader->setUniform("material.texture_albedo", kMaterialTextureAlbedoUnit);
		shader->setUniform("material.texture_normal", kMaterialTextureNormalUnit);
		shader->setUniform("material.texture_metallic", kMaterialTextureMetallicUnit);
		shader->setUniform("material.texture_roughness", kMaterialTextureRoughnessUnit);
		shader->setUniform("material.texture_ao", kMaterialTextureAOUnit);
		shader->setUniform("material.texture_displacement", kMaterialTextureDisplacementUnit);
		shader->setUniform("material.texture_emission", kMaterialTextureEmissionUnit);
	}

	void setModernMaterialSamplerBindings(engine::Shader* shader) {
		shader->setUniform("texture_albedo", kMaterialTextureAlbedoUnit);
		shader->setUniform("texture_normal", kMaterialTextureNormalUnit);
		shader->setUniform("texture_metallic", kMaterialTextureMetallicUnit);
		shader->setUniform("texture_roughness", kMaterialTextureRoughnessUnit);
		shader->setUniform("texture_ao", kMaterialTextureAOUnit);
		shader->setUniform("texture_displacement", kMaterialTextureDisplacementUnit);
		shader->setUniform("texture_emission", kMaterialTextureEmissionUnit);
	}

	void setLegacyMaterialSamplerBindingsOnce(engine::Shader* shader) {
		if (!shader) {
			return;
		}

		static std::unordered_set<engine::rhi::HandleBase::HandleId> s_InitializedLegacyPrograms;
		const auto program = shader->getProgramHandle();
		if (program) {
			const auto [_, inserted] = s_InitializedLegacyPrograms.emplace(program.getId());
			if (!inserted) {
				return;
			}
		}
		setLegacyMaterialSamplerBindings(shader);
	}

	void setModernMaterialSamplerBindingsOnce(engine::Shader* shader) {
		if (!shader) {
			return;
		}

		static std::unordered_set<engine::rhi::HandleBase::HandleId> s_InitializedModernPrograms;
		const auto program = shader->getProgramHandle();
		if (program) {
			const auto [_, inserted] = s_InitializedModernPrograms.emplace(program.getId());
			if (!inserted) {
				return;
			}
		}
		setModernMaterialSamplerBindings(shader);
	}

	void setModernMaterialSamplerBindingsOnce(engine::rhi::CommandBuffer& cmd, engine::rhi::ProgramHandle program) {
		static std::unordered_set<engine::rhi::HandleBase::HandleId> s_InitializedModernPrograms;
		if (program) {
			const auto [_, inserted] = s_InitializedModernPrograms.emplace(program.getId());
			if (!inserted) {
				return;
			}
		}

		cmd.setUniformInt(program, "texture_albedo", kMaterialTextureAlbedoUnit);
		cmd.setUniformInt(program, "texture_normal", kMaterialTextureNormalUnit);
		cmd.setUniformInt(program, "texture_metallic", kMaterialTextureMetallicUnit);
		cmd.setUniformInt(program, "texture_roughness", kMaterialTextureRoughnessUnit);
		cmd.setUniformInt(program, "texture_ao", kMaterialTextureAOUnit);
		cmd.setUniformInt(program, "texture_displacement", kMaterialTextureDisplacementUnit);
		cmd.setUniformInt(program, "texture_emission", kMaterialTextureEmissionUnit);
	}
}

namespace engine {
	Material::Material(Texture* albedoMap, Texture* normalMap, Texture* metallicMap, Texture* roughnessMap, Texture* ambientOcclusionMap, Texture* emissionMap)
		: m_AlbedoMap(albedoMap), m_NormalMap(normalMap), m_MetallicMap(metallicMap), m_RoughnessMap(roughnessMap), m_AmbientOcclusionMap(ambientOcclusionMap), m_EmissionMap(emissionMap) {
	}


	void Material::BindMaterialInformation(Shader* shader) const {
		// 纹理单元0留给shadowmap
		// 纹理单元 1 保留用于用于间接漫反射 IBL 的 irradianceMap
		// 纹理单元 2 保留给 prefilterMap
		// 纹理单元 3 保留给 brdfLUT
		int currentTextureUnit = kMaterialTextureUnitBase;
		setLegacyMaterialSamplerBindingsOnce(shader);

		shader->setUniform("material.albedoColour", m_AlbedoColour);
		if (m_AlbedoMap) {
			m_AlbedoMap->bind(currentTextureUnit++);
			shader->setUniform("material.hasAlbedoTexture", true);
		}
		else {
			TextureLoader::getDefaultAlbedo()->bind(currentTextureUnit++);
			shader->setUniform("material.hasAlbedoTexture", false);
		}

		// 传递材质参数到着色器
		shader->setUniform("material.metallicValue", m_MetallicValue);
		shader->setUniform("material.roughnessValue", m_RoughnessValue);
		shader->setUniform("material.emissionColour", m_EmissionColour);
		shader->setUniform("material.emissionIntensity", m_EmissionIntensity);
		shader->setUniform("parallaxStrength", m_ParallaxStrength);

		if (m_NormalMap) {
			m_NormalMap->bind(currentTextureUnit++);
		}
		else {
			TextureLoader::getDefaultNormal()->bind(currentTextureUnit++);
		}

		if (m_MetallicMap) {
			m_MetallicMap->bind(currentTextureUnit++);
			shader->setUniform("material.hasMetallicTexture", true);

		}
		else {
			TextureLoader::getDefaultMetallic()->bind(currentTextureUnit++);
			shader->setUniform("material.hasMetallicTexture", false);
		}

		if (m_RoughnessMap) {
			m_RoughnessMap->bind(currentTextureUnit++);
			shader->setUniform("material.hasRoughnessTexture", true);

		}
		else {
			TextureLoader::getDefaultRoughness()->bind(currentTextureUnit++);
			shader->setUniform("material.hasRoughnessTexture", false);

		}

		if (m_AmbientOcclusionMap) {
			m_AmbientOcclusionMap->bind(currentTextureUnit++);
		}
		else {
			TextureLoader::getDefaultAO()->bind(currentTextureUnit++);
		}

		// Bind displacement texture (currently not used in forward rendering, but keeping consistency)
		TextureLoader::getDefaultNormal()->bind(currentTextureUnit++); // Use default for now

		if (m_EmissionMap) {
			m_EmissionMap->bind(currentTextureUnit++);
			shader->setUniform("material.hasEmissionTexture", true);
		}
		else {
			TextureLoader::getDefaultEmission()->bind(currentTextureUnit++);
			shader->setUniform("material.hasEmissionTexture", false);
		}
	}

	// ===== UBO 新接口 =====
	void Material::fillMaterialUBO(UBOMaterialParams& params) const {
		params.albedoColour = m_AlbedoColour;
		params.emissionColour = glm::vec4(m_EmissionColour, m_EmissionIntensity);
		params.metallicValue = m_MetallicValue;
		params.roughnessValue = m_RoughnessValue;
		params.parallaxStrength = m_ParallaxStrength;
		params.tilingAmount = 1.0f;  // default
		params.hasAlbedoTexture = m_AlbedoMap ? 1 : 0;
		params.hasMetallicTexture = m_MetallicMap ? 1 : 0;
		params.hasRoughnessTexture = m_RoughnessMap ? 1 : 0;
		params.hasEmissionTexture = m_EmissionMap ? 1 : 0;
		params.hasDisplacement = 0;
		params.hasEmission = (m_EmissionMap || glm::length(m_EmissionColour) > 0.001f) ? 1 : 0;
		params.minMaxDisplacementSteps = glm::vec2(8.0f, 32.0f);
	}

	void Material::bindMaterialTextures(Shader* shader) const {
		// 纹理单元分配与旧接口一致
		// 0: shadowmap, 1: irradianceMap, 2: prefilterMap, 3: brdfLUT
		int currentTextureUnit = kMaterialTextureUnitBase;
		setModernMaterialSamplerBindingsOnce(shader);

		if (m_AlbedoMap) {
			m_AlbedoMap->bind(currentTextureUnit++);
		} else {
			TextureLoader::getDefaultAlbedo()->bind(currentTextureUnit++);
		}

		if (m_NormalMap) {
			m_NormalMap->bind(currentTextureUnit++);
		} else {
			TextureLoader::getDefaultNormal()->bind(currentTextureUnit++);
		}

		if (m_MetallicMap) {
			m_MetallicMap->bind(currentTextureUnit++);
		} else {
			TextureLoader::getDefaultMetallic()->bind(currentTextureUnit++);
		}

		if (m_RoughnessMap) {
			m_RoughnessMap->bind(currentTextureUnit++);
		} else {
			TextureLoader::getDefaultRoughness()->bind(currentTextureUnit++);
		}

		if (m_AmbientOcclusionMap) {
			m_AmbientOcclusionMap->bind(currentTextureUnit++);
		} else {
			TextureLoader::getDefaultAO()->bind(currentTextureUnit++);
		}

		TextureLoader::getDefaultNormal()->bind(currentTextureUnit++);

		if (m_EmissionMap) {
			m_EmissionMap->bind(currentTextureUnit++);
		} else {
			TextureLoader::getDefaultEmission()->bind(currentTextureUnit++);
		}
	}

	void Material::bindMaterialTextures(rhi::CommandBuffer& cmd, rhi::ProgramHandle program) const {
		// 纹理单元分配与旧接口一致
		// 0: shadowmap, 1: irradianceMap, 2: prefilterMap, 3: brdfLUT
		int currentTextureUnit = kMaterialTextureUnitBase;
		setModernMaterialSamplerBindingsOnce(cmd, program);

		if (m_AlbedoMap) {
			cmd.bindTextureUnit(m_AlbedoMap->getRHIHandle(), currentTextureUnit++);
		} else {
			cmd.bindTextureUnit(TextureLoader::getDefaultAlbedo()->getRHIHandle(), currentTextureUnit++);
		}

		if (m_NormalMap) {
			cmd.bindTextureUnit(m_NormalMap->getRHIHandle(), currentTextureUnit++);
		} else {
			cmd.bindTextureUnit(TextureLoader::getDefaultNormal()->getRHIHandle(), currentTextureUnit++);
		}

		if (m_MetallicMap) {
			cmd.bindTextureUnit(m_MetallicMap->getRHIHandle(), currentTextureUnit++);
		} else {
			cmd.bindTextureUnit(TextureLoader::getDefaultMetallic()->getRHIHandle(), currentTextureUnit++);
		}

		if (m_RoughnessMap) {
			cmd.bindTextureUnit(m_RoughnessMap->getRHIHandle(), currentTextureUnit++);
		} else {
			cmd.bindTextureUnit(TextureLoader::getDefaultRoughness()->getRHIHandle(), currentTextureUnit++);
		}

		if (m_AmbientOcclusionMap) {
			cmd.bindTextureUnit(m_AmbientOcclusionMap->getRHIHandle(), currentTextureUnit++);
		} else {
			cmd.bindTextureUnit(TextureLoader::getDefaultAO()->getRHIHandle(), currentTextureUnit++);
		}

		cmd.bindTextureUnit(TextureLoader::getDefaultNormal()->getRHIHandle(), currentTextureUnit++);

		if (m_EmissionMap) {
			cmd.bindTextureUnit(m_EmissionMap->getRHIHandle(), currentTextureUnit++);
		} else {
			cmd.bindTextureUnit(TextureLoader::getDefaultEmission()->getRHIHandle(), currentTextureUnit++);
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
