#include "pch.h"
#include "ReflectionProbe.h"

namespace engine {

	Texture* ReflectionProbe::s_BRDF_LUT = nullptr;

	ReflectionProbe::ReflectionProbe(glm::vec3& probePosition, glm::vec2& probeResolution, bool isStatic)
		:m_Position(probePosition), m_ProbeResolution(probeResolution), m_IsStatic(isStatic), m_Generated(false), m_PrefilterMap(nullptr)
	{
	}

	ReflectionProbe::~ReflectionProbe() {
		delete m_PrefilterMap;
	}

	void ReflectionProbe::generate() {
		// 生成 HDR 环境探针并设置生成标志
		CubemapSettings settings;
		settings.format = rhi::TextureFormat::RGBA16F;
		settings.formatExplicitlySet = true;
		settings.minFilter = rhi::FilterMode::LinearMipmapLinear;
		settings.HasMips = true;

		m_PrefilterMap = new Cubemap(settings);
		for (int i = 0; i < 6; i++) {
			m_PrefilterMap->generateCubemapFace(static_cast<uint8_t>(i), (unsigned int)m_ProbeResolution.x, (unsigned int)m_ProbeResolution.y, ChannelLayout::RGBA, nullptr);
		}

		m_Generated = true;
	}

	void ReflectionProbe::bind(Shader* shader) {
		// reflectionProbeMipCount 通过 IBLParams UBO 由调用方设置
		m_PrefilterMap->bind(2);
		shader->setUniform("prefilterMap", 2);
		s_BRDF_LUT->bind(3);
		shader->setUniform("brdfLUT", 3);
	}

	void ReflectionProbe::bind(rhi::CommandBuffer& cmd, rhi::ProgramHandle program) {
		cmd.bindTextureUnit(m_PrefilterMap->getRHIHandle(), 2);
		cmd.setUniformInt(program, "prefilterMap", 2);
		cmd.bindTextureUnit(s_BRDF_LUT->getRHIHandle(), 3);
		cmd.setUniformInt(program, "brdfLUT", 3);
	}

}
