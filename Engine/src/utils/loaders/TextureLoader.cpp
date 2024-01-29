#include "TextureLoader.h"

namespace engine {
	namespace utils {
		std::map<std::string, graphics::Texture> TextureLoader::m_TextureCache;
		TextureLoader::DefaultTextures TextureLoader::m_DefaultTextures;

		graphics::Texture* TextureLoader::load2DTexture(const std::string& path) {
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
			texture.generate2DTexture(width, height, format, format, data);

			m_TextureCache.insert(std::pair<std::string, graphics::Texture>(path, texture));

			return &m_TextureCache[path];
		}

		graphics::Cubemap* TextureLoader::loadCubemapTexture(const std::string& right, const std::string& left, const std::string& top, const std::string& bottom, const std::string& back, const std::string& front) {
			graphics::Cubemap* cubemap = new graphics::Cubemap();

			std::vector<std::string> faces = { right, left, top, bottom, back, front };

			// Load the textures for the cubemap
			int width, height, numComponents;
			for (unsigned int i = 0; i < 6; ++i) {
				unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &numComponents, 0);

				if (data) {
					GLenum format;
					switch (numComponents) {
					case 1: format = GL_RED;  break;
					case 3: format = GL_RGB;  break;
					case 4: format = GL_RGBA; break;
					}

					cubemap->generateCubemapFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, width, height, format, format, data);
					stbi_image_free(data);
				}
				else {
					utils::Logger::getInstance().error("logged_files/error.txt", "Cubemap initialization", "Couldn't load cubemap using 6 filepaths. Filepath error: " + faces[i]);
					stbi_image_free(data);
					return cubemap;
				}
			}

			return cubemap;
		}

		void TextureLoader::initializeDefaultTextures() {
			m_DefaultTextures.m_DefaultDiffuse = load2DTexture(std::string("res/textures/default/defaultDiffuse.png"));
			m_DefaultTextures.m_DefaultDiffuse->setAnisotropicFilteringMode(1.0f, true);
			m_DefaultTextures.m_DefaultDiffuse->setTextureMinFilter(GL_NEAREST);
			m_DefaultTextures.m_DefaultDiffuse->setTextureMagFilter(GL_NEAREST);

			m_DefaultTextures.m_FullSpecular = load2DTexture(std::string("res/textures/default/fullSpec.png"));
			m_DefaultTextures.m_FullSpecular->setAnisotropicFilteringMode(1.0f, true);
			m_DefaultTextures.m_FullSpecular->setTextureMinFilter(GL_NEAREST);
			m_DefaultTextures.m_FullSpecular->setTextureMagFilter(GL_NEAREST);

			m_DefaultTextures.m_NoSpecular = load2DTexture(std::string("res/textures/default/noSpec.png"));
			m_DefaultTextures.m_NoSpecular->setAnisotropicFilteringMode(1.0f, true);
			m_DefaultTextures.m_NoSpecular->setTextureMinFilter(GL_NEAREST);
			m_DefaultTextures.m_NoSpecular->setTextureMagFilter(GL_NEAREST);

			m_DefaultTextures.m_DefaultNormal = load2DTexture(std::string("res/textures/default/defaultNormal.png"));
			m_DefaultTextures.m_DefaultNormal->setAnisotropicFilteringMode(1.0f, true);
			m_DefaultTextures.m_DefaultNormal->setTextureMinFilter(GL_NEAREST);
			m_DefaultTextures.m_DefaultNormal->setTextureMagFilter(GL_NEAREST);

			m_DefaultTextures.m_DefaultEmission = load2DTexture(std::string("res/textures/default/defaultEmission.png"));
			m_DefaultTextures.m_DefaultEmission->setAnisotropicFilteringMode(1.0f, true);
			m_DefaultTextures.m_DefaultEmission->setTextureMinFilter(GL_NEAREST);
			m_DefaultTextures.m_DefaultEmission->setTextureMagFilter(GL_NEAREST);
		}

	}
}