#include "pch.h"
#include "IndexBuffer.h"

namespace engine {
	namespace opengl {

		IndexBuffer::IndexBuffer() {
			glGenBuffers(1, &m_BufferID);
		}

		IndexBuffer::IndexBuffer(GLuint* data, GLsizei amount) : m_Count(amount)
		{
			glGenBuffers(1, &m_BufferID);
			load(data, amount);
		}

		IndexBuffer::~IndexBuffer() {
			glDeleteBuffers(1, &m_BufferID);
		}

		void IndexBuffer::load(GLuint* data, GLsizei amount) {
			m_Count = amount;

			bind();
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, amount * sizeof(GLuint), data, GL_STATIC_DRAW);
		}

		void IndexBuffer::bind() const {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferID);
		}

		void IndexBuffer::unbind() const {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

	}
}