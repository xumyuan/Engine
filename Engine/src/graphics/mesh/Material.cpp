#include "pch.h"
#include "Material.h"
#include "graphics/Window.h"

namespace engine {

	Material::Material(Texture* albedoMap, Texture* normalMap, Texture* metallicMap, Texture* roughnessMap, Texture* ambientOcclusionMap, Texture* emissionMap)
		: m_AlbedoMap(albedoMap), m_NormalMap(normalMap), m_MetallicMap(metallicMap), m_RoughnessMap(roughnessMap), m_AmbientOcclusionMap(ambientOcclusionMap), m_EmissionMap(emissionMap) {}


	void Material::BindMaterialInformation(Shader* shader) const {
		// ����Ԫ0����shadowmap
		// ����Ԫ 1 �����������ڼ�������� IBL �� irradianceMap
		// ����Ԫ 2 ������ prefilterMap
		// ����Ԫ 3 ������ brdfLUT
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

}