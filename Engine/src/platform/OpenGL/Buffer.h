#pragma once

namespace engine {
	
	enum DrawType {
		Static,
		Dynamic
	};

	class Buffer {
	private:
		GLuint m_BufferID;
		GLuint m_ComponentCount;
		GLuint m_ElementNum;
	public:
		Buffer();
		Buffer(GLfloat* data, GLsizei amount, GLuint componentCount);
		~Buffer();

		void load(GLfloat* data, GLsizei amount, GLuint componentCount,DrawType type = DrawType::Static);
		void subData(GLfloat* data);

		void bind() const;
		void unbind() const;

		inline GLuint getComponentCount() const { return m_ComponentCount; }
		inline GLuint getElementNum()const { return m_ElementNum; }
	};


}
