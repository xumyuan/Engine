#pragma once

#include "graphics/Shader.h"
#include "graphics/mesh/Mesh.h"
#include "graphics/mesh/Model.h"
#include "utils/loaders/TextureLoader.h"

namespace engine {
	class Terrain {
	private:

	public:
		Terrain(const glm::vec3& worldPosition);
		~Terrain();

		void Draw(Shader *shader, RenderPassType pass) const;

		inline const glm::vec3& getPosition() const { return m_Position; }

		void setVisible(bool visible) { m_isVisible = visible; }
		bool getVisible() { return m_isVisible; }
	private:
		glm::vec3 calculateNormal(unsigned int x, unsigned int z, unsigned char* heightMapData);
		GLfloat getVertexHeight(unsigned int x, unsigned int y, unsigned char* heightMapData);

		float m_TextureTilingAmount;
		float m_TerrainSize;
		unsigned int m_VertexSideCount;
		GLushort m_HeightMapScale;

		glm::mat4 m_ModelMatrix;
		glm::vec3 m_Position;
		Mesh* m_Mesh;
		std::array<Texture*, 21> m_Textures; 

		// visible
		bool m_isVisible;
	};

}

