#pragma once
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "..\graphics\Mesh.h"
#include "..\graphics\Shader.h"
#include "stb_image.h"
#include <SOIL/SOIL.h>
#include "../platform/OpenGL/Utility.h"

namespace engine {
	namespace terrain {

		class Terrain {
		private:
			GLfloat m_TerrainSize;
			GLuint m_VertexSideCount;
			GLushort m_HeightMapScale;

			glm::mat4 m_ModelMatrix;
			glm::vec3 m_Position;
			graphics::Mesh* m_Mesh;
		public:
			Terrain(glm::vec3& worldPosition);
			~Terrain();

			void Draw(graphics::Shader& shader) const;

			inline glm::vec3 getPosition() { return m_Position; }
		private:
			glm::vec3 calculateNormal(int x, int z, unsigned char* heightMapData);
			GLfloat getVertexHeight(int x, int y, unsigned char* heightMapData);
		};

	}
}

