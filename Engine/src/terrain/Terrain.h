#pragma once

#include "graphics/Shader.h"
#include "graphics/mesh/Mesh.h"
#include "graphics/mesh/Model.h"
#include "utils/loaders/TextureLoader.h"


namespace engine {
	namespace terrain {

		class Terrain {
		private:

		public:
			Terrain(glm::vec3& worldPosition);
			~Terrain();

			void Draw(graphics::Shader& shader, graphics::RenderPass pass) const;

			inline const glm::vec3& getPosition() const { return m_Position; }
		private:
			glm::vec3 calculateNormal(unsigned int x, unsigned int z, unsigned char* heightMapData);
			GLfloat getVertexHeight(unsigned int x, unsigned int y, unsigned char* heightMapData);


			float m_TerrainSize;
			unsigned int m_VertexSideCount;
			GLushort m_HeightMapScale;

			glm::mat4 m_ModelMatrix;
			glm::vec3 m_Position;
			graphics::Mesh* m_Mesh;
			std::array<graphics::Texture*, 9> m_Textures; // 表示地形纹理喷溅支持的所有纹理
		};

	}
}

