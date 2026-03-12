#include "pch.h"
#include "PointLight.h"

namespace engine {

	PointLight::PointLight(const glm::vec3& lightColor, const glm::vec3& pos)
		: DynamicLight(lightColor), position(pos) {}

}