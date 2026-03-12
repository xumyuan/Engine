#include "pch.h"
#include "DirectionalLight.h"

namespace engine {
	DirectionalLight::DirectionalLight(const glm::vec3& lightColor, const glm::vec3& dir)
		: DynamicLight(lightColor), direction(dir) {
	}
}
