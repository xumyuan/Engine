#pragma once

#include "rhi/include/RHIShaderProgram.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace engine {

	class Shader {
	private:
		std::unique_ptr<rhi::RHIShaderProgram> m_Program;
		std::string m_ShaderFilePath;

	public:
		// 接管一个已编译的 RHIShaderProgram
		Shader(const std::string& path, std::unique_ptr<rhi::RHIShaderProgram> program);
		~Shader();

		void enable() const;
		void disable() const;

		void setUniform(const char* name, float value);
		void setUniform(const char* name, int value);
		void setUniform(const char* name, const glm::vec2& vector);
		void setUniform(const char* name, const glm::vec3& vector);
		void setUniform(const char* name, const glm::vec4& vector);
		void setUniform(const char* name, const glm::ivec4& vector);
		void setUniform(const char* name, const glm::mat4& matrix);
		void setUniform(const char* name, const glm::mat3& matrix);

		// std::string 重载
		inline void setUniform(const std::string& name, float value) { setUniform(name.c_str(), value); }
		inline void setUniform(const std::string& name, int value) { setUniform(name.c_str(), value); }
		inline void setUniform(const std::string& name, const glm::vec2& vector) { setUniform(name.c_str(), vector); }
		inline void setUniform(const std::string& name, const glm::vec3& vector) { setUniform(name.c_str(), vector); }
		inline void setUniform(const std::string& name, const glm::vec4& vector) { setUniform(name.c_str(), vector); }
		inline void setUniform(const std::string& name, const glm::ivec4& vector) { setUniform(name.c_str(), vector); }
		inline void setUniform(const std::string& name, const glm::mat4& matrix) { setUniform(name.c_str(), matrix); }
		inline void setUniform(const std::string& name, const glm::mat3& matrix) { setUniform(name.c_str(), matrix); }

		// 获取 RHI ProgramHandle
		inline rhi::ProgramHandle getProgramHandle() const {
			return m_Program ? m_Program->getProgramHandle() : rhi::ProgramHandle();
		}
	};

}
