#pragma once
#pragma once

#include "utils/FileUtils.h"

namespace engine {
	namespace graphics {

		class Shader {
		private:
			GLuint m_ShaderID;
			const char* m_VertPath, * m_FragPath, * m_GeomPath;
		public:
			Shader(const char* vertPath, const char* fragPath);
			Shader(const char* vertPath, const char* fragPath, const char* geoPath);
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
			GLint getUniformLocation(const GLchar* name);
			GLuint load();
		};

	}
}
