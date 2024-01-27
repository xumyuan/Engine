#pragma once

#include "../Mesh.h"

namespace engine {
	namespace graphics {

		class Sphere : public Mesh {
		public:
			Sphere(int xSegments = 30, int ySegments = 30);
		};

	}
}