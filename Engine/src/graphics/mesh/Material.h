#pragma once
#include <GL\glew.h>
#include <GLFW\glfw3.h>

#include "../Shader.h"
#include "../texture/Texture.h"
#include "../../utils/loaders/TextureLoader.h"

namespace engine {
	namespace graphics {
		class Material
		{
		public:
			Material(Texture* diffuseMap = nullptr, Texture* specularMap = nullptr, Texture* normalMap = nullptr, Texture* emissionMap = nullptr, float shininess = 128.0f);

			void BindMaterialInformation(Shader& shader) const;

			inline void setDiffuseMap(Texture* texture) { m_DiffuseMap = texture; }
			inline void setSpecularMap(Texture* texture) { m_SpecularMap = texture; }
			inline void setNormalMap(Texture* texture) { m_NormalMap = texture; }
			inline void setEmissionMap(Texture* texture) { m_EmissionMap = texture; }

			inline void setShininess(float shininess) { m_Shininess = shininess; }
		private:
			Texture* m_DiffuseMap, * m_SpecularMap, * m_NormalMap, * m_EmissionMap;
			float m_Shininess;

		};
	}
}


