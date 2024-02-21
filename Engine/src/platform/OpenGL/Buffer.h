#pragma once

namespace engine {


	class Buffer {
	private:
		GLuint m_BufferID;
		GLuint m_ComponentCount;
	public:
		Buffer();
		Buffer(GLfloat* data, GLsizei amount, GLuint componentCount);
		~Buffer();

		void load(GLfloat* data, GLsizei amount, GLuint componentCount);

		void bind() const;
		void unbind() const;

		inline GLuint getComponentCount() const { return m_ComponentCount; }
	};


}
