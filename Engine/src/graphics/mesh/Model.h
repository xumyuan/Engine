#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../Shader.h"
#include "Mesh.h"
#include "../../utils/loaders/TextureLoader.h"

namespace engine {
	namespace graphics {


		class Model {
		public:
			Model(const char* path);
			Model(const Mesh& mesh);
			Model(const std::vector<Mesh>& meshes);

			void Draw(Shader& shader) const;
		private:
			std::vector<Mesh> m_Meshes;
			std::string m_Directory;


			void loadModel(const std::string& path);
			void processNode(aiNode* node, const aiScene* scene);
			Mesh processMesh(aiMesh* mesh, const aiScene* scene);
			Texture* loadMaterialTexture(aiMaterial* mat, aiTextureType type);
		};

	}
}