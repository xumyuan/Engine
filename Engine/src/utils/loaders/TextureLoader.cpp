#include "TextureLoader.h"

namespace engine {
	namespace utils {
		std::map<std::string, graphics::Texture> TextureLoader::m_TextureCache;
		TextureLoader::DefaultTextures TextureLoader::m_DefaultTextures;

		graphics::Texture* TextureLoader::Load2DTexture(const std::string& path) {
			// Check the cache
			std::map<std::string, graphics::Texture>::iterator iter = m_TextureCache.find(path);
			if (iter != m_TextureCache.end()) {
				return &iter->second;
			}

			// Load the texture
			int width, height, numComponents;
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &numComponents, 0);
			if (!data) {
				utils::Logger::getInstance().error("logged_files/texture_loading.txt", "texture load fail - path:", path);
				stbi_image_free(data);
				return nullptr;
			}

			GLenum format;
			switch (numComponents) {
			case 1: format = GL_RED;  break;
			case 3: format = GL_RGB;  break;
			case 4: format = GL_RGBA; break;
			}

			graphics::Texture texture;
			texture.Generate2DTexture(width, height, format, format, data);

			m_TextureCache.insert(std::pair<std::string, graphics::Texture>(path, texture));

			return &m_TextureCache[path];
		}

		void TextureLoader::InitializeDefaultTextures() {
			m_DefaultTextures.m_DefaultDiffuse = Load2DTexture(std::string("res/textures/default/defaultDiffuse.png"));
			m_DefaultTextures.m_FullSpecular = Load2DTexture(std::string("res/textures/default/fullSpec.png"));
			m_DefaultTextures.m_NoSpecular = Load2DTexture(std::string("res/textures/default/noSpec.png"));
			m_DefaultTextures.m_DefaultNormal = Load2DTexture(std::string("res/textures/default/defaultNormal.png"));
			m_DefaultTextures.m_DefaultEmission = Load2DTexture(std::string("res/textures/default/defaultEmission.png"));
		}

	}
}