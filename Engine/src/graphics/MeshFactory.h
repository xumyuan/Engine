#pragma once

#include <vector>
#include "Mesh.h"
#include "../platform/OpenGL/Utility.h"

namespace engine {
	namespace graphics {
		class MeshFactory
		{
		public:
			Mesh* CreateQuad(const char* path, bool shouldHaveSpec = false);
		};
	}
}



