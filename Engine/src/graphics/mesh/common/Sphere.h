#pragma once

#include "graphics/mesh/Mesh.h"

namespace engine {

	class Sphere : public Mesh {
	public:
		Sphere(int xSegments = 30, int ySegments = 30);
	};

}