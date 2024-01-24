#pragma once

#include "DynamicLight.h"

#include <string>

namespace engine {
	namespace graphics {

		struct PointLight : public DynamicLight {
		public:
			PointLight(glm::vec3& amb, glm::vec3& diff, glm::vec3& spec, glm::vec3& pos, float cons, float lin, float quad);

			PointLight(const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec, const glm::vec3& pos, float cons, float lin, float quad);

			virtual void setupUniforms(Shader& shader, int currentLightIndex) override;


			glm::vec3 position;
			float constant, linear, quadratic;
		};

	}
}