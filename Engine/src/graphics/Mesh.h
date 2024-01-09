#pragma once

#include <glm/glm.hpp>
#include <assimp\Importer.hpp>
#include <string>
#include <vector>
#include "Shader.h"

namespace engine {
	namespace graphics {

		struct Vertex {
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec2 TexCoords;
		};

		struct Texture {
			unsigned int id;
			std::string type;
			aiString path;
		};

		class Mesh {
		public:
			Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture> textures);
			void Draw(Shader& shader) const;

			// Getters
			inline std::vector<Vertex> const& getVertices() const { return m_Vertices; }
			inline std::vector<unsigned int> const& getIndices() const { return m_Indices; }
			inline std::vector<Texture> const& getTextures() const { return m_Textures; }
		private:
			// Mesh Data
			std::vector<Vertex> m_Vertices;
			std::vector<unsigned int> m_Indices;
			std::vector<Texture> m_Textures;

			unsigned int m_VAO, m_VBO, m_EBO;

			void setupMesh();
		};

	}
}