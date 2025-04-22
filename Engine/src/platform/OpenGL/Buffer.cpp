#include "pch.h"
#include "Buffer.h"

namespace engine {


	Buffer::Buffer() {
		glGenBuffers(1, &m_BufferID);
	}

	Buffer::Buffer(GLfloat* data, GLsizei amount, GLuint componentCount) {
		glGenBuffers(1, &m_BufferID);
		load(data, amount, componentCount);
	}

	Buffer::~Buffer() {
		glDeleteBuffers(1, &m_BufferID);
	}

	void Buffer::load(GLfloat* data, GLsizei amount, GLuint componentCount, DrawType type) {
		m_ComponentCount = componentCount;
		m_ElementNum = amount;

		bind();
		if (type == DrawType::Dynamic)
		{
			glBufferData(GL_ARRAY_BUFFER, amount * sizeof(float), 0, GL_DYNAMIC_DRAW);
		}
		else if (type == DrawType::Static) {
			glBufferData(GL_ARRAY_BUFFER, amount * sizeof(float), data, GL_STATIC_DRAW);
		}
	}

	void Buffer::subData(GLfloat* data) {
		bind();
		glBufferSubData(GL_ARRAY_BUFFER, 0, m_ElementNum * sizeof(float), (void*)data);
		unbind();
	}

	void Buffer::bind() const {
		glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
	}

	void Buffer::unbind() const {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}


}
