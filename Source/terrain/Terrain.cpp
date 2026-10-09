#include "pch.h"
#include "Terrain.h"
#include <ui/DebugPane.h>

#include <imgui.h>
#include "graphics/UniformBufferManager.h"
#include "graphics/TextureBindings.h"


namespace engine {
	Terrain::Terrain(const glm::vec3& worldPosition) : m_Position(worldPosition)
	{
		m_isVisible = false;
		m_DebugSection = DebugPane::addSection("Terrain", [this]() {
			ImGui::Checkbox("Visible", &m_isVisible);
		}, 20);

		m_TextureTilingAmount = 8;
		m_ModelMatrix = glm::translate(glm::mat4(1), worldPosition);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> normals;

		std::vector<unsigned int> indices;


		// load textures
		TextureSettings srgbTextureSettings;
		srgbTextureSettings.IsSRGB = true;

		m_Textures[0] = TextureLoader::load2DTexture(std::string("Assets/terrain/grass/grassAlbedo.tga"), &srgbTextureSettings);
		m_Textures[1] = TextureLoader::load2DTexture(std::string("Assets/terrain/dirt/dirtAlbedo.tga"), &srgbTextureSettings);
		m_Textures[2] = TextureLoader::load2DTexture(std::string("Assets/terrain/branches/branchesAlbedo.tga"), &srgbTextureSettings);
		m_Textures[3] = TextureLoader::load2DTexture(std::string("Assets/terrain/rock/rockAlbedo.tga"), &srgbTextureSettings);

		TextureSettings textureSettings;
		textureSettings.format = rhi::TextureFormat::RGB8;

		m_Textures[4] = TextureLoader::load2DTexture(std::string("Assets/terrain/grass/grassNormal.tga"), &textureSettings);
		m_Textures[5] = TextureLoader::load2DTexture(std::string("Assets/terrain/dirt/dirtNormal.tga"), &textureSettings);
		m_Textures[6] = TextureLoader::load2DTexture(std::string("Assets/terrain/branches/branchesNormal.tga"), &textureSettings);
		m_Textures[7] = TextureLoader::load2DTexture(std::string("Assets/terrain/rock/rockNormal.tga"), &textureSettings);

		m_Textures[8] = TextureLoader::load2DTexture(std::string("Assets/terrain/grass/grassRoughness.tga"), &textureSettings);
		m_Textures[9] = TextureLoader::load2DTexture(std::string("Assets/terrain/dirt/dirtRoughness.tga"), &textureSettings);
		m_Textures[10] = TextureLoader::load2DTexture(std::string("Assets/terrain/branches/branchesRoughness.tga"), &textureSettings);
		m_Textures[11] = TextureLoader::load2DTexture(std::string("Assets/terrain/rock/rockRoughness.tga"), &textureSettings);

		m_Textures[12] = TextureLoader::load2DTexture(std::string("Assets/terrain/grass/grassMetallic.tga"), &textureSettings);
		m_Textures[13] = TextureLoader::load2DTexture(std::string("Assets/terrain/dirt/dirtMetallic.tga"), &textureSettings);
		m_Textures[14] = TextureLoader::load2DTexture(std::string("Assets/terrain/branches/branchesMetallic.tga"), &textureSettings);
		m_Textures[15] = TextureLoader::load2DTexture(std::string("Assets/terrain/rock/rockMetallic.tga"), &textureSettings);

		m_Textures[16] = TextureLoader::load2DTexture(std::string("Assets/terrain/grass/grassAO.tga"), &textureSettings);
		m_Textures[17] = TextureLoader::load2DTexture(std::string("Assets/terrain/dirt/dirtAO.tga"), &textureSettings);
		m_Textures[18] = TextureLoader::load2DTexture(std::string("Assets/terrain/branches/branchesAO.tga"), &textureSettings);
		m_Textures[19] = TextureLoader::load2DTexture(std::string("Assets/terrain/rock/rockAO.tga"), &textureSettings);

		m_Textures[20] = TextureLoader::load2DTexture(std::string("Assets/terrain/blendMap.tga"), &textureSettings);

		// generate mesh
		int mapWidth, mapHeight;
		unsigned char* heightMapImage = stbi_load("Assets/terrain/heightMap.png", &mapWidth, &mapHeight, 0, SOIL_LOAD_L);
		if (mapWidth != mapHeight) {
			//std::cout << "ERROR: Can't use a heightmap with a different width and height" << std::endl;
			spdlog::error("Can't use a heightmap with a different width and height");
			return;
		}

		m_VertexSideCount = mapWidth;
		m_TerrainSize = 4;
		m_HeightMapScale = 220;


		std::vector<glm::vec3> tangents(m_VertexSideCount * m_VertexSideCount, glm::vec3(0.0f));
		std::vector<glm::vec3> bitangents(m_VertexSideCount * m_VertexSideCount, glm::vec3(0.0f));

		// 顶点生成
		for (unsigned int z = 0; z < m_VertexSideCount; z++) {
			for (unsigned int x = 0; x < m_VertexSideCount; x++) {
				positions.push_back(glm::vec3(x * m_TerrainSize, getVertexHeight(x, z, heightMapImage), z * m_TerrainSize));
				uvs.push_back(glm::vec2((float)x / ((float)m_VertexSideCount - 1.0f), (float)z / ((float)m_VertexSideCount - 1.0f)));
				normals.push_back(calculateNormal(x, z, heightMapImage));
			}
		}

		stbi_image_free(heightMapImage);


		// 生成索引
		// 统一三角形顶点顺序，允许使用背面剔除
		for (unsigned int height = 0; height < m_VertexSideCount - 1; ++height) {
			for (unsigned int width = 0; width < m_VertexSideCount - 1; ++width) {
				//  T: top  B: bottom
				//  L: left R: right
				unsigned int indexTL = width + (height * m_VertexSideCount);
				unsigned int indexTR = 1 + width + (height * m_VertexSideCount);
				unsigned int indexBL = m_VertexSideCount + width + (height * m_VertexSideCount);
				unsigned int indexBR = 1 + m_VertexSideCount + width + (height * m_VertexSideCount);

				// Triangle 1
				indices.push_back(indexTL);
				indices.push_back(indexBR);
				indices.push_back(indexTR);

				// Triangle 2
				indices.push_back(indexTL);
				indices.push_back(indexBL);
				indices.push_back(indexBR);

				// Triangle 1 tangents 
				glm::vec3& v0 = positions[indexTL];
				glm::vec3& v1 = positions[indexBR];
				glm::vec3& v2 = positions[indexTR];
				glm::vec2& uv0 = uvs[indexTL];
				glm::vec2& uv1 = uvs[indexBR];
				glm::vec2& uv2 = uvs[indexTR];

				glm::vec3 deltaPos1 = v1 - v0;
				glm::vec3 deltaPos2 = v2 - v0;
				glm::vec2 deltaUV1 = uv1 - uv0;
				glm::vec2 deltaUV2 = uv2 - uv0;

				float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
				glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
				tangents[indexTL] += tangent;
				tangents[indexBR] += tangent;
				tangents[indexTR] += tangent;

				// Triangle 2 tangent
				v0 = positions[indexTL];
				v1 = positions[indexBR];
				v2 = positions[indexTR];
				uv0 = uvs[indexTL];
				uv1 = uvs[indexBR];
				uv2 = uvs[indexTR];
				deltaPos1 = v1 - v0;
				deltaPos2 = v2 - v0;
				deltaUV1 = uv1 - uv0;
				deltaUV2 = uv2 - uv0;
				r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
				tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
				tangents[indexTL] += tangent;
				tangents[indexBL] += tangent;
				tangents[indexBR] += tangent;
			}
		}

		for (unsigned int i = 0; i < tangents.size(); i++)
		{
			const glm::vec3& normal = normals[i];
			glm::vec3 tangent = glm::normalize(tangents[i]);

			tangent = glm::normalize(tangent - glm::dot(tangent, normal) * normal);
			glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

			tangents[i] = tangent;
			bitangents[i] = bitangent;
		}


		m_Mesh = new Mesh(positions, uvs, normals, tangents, bitangents, indices);
		m_Mesh->LoadData(true);
	}

