#pragma once

#include "graphics/texture/Texture.h"
#include "graphics/texture/Cubemap.h"

namespace engine {

	class TextureLoader {
	public:
		static void initializeDefaultTextures();

		// isSRGB是否要线性化的标志
		// 所有颜色纹理都应线性化，而法线、高度、金属贴图不用
		static Texture* load2DTexture(const std::string& path, TextureSettings* settings = nullptr);

		static Cubemap* loadCubemapTexture(const std::string& right, const std::string& left, const std::string& top, const std::string& bottom, const std::string& back, const std::string& front, CubemapSettings* settings = nullptr);

		static void processMainThreadTasks(); 

		inline static Texture* getDefaultAlbedo() { return s_DefaultAlbedo; }
		inline static Texture* getDefaultNormal() { return s_DefaultNormal; }
		inline static Texture* getDefaultMetallic() { return s_NoMetallic; }
		inline static Texture* getDefaultRoughness() { return s_NoRoughness; }
		inline static Texture* getDefaultAO() { return s_DefaultAO; }
		inline static Texture* getDefaultEmission() { return s_DefaultEmission; }
		inline static Texture* getFullMetallic() { return s_FullMetallic; }
		inline static Texture* getNoMetallic() { return s_NoMetallic; }
		inline static Texture* getFullRoughness() { return s_FullRoughness; }
		inline static Texture* getNoRoughness() { return s_NoRoughness; }
	private:
		static std::unordered_map<std::string, Texture*> m_TextureCache;

		// Default Textures
		static Texture* s_DefaultAlbedo;
		static Texture* s_DefaultNormal;
		static Texture* s_FullMetallic, * s_NoMetallic;
		static Texture* s_FullRoughness, * s_NoRoughness;
		static Texture* s_DefaultAO;
		static Texture* s_DefaultEmission;


		static std::queue<std::function<void()>> mainThreadTasks;
		static std::mutex taskMutex;
	};


}