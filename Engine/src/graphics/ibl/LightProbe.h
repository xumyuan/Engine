#pragma once

#include "graphics/Shader.h"
#include "graphics/texture/Cubemap.h"

namespace engine {

	class LightProbe {
	public:
		LightProbe(glm::vec3& probePosition, glm::vec2& probeResolution);
		~LightProbe();
		void generate();

		// Assumes the shader is bound
		void bind(Shader* shader);

		// Getters
		inline Cubemap* getIrradianceMap() { return m_IrradianceMap; }
	private:
		Cubemap* m_IrradianceMap;

		glm::vec3 m_Position;
		bool m_Generated;

		glm::vec2 m_ProbeResolution;
	};

}