	Terrain::~Terrain() {
		DebugPane::removeSection(m_DebugSection);
		delete m_Mesh;
	}

	void Terrain::Draw(rhi::CommandBuffer& cmd, rhi::ProgramHandle program, RenderPassType pass) const {
		if (!m_isVisible) return;

		// 阴影 pass 也需要正确的 model 矩阵，调用方不负责写 PerObject
		if (auto* uboMgr = getUBOManager()) {
			if (pass != RenderPassType::ShadowmapPassType) {
				glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(m_ModelMatrix)));
				uboMgr->preparePerObject(m_ModelMatrix, normalMatrix);
			}
			else {
				uboMgr->preparePerObject(m_ModelMatrix);
			}
			cmd.updateBuffer(uboMgr->getPerObjectHandle(), &uboMgr->getPerObjectData(), sizeof(UBOPerObject));
			cmd.bindUBO(UBOBinding::PerObject, uboMgr->getPerObjectHandle(), sizeof(UBOPerObject));

			if (pass != RenderPassType::ShadowmapPassType) {
				// 地形 shader 只读取 tilingAmount
				UBOMaterialParams materialParams{};
				materialParams.tilingAmount = m_TextureTilingAmount;
				cmd.updateBuffer(uboMgr->getMaterialParamsHandle(), &materialParams, sizeof(UBOMaterialParams));
				cmd.bindUBO(UBOBinding::MaterialParams, uboMgr->getMaterialParamsHandle(), sizeof(UBOMaterialParams));
			}
		}

		if (pass != RenderPassType::ShadowmapPassType) {
			static_assert(std::tuple_size_v<decltype(m_Textures)> == TextureUnit::TerrainTextureCount);
			for (uint32_t i = 0; i < TextureUnit::TerrainTextureCount; ++i) {
				cmd.bindTextureUnit(m_Textures[i]->getRHIHandle(), TextureUnit::TerrainBase + i);
			}
		}

		m_Mesh->Draw(cmd);
	}

	glm::vec3 Terrain::calculateNormal(unsigned int x, unsigned int z, unsigned char* heightMapData) {
		float heightR = getVertexHeight(x + 1, z, heightMapData);
		float heightL = getVertexHeight(x - 1, z, heightMapData);
		float heightU = getVertexHeight(x, z + 1, heightMapData);
		float heightD = getVertexHeight(x, z - 1, heightMapData);

		glm::vec3 normal(heightL - heightR, 2.0f, heightD - heightU);
		normal = glm::normalize(normal);

		return normal;
	}

	float Terrain::getVertexHeight(unsigned int x, unsigned int z, unsigned char* heightMapData) {
		if (x < 0 || x >= m_VertexSideCount || z < 0 || z >= m_VertexSideCount) {
			return 0.0f;
		}

		// Normalize height to [0, 1] then multiply it by the height map scale
		return (heightMapData[x + (z * m_VertexSideCount)] / 255.0f) * m_HeightMapScale;
	}

}
