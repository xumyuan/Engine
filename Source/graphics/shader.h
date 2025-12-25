#pragma once
#pragma once

#include "utils/FileUtils.h"

namespace engine {

	class Shader {
	private:
		GLuint m_ShaderID;
		std::string m_ShaderFilePath;

	public:
		Shader(const std::string& path);
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

		inline GLuint getShaderID() { return m_ShaderID; }
	private:

		static GLenum ShaderTypeFromString(const std::string& type);
		std::unordered_map<GLenum, std::string> PreProcessShaderBinary(std::string& source);
		void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);
	private:
		GLint getUniformLocation(const char* name);
		GLuint load();
	};

}
