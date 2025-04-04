#include "pch.h"
#include "Terrain.h"


namespace engine {
	Terrain::Terrain(const glm::vec3& worldPosition) : m_Position(worldPosition)
	{
		m_TextureTilingAmount = 8;
		m_ModelMatrix = glm::translate(glm::mat4(1), worldPosition);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;


		// load textures
		TextureSettings srgbTextureSettings;
		srgbTextureSettings.IsSRGB = true;

		m_Textures[0] = TextureLoader::load2DTexture(std::string("res/terrain/grass/grassAlbedo.tga"), &srgbTextureSettings);
		m_Textures[1] = TextureLoader::load2DTexture(std::string("res/terrain/dirt/dirtAlbedo.tga"), &srgbTextureSettings);
		m_Textures[2] = TextureLoader::load2DTexture(std::string("res/terrain/branches/branchesAlbedo.tga"), &srgbTextureSettings);
		m_Textures[3] = TextureLoader::load2DTexture(std::string("res/terrain/rock/rockAlbedo.tga"), &srgbTextureSettings);

		m_Textures[4] = TextureLoader::load2DTexture(std::string("res/terrain/grass/grassNormal.tga"), &srgbTextureSettings);
		m_Textures[5] = TextureLoader::load2DTexture(std::string("res/terrain/dirt/dirtNormal.tga"), &srgbTextureSettings);
		m_Textures[6] = TextureLoader::load2DTexture(std::string("res/terrain/branches/branchesNormal.tga"), &srgbTextureSettings);
		m_Textures[7] = TextureLoader::load2DTexture(std::string("res/terrain/rock/rockNormal.tga"), &srgbTextureSettings);

		TextureSettings textureSettings;
		textureSettings.TextureFormat = GL_RGB;

		m_Textures[8] = TextureLoader::load2DTexture(std::string("res/terrain/grass/grassRoughness.tga"), &textureSettings);
		m_Textures[9] = TextureLoader::load2DTexture(std::string("res/terrain/dirt/dirtRoughness.tga"), &textureSettings);
		m_Textures[10] = TextureLoader::load2DTexture(std::string("res/terrain/branches/branchesRoughness.tga"), &textureSettings);
		m_Textures[11] = TextureLoader::load2DTexture(std::string("res/terrain/rock/rockRoughness.tga"), &textureSettings);

		m_Textures[12] = TextureLoader::load2DTexture(std::string("res/terrain/grass/grassMetallic.tga"), &textureSettings);
		m_Textures[13] = TextureLoader::load2DTexture(std::string("res/terrain/dirt/dirtMetallic.tga"), &textureSettings);
		m_Textures[14] = TextureLoader::load2DTexture(std::string("res/terrain/branches/branchesMetallic.tga"), &textureSettings);
		m_Textures[15] = TextureLoader::load2DTexture(std::string("res/terrain/rock/rockMetallic.tga"), &textureSettings);

		m_Textures[16] = TextureLoader::load2DTexture(std::string("res/terrain/grass/grassAO.tga"), &textureSettings);
		m_Textures[17] = TextureLoader::load2DTexture(std::string("res/terrain/dirt/dirtAO.tga"), &textureSettings);
		m_Textures[18] = TextureLoader::load2DTexture(std::string("res/terrain/branches/branchesAO.tga"), &textureSettings);
		m_Textures[19] = TextureLoader::load2DTexture(std::string("res/terrain/rock/rockAO.tga"), &textureSettings);

		m_Textures[20] = TextureLoader::load2DTexture(std::string("res/terrain/blendMap.tga"), &textureSettings);

		// generate mesh
		GLint mapWidth, mapHeight;
		unsigned char* heightMapImage = stbi_load("res/terrain/heightMap.png", &mapWidth, &mapHeight, 0, SOIL_LOAD_L);
		if (mapWidth != mapHeight) {
			//std::cout << "ERROR: Can't use a heightmap with a different width and height" << std::endl;
			spdlog::error("Can't use a heightmap with a different width and height");
			return;
		}

		m_VertexSideCount = mapWidth;
		m_TerrainSize = 4;
		m_HeightMapScale = 220;

		// 顶点生成
		for (GLuint z = 0; z < m_VertexSideCount; z++) {
			for (GLuint x = 0; x < m_VertexSideCount; x++) {
				positions.push_back(glm::vec3(x * m_TerrainSize, getVertexHeight(x, z, heightMapImage), z * m_TerrainSize));
				uvs.push_back(glm::vec2((float)x / ((float)m_VertexSideCount - 1.0f), (float)z / ((float)m_VertexSideCount - 1.0f)));
				normals.push_back(calculateNormal(x, z, heightMapImage));
			}
		}

		stbi_image_free(heightMapImage);


		// 生成索引
		// 统一三角形顶点顺序，允许使用背面剔除
		for (GLuint height = 0; height < m_VertexSideCount - 1; ++height) {
			for (GLuint width = 0; width < m_VertexSideCount - 1; ++width) {
				// Triangle 1
				indices.push_back(width + (height * m_VertexSideCount));
				indices.push_back(1 + m_VertexSideCount + width + (height * m_VertexSideCount));
				indices.push_back(1 + width + (height * m_VertexSideCount));

				// Triangle 2
				indices.push_back(width + (height * m_VertexSideCount));
				indices.push_back(m_VertexSideCount + width + (height * m_VertexSideCount));
				indices.push_back(1 + m_VertexSideCount + width + (height * m_VertexSideCount));
			}
		}

		m_Mesh = new Mesh(positions, uvs, normals, indices);
		m_Mesh->LoadData(true);
	}

