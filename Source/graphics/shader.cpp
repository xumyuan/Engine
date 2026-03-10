#include "pch.h"
#include "Shader.h"
#include "rhi/opengl/OpenGLDevice.h"
#include "rhi/opengl/ShaderCompilerService.h"
#include "rhi/opengl/OpenGLProgram.h"

namespace engine {

	Shader::Shader(const std::string& path, rhi::ShaderCompilerService& compiler,
	               rhi::OpenGLDevice& device)
		: m_ShaderFilePath(path), m_Device(&device) {

		// 通过 ShaderCompilerService 编译着色器
		m_ProgramHandle = compiler.loadAndCompile(m_ShaderFilePath);

		if (static_cast<bool>(m_ProgramHandle)) {
			// 从 OpenGLDevice 获取 GL program ID，构造 OpenGLProgram
			GLuint glId = device.getGLProgramId(m_ProgramHandle);
			m_Program = std::make_unique<rhi::OpenGLProgram>(glId);
		} else {
			spdlog::error("Failed to compile shader: {}", m_ShaderFilePath);
		}
	}

	Shader::~Shader() {
		m_Program.reset();
		if (m_Device && static_cast<bool>(m_ProgramHandle)) {
			m_Device->destroyProgram(m_ProgramHandle);
		}
	}

	void Shader::enable() const {
		if (m_Program) {
			m_Program->use();
		}
	}

	void Shader::disable() const {
		rhi::OpenGLProgram::unuse();
	}

	void Shader::setUniform(const char* name, float value) {
		if (m_Program) m_Program->setUniform(name, value);
	}

	void Shader::setUniform(const char* name, int value) {
		if (m_Program) m_Program->setUniform(name, value);
	}

	void Shader::setUniform(const char* name, const glm::vec2& vector) {
		if (m_Program) m_Program->setUniform(name, vector);
	}

	void Shader::setUniform(const char* name, const glm::vec3& vector) {
		if (m_Program) m_Program->setUniform(name, vector);
	}

	void Shader::setUniform(const char* name, const glm::vec4& vector) {
		if (m_Program) m_Program->setUniform(name, vector);
	}

	void Shader::setUniform(const char* name, const glm::ivec4& vector) {
		if (m_Program) m_Program->setUniform(name, vector);
	}

	void Shader::setUniform(const char* name, const glm::mat4& matrix) {
		if (m_Program) m_Program->setUniform(name, matrix);
	}

	void Shader::setUniform(const char* name, const glm::mat3& matrix) {
		if (m_Program) m_Program->setUniform(name, matrix);
	}

}
