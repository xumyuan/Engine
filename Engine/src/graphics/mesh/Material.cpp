#include "Material.h"

namespace engine {
	namespace graphics {

		Material::Material(unsigned int diffuseMap, unsigned int specularMap, unsigned int normalMap, unsigned int emissionMap, float shininess)
			: m_DiffuseMap(diffuseMap), m_SpecularMap(specularMap), m_NormalMap(normalMap), m_EmissionMap(emissionMap), m_Shininess(shininess) {}


		void Material::BindMaterialInformation(Shader& shader) const {
			int currentTextureUnit = 0;
			// Todo:这里修复贴图bug的方式不太行？
			if (m_DiffuseMap > 0) {
				glActiveTexture(GL_TEXTURE0);
				shader.setUniform1i("material.texture_diffuse", currentTextureUnit++);
				glBindTexture(GL_TEXTURE_2D, m_DiffuseMap);
			}
			else {
				shader.setUniform1i("material.texture_diffuse", 0);
			}

			if (m_SpecularMap > 0) {
				glActiveTexture(GL_TEXTURE0 + currentTextureUnit);
				shader.setUniform1i("material.texture_specular", currentTextureUnit++);
				glBindTexture(GL_TEXTURE_2D, m_SpecularMap);
			}
			else {
				shader.setUniform1i("material.texture_specular", 0);
			}

			if (m_NormalMap > 0) {
				glActiveTexture(GL_TEXTURE0 + currentTextureUnit);
				shader.setUniform1i("material.texture_normal", currentTextureUnit++);
				glBindTexture(GL_TEXTURE_2D, m_NormalMap);
			}
			if (m_EmissionMap > 0) {
				glActiveTexture(GL_TEXTURE0 + currentTextureUnit);
				shader.setUniform1i("material.texture_emission", currentTextureUnit++);
				glBindTexture(GL_TEXTURE_2D, m_EmissionMap);
			}

			shader.setUniform1f("material.shininess", m_Shininess);
		}

	}
}