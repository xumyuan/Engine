#include "pch.h"
#include "Terrain.h"


namespace engine {
	Terrain::Terrain(const glm::vec3& worldPosition) : m_Position(worldPosition)
	{
		m_ModelMatrix = glm::translate(glm::mat4(1), worldPosition);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;

		GLint mapWidth, mapHeight;
		unsigned char* heightMapImage = stbi_load("res/terrain/heightMap.png", &mapWidth, &mapHeight, 0, SOIL_LOAD_L);
		if (mapWidth != mapHeight) {
			std::cout << "ERROR: Can't use a heightmap with a different width and height" << std::endl;
			Logger::getInstance().error("logged_files/terrain_creation.txt", "terrain initialization", "Can't use a heightmap with a different width and height");
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

		// Textures
		TextureSettings srgbTextureSettings;
		srgbTextureSettings.IsSRGB = true;

		m_Textures[0] = TextureLoader::load2DTexture(std::string("res/terrain/grass.png"), &srgbTextureSettings);
		m_Textures[1] = TextureLoader::load2DTexture(std::string("res/terrain/dirt.png"), &srgbTextureSettings);
		m_Textures[2] = TextureLoader::load2DTexture(std::string("res/terrain/sand.png"), &srgbTextureSettings);
		m_Textures[3] = TextureLoader::load2DTexture(std::string("res/terrain/stone.png"), &srgbTextureSettings);
		m_Textures[4] = TextureLoader::load2DTexture(std::string("res/terrain/blendMap.png"));
		m_Textures[5] = TextureLoader::load2DTexture(std::string("res/terrain/grass_normal.png"));
		m_Textures[6] = TextureLoader::load2DTexture(std::string("res/terrain/dirt_normal.png"));
		m_Textures[7] = TextureLoader::load2DTexture(std::string("res/terrain/sand_normal.png"));
		m_Textures[8] = TextureLoader::load2DTexture(std::string("res/terrain/stone_normal.png"));

		m_Mesh = new Mesh(positions, uvs, normals, indices);
		m_Mesh->LoadData(true);
	}

	Terrain::~Terrain() {
		delete m_Mesh;
	}

	void Terrain::Draw(Shader* shader, RenderPassType pass) const {

		// Texture unit 0 is reserved for the shadowmap
		if (pass != RenderPassType::ShadowmapPassType) {
			m_Textures[0]->bind(1);
			shader->setUniform1i("material.texture_diffuse1", 1);
			m_Textures[1]->bind(2);
			shader->setUniform1i("material.texture_diffuse2", 2);
			m_Textures[2]->bind(3);
			shader->setUniform1i("material.texture_diffuse3", 3);
			m_Textures[3]->bind(4);
			shader->setUniform1i("material.texture_diffuse4", 4);
			m_Textures[4]->bind(5);
			shader->setUniform1i("material.blendmap", 5);
			m_Textures[5]->bind(6);
			shader->setUniform1i("material.texture_normal1", 6);
			m_Textures[6]->bind(7);
			shader->setUniform1i("material.texture_normal2", 7);
			m_Textures[7]->bind(8);
			shader->setUniform1i("material.texture_normal3", 8);
			m_Textures[8]->bind(9);
			shader->setUniform1i("material.texture_normal4", 9);
		}


		shader->setUniformMat4("model", m_ModelMatrix);
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