#pragma once

#include "Buffer.h"

namespace engine {

	class VertexArray {
	private:
		GLuint m_VertexArrayID;
		std::vector<Buffer*> m_Buffers;
	public:
		VertexArray();
		~VertexArray();

		void addBuffer(Buffer* buffer, int index,GLuint stride = 0);
		void bind() const;
		void unbind() const;
	};

}
