#pragma once
#include "graphics/Shader.h"
#include "graphics/texture/Texture.h"
#include "utils/loaders/TextureLoader.h"
#include "utils/json/json_type.h"

namespace engine {
	class Material
	{
	public:
		Material(Texture* albedoMap = nullptr, Texture* normalMap = nullptr, Texture* metallicMap = nullptr, Texture* roughnessMap = nullptr,
			Texture* ambientOcclusionMap = nullptr, Texture* emissionMap = nullptr);

		void BindMaterialInformation(Shader* shader) const;

		void processMaterial(const SceneInfo::ModelInfo& modelinfo);

		inline void setAlbedoMap(Texture* texture) { m_AlbedoMap = texture; }
		inline void setNormalMap(Texture* texture) { m_NormalMap = texture; }
		inline void setMetallicMap(Texture* texture) { m_MetallicMap = texture; }
		inline void setRoughnessMap(Texture* texture) { m_RoughnessMap = texture; }
		inline void setAmbientOcclusionMap(Texture* texture) { m_AmbientOcclusionMap = texture; }
		inline void setEmissionMap(Texture* texture) { m_EmissionMap = texture; }


		inline void SetAlbedoColour(glm::vec4 value) { m_AlbedoColour = value; }

	private:
		Texture* m_AlbedoMap, * m_NormalMap, * m_MetallicMap, * m_RoughnessMap, * m_AmbientOcclusionMap, * m_EmissionMap;
		float m_ParallaxStrength = 0.07f;
		glm::vec4 m_AlbedoColour = glm::vec4(0.894f, 0.023f, 0.992f, 1.0f);
		float m_MetallicValue = 0.0f, m_RoughnessValue = 0.0f;

		// Emission values
		float m_EmissionIntensity = 1.0f;
		glm::vec3 m_EmissionColour = glm::vec3(0.0f, 0.0f, 0.0f);
	};
}


