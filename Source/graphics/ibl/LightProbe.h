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

		// Assumes the shader is bound
		void bind(Shader* shader);
		void bind(rhi::CommandBuffer& cmd, rhi::ProgramHandle program);

		// Getters
		inline Cubemap* getIrradianceMap() { return m_IrradianceMap; }
	private:
		Cubemap* m_IrradianceMap;

		glm::vec3 m_Position;
		glm::vec2 m_ProbeResolution;

		bool m_Generated;
	};

}