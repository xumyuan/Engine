#pragma once
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include "../Shader.h"

namespace engine {
	namespace graphics {
		class Material
		{//����PBR
		public:
			Material(unsigned int diffuseMap = 0, unsigned int specularMap = 0, unsigned int normalMap = 0, unsigned int emissionMap = 0, float shininess = 128.0f);

			void BindMaterialInformation(Shader& shader) const;

			inline unsigned int getDiffuseMapId() { return m_DiffuseMap; }
			inline unsigned int getSpecularMapId() { return m_SpecularMap; }
			inline unsigned int getNormalMapId() { return m_NormalMap; }
			inline unsigned int getEmissionMapId() { return m_EmissionMap; }
			inline float getShininess() { return m_Shininess; }

			inline void setDiffuseMapId(unsigned int id) { m_DiffuseMap = id; }
			inline void setSpecularMapId(unsigned int id) { m_SpecularMap = id; }
			inline void setNormalMapId(unsigned int id) { m_NormalMap = id; }
			inline void setEmissionMapId(unsigned int id) { m_EmissionMap = id; }
			inline void setShininess(float shininess) { m_Shininess = shininess; }
		private:
			unsigned int m_DiffuseMap, m_SpecularMap, m_NormalMap, m_EmissionMap;
			float m_Shininess;

		};
	}
}


