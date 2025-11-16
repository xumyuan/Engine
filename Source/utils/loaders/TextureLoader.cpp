#include "pch.h"
#include "TextureLoader.h"
#include "thread/thread_pool.h"

namespace engine {

	// Static declarations
	std::unordered_map<std::string, Texture*> TextureLoader::m_TextureCache;
	Texture* TextureLoader::s_DefaultAlbedo;
	Texture* TextureLoader::s_DefaultNormal;
	Texture* TextureLoader::s_FullMetallic, * TextureLoader::s_NoMetallic;
	Texture* TextureLoader::s_FullRoughness, * TextureLoader::s_NoRoughness;
	Texture* TextureLoader::s_DefaultAO;
	Texture* TextureLoader::s_DefaultEmission;

	std::queue<std::function<void()>> TextureLoader::mainThreadTasks;
	std::mutex TextureLoader::taskMutex;


	Texture* TextureLoader::load2DTexture(const std::string& path, TextureSettings* settings) {
		// Check the cache
		auto iter = m_TextureCache.find(path);
		if (iter != m_TextureCache.end()) {
			return iter->second;
		}

		Texture* texture = (settings != nullptr) ? new Texture(*settings) : new Texture();
		m_TextureCache[path] = texture;
		
		thread_pool.addTask(new TextureLoadTask([=]() {
			// Load the texture
			int width, height, numComponents;
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &numComponents, 0);
			if (!data) {
				spdlog::error("texture load fail - path:{0}", path);
				return nullptr;
			}

			GLenum dataFormat;
			switch (numComponents) {
			case 1: dataFormat = GL_RED;  break;
			case 3: dataFormat = GL_RGB;  break;
			case 4: dataFormat = GL_RGBA; break;
			}

			{
				std::lock_guard<std::mutex> lock(taskMutex);
				mainThreadTasks.push([=] {
					texture->generate2DTexture(width, height, dataFormat, GL_UNSIGNED_BYTE, data);
					stbi_image_free(data);
					}
				);
			}
			
			}));
		return m_TextureCache[path];
	}

	Cubemap* TextureLoader::loadCubemapTexture(const std::string& right, const std::string& left, const std::string& top, const std::string& bottom, const std::string& back, const std::string& front, CubemapSettings* settings) {

		Cubemap* cubemap = new Cubemap();
		if (settings != nullptr)
			cubemap->setCubemapSettings(*settings);

		std::vector<std::string> faces = { right, left, top, bottom, back, front };

		// Load the textures for the cubemap
		int width, height, numComponents;
		for (unsigned int i = 0; i < 6; ++i) {
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &numComponents, 0);

			if (data) {
				GLenum dataFormat;
				switch (numComponents) {
				case 1: dataFormat = GL_RED;  break;
				case 3: dataFormat = GL_RGB;  break;
				case 4: dataFormat = GL_RGBA; break;
				}

				cubemap->generateCubemapFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, width, height, dataFormat, data);
				stbi_image_free(data);
			}
			else {
				spdlog::error("Couldn't load cubemap using 6 filepaths. Filepath error: {0}", faces[i]);
				stbi_image_free(data);
				return cubemap;
			}
		}

		return cubemap;
	}

	void TextureLoader::initializeDefaultTextures() {
		// Setup texture and minimal filtering because they are 1x1 textures so they require none
		TextureSettings srgbTextureSettings;
		srgbTextureSettings.IsSRGB = true;

		s_DefaultAlbedo = load2DTexture(std::string("Assets/textures/default/defaultAlbedo.png"), &srgbTextureSettings);
		s_DefaultAlbedo->setAnisotropicFilteringMode(1.0f);
		s_DefaultAlbedo->setTextureMinFilter(GL_NEAREST);
		s_DefaultAlbedo->setTextureMagFilter(GL_NEAREST);
		s_DefaultNormal = load2DTexture(std::string("Assets/textures/default/defaultNormal.png"));
		s_DefaultNormal->setAnisotropicFilteringMode(1.0f);
		s_DefaultNormal->setTextureMinFilter(GL_NEAREST);
		s_DefaultNormal->setTextureMagFilter(GL_NEAREST);
		s_FullMetallic = load2DTexture(std::string("Assets/textures/default/white.png"));
		s_FullMetallic->setAnisotropicFilteringMode(1.0f);
		s_FullMetallic->setTextureMinFilter(GL_NEAREST);
		s_FullMetallic->setTextureMagFilter(GL_NEAREST);
		s_NoMetallic = load2DTexture(std::string("Assets/textures/default/black.png"));
		s_NoMetallic->setAnisotropicFilteringMode(1.0f);
		s_NoMetallic->setTextureMinFilter(GL_NEAREST);
		s_NoMetallic->setTextureMagFilter(GL_NEAREST);
		s_FullRoughness = load2DTexture(std::string("Assets/textures/default/white.png"));
		s_FullRoughness->setAnisotropicFilteringMode(1.0f);
		s_FullRoughness->setTextureMinFilter(GL_NEAREST);
		s_FullRoughness->setTextureMagFilter(GL_NEAREST);
		s_NoRoughness = load2DTexture(std::string("Assets/textures/default/black.png"));
		s_NoRoughness->setAnisotropicFilteringMode(1.0f);
		s_NoRoughness->setTextureMinFilter(GL_NEAREST);
		s_NoRoughness->setTextureMagFilter(GL_NEAREST);
		s_DefaultAO = load2DTexture(std::string("Assets/textures/default/white.png"));
		s_DefaultAO->setAnisotropicFilteringMode(1.0f);
		s_DefaultAO->setTextureMinFilter(GL_NEAREST);
		s_DefaultAO->setTextureMagFilter(GL_NEAREST);

		s_DefaultNormal = load2DTexture(std::string("Assets/textures/default/defaultNormal.png"), &srgbTextureSettings);
		s_DefaultNormal->setAnisotropicFilteringMode(1.0f);
		s_DefaultNormal->setTextureMinFilter(GL_NEAREST);
		s_DefaultNormal->setTextureMagFilter(GL_NEAREST);

		s_DefaultEmission = load2DTexture(std::string("Assets/textures/default/black.png"), &srgbTextureSettings);
		s_DefaultEmission->setAnisotropicFilteringMode(1.0f);
		s_DefaultEmission->setTextureMinFilter(GL_NEAREST);
		s_DefaultEmission->setTextureMagFilter(GL_NEAREST);
	}


	void TextureLoader::processMainThreadTasks() {
		std::lock_guard<std::mutex> lock(taskMutex);
		while (!mainThreadTasks.empty()) {
			auto task = mainThreadTasks.front();
			mainThreadTasks.pop();
			task();
		}
	}
}
