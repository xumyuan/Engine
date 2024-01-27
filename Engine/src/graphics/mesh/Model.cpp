#include "Model.h"

#include <iostream>
#include "stb_image.h"

#include "../../platform/OpenGL/Utility.h"
#include "../../utils/Logger.h"
#include "Mesh.h"

namespace engine {
	namespace graphics {

		std::vector<Texture> Model::m_LoadedTextures;

		Model::Model(const char* path) {
			loadModel(path);
		}

		Model::Model(const std::vector<Mesh>& meshes) {
			m_Meshes = meshes;
		}

		void Model::Draw(Shader& shader) const {
			for (unsigned int i = 0; i < m_Meshes.size(); ++i) {
				m_Meshes[i].m_Material.BindMaterialInformation(shader);
				m_Meshes[i].Draw();
			}
		}

		void Model::loadModel(const std::string& path) {
			Assimp::Importer import;
			const aiScene * scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				utils::Logger::getInstance().error("logged_files/model_loading.txt", "model initialization", import.GetErrorString());
				return;
			}

			m_Directory = path.substr(0, path.find_last_of('/'));

			processNode(scene->mRootNode, scene);
		}

		void Model::processNode(aiNode* node, const aiScene* scene) {
			// Process all of the node's meshes (if any)
			for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
				// Each node has an array of mesh indices, use these indices to get the meshes from the scene
				aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				m_Meshes.push_back(processMesh(mesh, scene));
			}
			// Process all of the node's children
			for (unsigned int i = 0; i < node->mNumChildren; ++i) {
				processNode(node->mChildren[i], scene);
			}
		}

		Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
			std::vector<glm::vec3> positions;
			std::vector<glm::vec2> uvs;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec3> tangents;
			std::vector<glm::vec3> bitangents;
			std::vector<unsigned int> indices;
			indices.reserve(mesh->mNumFaces * 3); // 假设为三角形，不是三角形也能被加载，不过不会被优化

			// 处理顶点数据
			for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
				glm::vec2 uvCoord;


				// Texture Coordinates (check if there is texture coordinates)
				if (mesh->mTextureCoords[0]) {
					// A vertex can contain up to 8 different texture coordinates. We are just going to use one set of TexCoords per vertex so grab the first one
					uvCoord.x = mesh->mTextureCoords[0][i].x;
					uvCoord.y = mesh->mTextureCoords[0][i].y;
				}
				else {
					uvCoord.x = 0.0f;
					uvCoord.y = 0.0f;
				}


				positions.push_back(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
				uvs.push_back(glm::vec2(uvCoord.x, uvCoord.y));
				normals.push_back(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
			}

			// Process Indices
			// Loop through every face (triangle thanks to aiProcess_Triangulate) and stores its indices in our meshes indices. This will ensure they are in the right order.
			for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
				aiFace face = mesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; ++j) {
					indices.push_back(face.mIndices[j]);
				}
			}
			Mesh newMesh(positions, uvs, normals, indices);
			newMesh.LoadData();
			// Process Materials (textures in this case)
			if (mesh->mMaterialIndex >= 0) {
				aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

				newMesh.m_Material.setDiffuseMapId(loadMaterialTexture(material, aiTextureType_DIFFUSE, "texture_diffuse"));
				newMesh.m_Material.setSpecularMapId(loadMaterialTexture(material, aiTextureType_SPECULAR, "texture_specular"));
				newMesh.m_Material.setNormalMapId(loadMaterialTexture(material, aiTextureType_NORMALS, "texture_normal"));
				newMesh.m_Material.setEmissionMapId(loadMaterialTexture(material, aiTextureType_EMISSIVE, "texture_emission"));
				float shininess = 0.0f;
				material->Get(AI_MATKEY_SHININESS, shininess); // Assimp 将镜面反射指数缩放 4 倍，因为大多数渲染器都是这样处理的。如果未指定，值默认为 0（即无镜面高光）
				newMesh.m_Material.setShininess(shininess);
			}

			return newMesh;

		}

		unsigned int Model::loadMaterialTexture(aiMaterial* mat, aiTextureType type, const char* typeName) {
			// Log material constraints are being violated (1 texture per type for the standard shader)
			if (mat->GetTextureCount(type) > 1)
				utils::Logger::getInstance().error("logged_files/material_creation.txt", "Mesh Loading", "Mesh's default material contains more than 1 " + std::string(typeName) + " map, which currently isn't supported by the standard shader");

			// 加载某种类型的纹理，假定只有一个
			if (mat->GetTextureCount(type) > 0) {
				aiString str;
				mat->GetTexture(type, 0, &str); // 只获取一个纹理，标准着色器只支持每种类型一个纹理

				// 如果纹理已经加载，就不用重复加载了
				for (unsigned int j = 0; j < Model::m_LoadedTextures.size(); ++j) {
					if (std::strcmp(str.C_Str(), Model::m_LoadedTextures[j].path.C_Str()) == 0) {
						return Model::m_LoadedTextures[j].id;
					}
				}

				// 没被加载则进行加载
				Texture texture;
				texture.id = opengl::Utility::loadTextureFromFile((m_Directory + "/" + std::string(str.C_Str())).c_str()); // 假定材质与模型在同一目录
				texture.type = typeName;
				texture.path = str;
				Model::m_LoadedTextures.push_back(texture); // 添加到已加载纹理，不会重复加载纹理
				return Model::m_LoadedTextures[m_LoadedTextures.size() - 1].id;
			}

			return 0;
		}
	}
}