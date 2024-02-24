#pragma once

#include <graphics/Shader.h>
#include <graphics/ibl/EnvironmentProbe.h>

namespace engine {

	enum EnvironmentProbeBlendSetting
	{
		PROBES_DISABLED, // Ignores probes and uses the skybox
		PROBES_SIMPLE, // Disables blending between probes
		PROBES_BLEND // Blends adjacent probes
	};

	class EnvironmentProbeManager {
	public:
		EnvironmentProbeManager(EnvironmentProbeBlendSetting sceneProbeBlendSetting);
		~EnvironmentProbeManager();

		void addProbe(EnvironmentProbe* probe);

		// Assumes shader is bound
		void bindProbe(glm::vec3& renderPosition, Shader& shader);
	private:
		EnvironmentProbeBlendSetting m_ProbeBlendSetting;
		std::vector<EnvironmentProbe*> m_Probes;
	};

}