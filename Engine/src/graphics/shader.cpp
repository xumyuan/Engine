#include "Shader.h"

namespace engine {
	namespace graphics {

		Shader::Shader(const char* vertPath, const char* fragPath)
			: m_VertPath(vertPath), m_FragPath(fragPath)
		{
			m_ShaderID = load();
		}

		Shader::~Shader() {
			glDeleteProgram(m_ShaderID);
		}

		GLuint Shader::load() {
			// Create the program and shaders
			GLuint program = glCreateProgram();
			GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
			GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

			// Variables need to be declared or the character pointers will become dangling pointers
			std::string vertSourceString = FileUtils::read_file(m_VertPath);
			std::string fragSourceString = FileUtils::read_file(m_FragPath);
			const char* vertSource = vertSourceString.c_str();
			const char* fragSource = fragSourceString.c_str();

			// Vertex Shader
			glShaderSource(vertex, 1, &vertSource, NULL);
			glCompileShader(vertex);
			GLint result;

			// Check to see if it was successful
			glGetShaderiv(vertex, GL_COMPILE_STATUS, &result);
			if (result == GL_FALSE) {
				GLint length;
				glGetShaderiv(vertex, GL_INFO_LOG_LENGTH, &length);
				std::vector<char> error(length);
				glGetShaderInfoLog(vertex, length, &length, &error[0]);
				std::cout << "Failed to Compile Vertex Shader" << std::endl << &error[0] << std::endl;
				glDeleteShader(vertex);
				return 0;
			}

			//Fragment Shader
			glShaderSource(fragment, 1, &fragSource, NULL);
			glCompileShader(fragment);

			// Check to see if it was successful
			glGetShaderiv(fragment, GL_COMPILE_STATUS, &result);
			if (result == GL_FALSE) {
				GLint length;
				glGetShaderiv(fragment, GL_INFO_LOG_LENGTH, &length);
				std::vector<char> error(length);
				glGetShaderInfoLog(fragment, length, &length, &error[0]);
				std::cout << "Failed to Compile Fragment Shader" << std::endl << &error[0] << std::endl;
				glDeleteShader(fragment);
				return 0;
			}

			// Attach the shaders to the program and link them
			glAttachShader(program, vertex);
			glAttachShader(program, fragment);
			glLinkProgram(program);
			glValidateProgram(program);

			// Delete the vertex and fragment shaders
			glDeleteShader(vertex);
			glDeleteShader(fragment);

			// Return the program id
			return program;
		}

		GLint Shader::getUniformLocation(const GLchar* name) {
			return glGetUniformLocation(m_ShaderID, name);
		}

		void Shader::setUniform1f(const GLchar* name, float value) {
			glUniform1f(getUniformLocation(name), value);
		}

		void Shader::setUniform1i(const GLchar* name, int value) {
			glUniform1i(getUniformLocation(name), value);
		}

		void Shader::setUniform2f(const GLchar* name, const glm::vec2& vector) {
			glUniform2f(getUniformLocation(name), vector.x, vector.y);
		}

		void Shader::setUniform3f(const GLchar* name, const glm::vec3& vector) {
			glUniform3f(getUniformLocation(name), vector.x, vector.y, vector.z);
		}

		void Shader::setUniform4f(const GLchar* name, const glm::vec4& vector) {
			glUniform4f(getUniformLocation(name), vector.x, vector.y, vector.z, vector.w);
		}

		void Shader::setUniformMat4(const GLchar* name, const glm::mat4& matrix) {
			glUniformMatrix4fv(glGetUniformLocation(m_ShaderID, name), 1, GL_FALSE, glm::value_ptr(matrix));
		}

		void Shader::enable() const {
			glUseProgram(m_ShaderID);
		}

		void Shader::disable() const {
			glUseProgram(0);
		}

	}
}