#include "pch.h"
#include "TextureLoader.h"
#include "thread/ThreadPool.h"

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

	// 将 stbi 通道数转为 ChannelLayout
	static ChannelLayout channelsFromCount(int numComponents) {
		switch (numComponents) {
		case 1: return ChannelLayout::R;
		case 2: return ChannelLayout::RG;
		case 3: return ChannelLayout::RGB;
		case 4: return ChannelLayout::RGBA;
		default: return ChannelLayout::RGBA;
		}
	}

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

			ChannelLayout channels = channelsFromCount(numComponents);

			{
				std::lock_guard<std::mutex> lock(taskMutex);
				mainThreadTasks.push([=] {
					texture->generate2DTexture(width, height, channels, data);
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
				ChannelLayout channels = channelsFromCount(numComponents);
				cubemap->generateCubemapFace(static_cast<uint8_t>(i), width, height, channels, data);
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
		TextureSettings srgbTextureSettings;
		srgbTextureSettings.IsSRGB = true;

		s_DefaultAlbedo = load2DTexture(std::string("Assets/textures/default/defaultAlbedo.png"), &srgbTextureSettings);
		s_DefaultAlbedo->setAnisotropicFilteringMode(1.0f);
		s_DefaultAlbedo->setTextureMinFilter(rhi::FilterMode::Nearest);
		s_DefaultAlbedo->setTextureMagFilter(rhi::FilterMode::Nearest);
		s_DefaultNormal = load2DTexture(std::string("Assets/textures/default/defaultNormal.png"));
		s_DefaultNormal->setAnisotropicFilteringMode(1.0f);
		s_DefaultNormal->setTextureMinFilter(rhi::FilterMode::Nearest);
		s_DefaultNormal->setTextureMagFilter(rhi::FilterMode::Nearest);
		s_FullMetallic = load2DTexture(std::string("Assets/textures/default/white.png"));
		s_FullMetallic->setAnisotropicFilteringMode(1.0f);
		s_FullMetallic->setTextureMinFilter(rhi::FilterMode::Nearest);
		s_FullMetallic->setTextureMagFilter(rhi::FilterMode::Nearest);
		s_NoMetallic = load2DTexture(std::string("Assets/textures/default/black.png"));
		s_NoMetallic->setAnisotropicFilteringMode(1.0f);
		s_NoMetallic->setTextureMinFilter(rhi::FilterMode::Nearest);
		s_NoMetallic->setTextureMagFilter(rhi::FilterMode::Nearest);
		s_FullRoughness = load2DTexture(std::string("Assets/textures/default/white.png"));
		s_FullRoughness->setAnisotropicFilteringMode(1.0f);
		s_FullRoughness->setTextureMinFilter(rhi::FilterMode::Nearest);
		s_FullRoughness->setTextureMagFilter(rhi::FilterMode::Nearest);
		s_NoRoughness = load2DTexture(std::string("Assets/textures/default/black.png"));
		s_NoRoughness->setAnisotropicFilteringMode(1.0f);
		s_NoRoughness->setTextureMinFilter(rhi::FilterMode::Nearest);
		s_NoRoughness->setTextureMagFilter(rhi::FilterMode::Nearest);
		s_DefaultAO = load2DTexture(std::string("Assets/textures/default/white.png"));
		s_DefaultAO->setAnisotropicFilteringMode(1.0f);
		s_DefaultAO->setTextureMinFilter(rhi::FilterMode::Nearest);
		s_DefaultAO->setTextureMagFilter(rhi::FilterMode::Nearest);

		s_DefaultNormal = load2DTexture(std::string("Assets/textures/default/defaultNormal.png"), &srgbTextureSettings);
		s_DefaultNormal->setAnisotropicFilteringMode(1.0f);
		s_DefaultNormal->setTextureMinFilter(rhi::FilterMode::Nearest);
		s_DefaultNormal->setTextureMagFilter(rhi::FilterMode::Nearest);

		s_DefaultEmission = load2DTexture(std::string("Assets/textures/default/black.png"), &srgbTextureSettings);
		s_DefaultEmission->setAnisotropicFilteringMode(1.0f);
		s_DefaultEmission->setTextureMinFilter(rhi::FilterMode::Nearest);
		s_DefaultEmission->setTextureMagFilter(rhi::FilterMode::Nearest);
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
