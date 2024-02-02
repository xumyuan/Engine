#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../Shader.h"
#include "Mesh.h"
#include "../../utils/loaders/TextureLoader.h"
#include "../renderer/RenderPass.h"

namespace engine {
	namespace graphics {
		class MeshRenderer;

		class Model {
		public:
			Model(const char* path);
			Model(const Mesh& mesh);
			Model(const std::vector<Mesh>& meshes);

			void Draw(Shader& shader, RenderPass pass) const;
			inline std::vector<Mesh>& getMeshes() { return m_Meshes; }
		private:
			std::vector<Mesh> m_Meshes;
			std::string m_Directory;


			void loadModel(const std::string& path);
			void processNode(aiNode* node, const aiScene* scene);
			Mesh processMesh(aiMesh* mesh, const aiScene* scene);
			Texture* loadMaterialTexture(aiMaterial* mat, aiTextureType type, bool isSRGB);
		};

	}
}