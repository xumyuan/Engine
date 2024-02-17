#pragma once

#include "Buffer.h"

namespace engine {
	namespace opengl {

		class VertexArray {
		private:
			GLuint m_VertexArrayID;
			std::vector<Buffer*> m_Buffers;
		public:
			VertexArray();
			~VertexArray();

			void addBuffer(Buffer* buffer, int index);
			void bind() const;
			void unbind() const;
		};

	}
}