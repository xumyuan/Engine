#include "DynamicLight.h"

namespace engine {
	namespace graphics {

		DynamicLight::DynamicLight(const glm::vec3& lightColor)
			: lightColor(lightColor), isActive(false) {}

	}
}