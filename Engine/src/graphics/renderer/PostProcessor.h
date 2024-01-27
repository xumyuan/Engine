#pragma once

#include <GL/glew.h>

namespace engine {
	namespace graphic {

		class PostProcessor {
		public:
			PostProcessor();
		public:
			GLuint texture;

			// Post Processing Toggles
			bool blur = false;
		};

	}
}