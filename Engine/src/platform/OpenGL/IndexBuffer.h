#pragma once

namespace engine {


	class IndexBuffer {
	private:
		GLuint m_BufferID;
		GLsizei m_Count;
	public:
		IndexBuffer();
		IndexBuffer(GLuint* data, GLsizei amount);
		~IndexBuffer();

		void load(GLuint* data, GLsizei amount);

		void bind() const;
		void unbind() const;

		inline GLsizei getCount() const { return m_Count; }
	};


}