	Terrain::~Terrain() {
		delete m_Mesh;
	}

	void Terrain::Draw(Shader* shader, RenderPassType pass) const {

		// Texture unit 0 is reserved for the directional light shadowmap
		// Texture unit 1 is reserved for the spot light shadowmap
		// Texture unit 2 is reserved for the point light shadowmap
		if (pass != RenderPassType::ShadowmapPassType) {
			int currentTextureUnit = 3;

			// Textures
			m_Textures[0]->bind(currentTextureUnit);
			shader->setUniform("material.texture_albedo1", currentTextureUnit++);
			m_Textures[1]->bind(currentTextureUnit);
			shader->setUniform("material.texture_albedo2", currentTextureUnit++);
			m_Textures[2]->bind(currentTextureUnit);
			shader->setUniform("material.texture_albedo3", currentTextureUnit++);
			m_Textures[3]->bind(currentTextureUnit);
			shader->setUniform("material.texture_albedo4", currentTextureUnit++);

			m_Textures[4]->bind(currentTextureUnit);
			shader->setUniform("material.texture_normal1", currentTextureUnit++);
			m_Textures[5]->bind(currentTextureUnit);
			shader->setUniform("material.texture_normal2", currentTextureUnit++);
			m_Textures[6]->bind(currentTextureUnit);
			shader->setUniform("material.texture_normal3", currentTextureUnit++);
			m_Textures[7]->bind(currentTextureUnit);
			shader->setUniform("material.texture_normal4", currentTextureUnit++);

			m_Textures[8]->bind(currentTextureUnit);
			shader->setUniform("material.texture_roughness1", currentTextureUnit++);
			m_Textures[9]->bind(currentTextureUnit);
			shader->setUniform("material.texture_roughness2", currentTextureUnit++);
			m_Textures[10]->bind(currentTextureUnit);
			shader->setUniform("material.texture_roughness3", currentTextureUnit++);
			m_Textures[11]->bind(currentTextureUnit);
			shader->setUniform("material.texture_roughness4", currentTextureUnit++);

			m_Textures[12]->bind(currentTextureUnit);
			shader->setUniform("material.texture_metallic1", currentTextureUnit++);
			m_Textures[13]->bind(currentTextureUnit);
			shader->setUniform("material.texture_metallic2", currentTextureUnit++);
			m_Textures[14]->bind(currentTextureUnit);
			shader->setUniform("material.texture_metallic3", currentTextureUnit++);
			m_Textures[15]->bind(currentTextureUnit);
			shader->setUniform("material.texture_metallic4", currentTextureUnit++);

			m_Textures[16]->bind(currentTextureUnit);
			shader->setUniform("material.texture_AO1", currentTextureUnit++);
			m_Textures[17]->bind(currentTextureUnit);
			shader->setUniform("material.texture_AO2", currentTextureUnit++);
			m_Textures[18]->bind(currentTextureUnit);
			shader->setUniform("material.texture_AO3", currentTextureUnit++);
			m_Textures[19]->bind(currentTextureUnit);
			shader->setUniform("material.texture_AO4", currentTextureUnit++);

			m_Textures[20]->bind(currentTextureUnit);
			shader->setUniform("material.blendmap", currentTextureUnit++);

			// Normal matrix
			glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(m_ModelMatrix)));
			shader->setUniform("normalMatrix", normalMatrix);

			// Tiling amount
			shader->setUniform("material.tilingAmount", m_TextureTilingAmount);
		}


		shader->setUniform("model", m_ModelMatrix);
		m_Mesh->Draw();
	}

	glm::vec3 Terrain::calculateNormal(unsigned int x, unsigned int z, unsigned char* heightMapData) {
		GLfloat heightR = getVertexHeight(x + 1, z, heightMapData);
		GLfloat heightL = getVertexHeight(x - 1, z, heightMapData);
		GLfloat heightU = getVertexHeight(x, z + 1, heightMapData);
		GLfloat heightD = getVertexHeight(x, z - 1, heightMapData);

		glm::vec3 normal(heightL - heightR, 2.0f, heightD - heightU);
		normal = glm::normalize(normal);

		return normal;
	}

	GLfloat Terrain::getVertexHeight(unsigned int x, unsigned int z, unsigned char* heightMapData) {
		if (x < 0 || x >= m_VertexSideCount || z < 0 || z >= m_VertexSideCount) {
			return 0.0f;
		}

		// Normalize height to [0, 1] then multiply it by the height map scale
		return (heightMapData[x + (z * m_VertexSideCount)] / 255.0f) * m_HeightMapScale;
	}

}