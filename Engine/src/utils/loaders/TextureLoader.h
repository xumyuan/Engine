#pragma once

#include "graphics/texture/Texture.h"
#include "graphics/texture/Cubemap.h"

namespace engine {

	class TextureLoader {
	private:

		struct DefaultTextures {
			Texture* m_DefaultAlbedo;
			Texture* m_DefaultNormal;
			Texture* m_FullMetallic, * m_NoMetallic;
			Texture* m_FullRoughness, * m_NoRoughness;
			Texture* m_DefaultAO;
			Texture* m_DefaultEmission;
		};
	public:
		static void initializeDefaultTextures();

		// isSRGB�Ƿ�Ҫ���Ի��ı�־
		// ������ɫ����Ӧ���Ի��������ߡ��߶ȡ�������ͼ����
		static Texture* load2DTexture(const std::string& path, bool isSRGB, TextureSettings* settings = nullptr);

		static Cubemap* loadCubemapTexture(const const std::string& right, const std::string& left, const std::string& top, const std::string& bottom, const std::string& back, const std::string& front, bool isSRGB, CubemapSettings* settings = nullptr);

		inline static Texture* getDefaultAlbedo() { return m_DefaultTextures.m_DefaultAlbedo; }
		inline static Texture* getDefaultNormal() { return m_DefaultTextures.m_DefaultNormal; }
		inline static Texture* getDefaultMetallic() { return m_DefaultTextures.m_NoMetallic; }
		inline static Texture* getDefaultRoughness() { return m_DefaultTextures.m_NoRoughness; }
		inline static Texture* getDefaultAO() { return m_DefaultTextures.m_DefaultAO; }
		inline static Texture* getDefaultEmission() { return m_DefaultTextures.m_DefaultEmission; }
		inline static Texture* getFullMetallic() { return m_DefaultTextures.m_FullMetallic; }
		inline static Texture* getNoMetallic() { return m_DefaultTextures.m_NoMetallic; }
		inline static Texture* getFullRoughness() { return m_DefaultTextures.m_FullRoughness; }
		inline static Texture* getNoRoughness() { return m_DefaultTextures.m_NoRoughness; }
	private:
		static std::map<std::string, Texture> m_TextureCache;
		static DefaultTextures m_DefaultTextures;
	};


}