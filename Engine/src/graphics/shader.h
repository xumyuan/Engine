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

		void setUniform1f(const GLchar* name, float value);
		void setUniform1i(const GLchar* name, int value);
		void setUniform2f(const GLchar* name, const glm::vec2& vector);
		void setUniform3f(const GLchar* name, const glm::vec3& vector);
		void setUniform4f(const GLchar* name, const glm::vec4& vector);
		void setUniformMat4(const GLchar* name, const glm::mat4& matrix);
		void setUniformMat3(const char* name, const glm::mat3& matrix);

		inline GLuint getShaderID() { return m_ShaderID; }
	private:
		int GetUniformLocation(const char* name);

		static GLenum ShaderTypeFromString(const std::string& type);
		std::unordered_map<GLenum, std::string> PreProcessShaderBinary(std::string& source);
		void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);
	private:
		GLint getUniformLocation(const GLchar* name);
		GLuint load();
	};

}
