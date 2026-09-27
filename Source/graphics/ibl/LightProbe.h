#pragma once

#include "graphics/Shader.h"
#include "graphics/texture/Cubemap.h"
#include "rhi/include/RHICommandBuffer.h"

namespace engine {

	class LightProbe {
	public:
		LightProbe(glm::vec3& probePosition, glm::vec2& probeResolution);
		~LightProbe();
		void generate();

		void bind(rhi::CommandBuffer& cmd);

		// Getters
		inline Cubemap* getIrradianceMap() { return m_IrradianceMap; }
	private:
		Cubemap* m_IrradianceMap;

		glm::vec3 m_Position;
		glm::vec2 m_ProbeResolution;

		bool m_Generated;
	};

}