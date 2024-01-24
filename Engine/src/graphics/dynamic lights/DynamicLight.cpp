#include "DynamicLight.h"

namespace engine {
	namespace graphics {

		DynamicLight::DynamicLight(glm::vec3& amb, glm::vec3& diff, glm::vec3& spec)
			: ambient(amb), diffuse(diff), specular(spec), isActive(false) {}

		DynamicLight::DynamicLight(const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec)
			: ambient(amb), diffuse(diff), specular(spec), isActive(false) {}

	}
}