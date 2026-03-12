#include "pch.h"
#include "SpotLight.h"

namespace engine {

	SpotLight::SpotLight(const glm::vec3& lightColor, const glm::vec3& pos, const glm::vec3& dir, float cutOffAngle, float outerCutOffAngle)
		: DynamicLight(lightColor), position(pos), direction(dir), cutOff(cutOffAngle), outerCutOff(outerCutOffAngle) {
	}

}
