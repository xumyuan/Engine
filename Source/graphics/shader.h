#pragma once

#include "rhi/include/RHIHandle.h"
#include "rhi/opengl/OpenGLProgram.h"

#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace engine {
namespace rhi {
    class OpenGLDevice;
    class ShaderCompilerService;
}

	class Shader {
	private:
		rhi::ProgramHandle m_ProgramHandle;
		std::unique_ptr<rhi::OpenGLProgram> m_Program;
		std::string m_ShaderFilePath;

	public:
		// 通过 ShaderCompilerService 编译并创建
		Shader(const std::string& path, rhi::ShaderCompilerService& compiler,
		       rhi::OpenGLDevice& device);
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

		// 获取底层 GL program ID（兼容 GLCache 等过渡期使用）
		inline GLuint getShaderID() {
			return m_Program ? m_Program->getGLProgramId() : 0;
		}

		// 获取 RHI ProgramHandle
		inline rhi::ProgramHandle getProgramHandle() const { return m_ProgramHandle; }

	private:
		rhi::OpenGLDevice* m_Device = nullptr;
	};

}
