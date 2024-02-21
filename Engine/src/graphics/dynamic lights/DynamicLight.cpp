#include "pch.h"
#include "DynamicLight.h"

namespace engine {

	DynamicLight::DynamicLight(const glm::vec3& lightColor)
		: lightColor(lightColor), isActive(false) {}

}