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
