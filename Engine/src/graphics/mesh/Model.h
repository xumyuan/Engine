#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../Shader.h"
#include "Mesh.h"

namespace engine {
	namespace graphics {
		struct Texture {
			unsigned int id;
			std::string type;
			aiString path; //用于标识纹理 TODO: change to a proper texture loading system
		};

		class Model {
		public:
			Model(const char* path);
			Model(const Mesh& mesh);
			Model(const std::vector<Mesh>& meshes);

			void Draw(Shader& shader) const;
		private:
			static std::vector<Texture> m_LoadedTextures;  // 使用同样的纹理数据，避免重复加载
			std::vector<Mesh> m_Meshes;
			std::string m_Directory;


			void loadModel(const std::string& path);
			void processNode(aiNode* node, const aiScene* scene);
			Mesh processMesh(aiMesh* mesh, const aiScene* scene);
			unsigned int loadMaterialTexture(aiMaterial* mat, aiTextureType type, const char* typeName);
		};

	}
}