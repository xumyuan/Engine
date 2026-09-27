#include "pch.h"
#include "Shader.h"

namespace engine {

	Shader::Shader(const std::string& path, std::unique_ptr<rhi::RHIShaderProgram> program)
		: m_ShaderFilePath(path), m_Program(std::move(program)) {

		if (!m_Program) {
			spdlog::error("Failed to create shader program: {}", m_ShaderFilePath);
		}
	}

	Shader::~Shader() {
		// unique_ptr 自动析构 RHIShaderProgram
	}

	void Shader::enable() const {
		if (m_Program) {
			m_Program->use();
		}
	}

	void Shader::disable() const {
		if (m_Program) {
			m_Program->unuse();
		}
	}


}
