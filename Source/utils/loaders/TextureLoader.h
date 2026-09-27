#pragma once

#include "graphics/texture/Texture.h"
#include "graphics/texture/Cubemap.h"

namespace engine {

	// 所有接口只能在主线程（持有 GL context 的线程）调用
	class TextureLoader {
	public:
		// 返回前默认纹理已上传完成
		static void initializeDefaultTextures();

		// 在 RHI 设备销毁前调用：等待后台解码，丢弃未上传的结果，释放缓存中的全部纹理
		static void shutdown();

		// 立即返回，像素在后台解码，processMainThreadTasks() 时上传
		// 上传前或加载失败时 isGenerated() 为 false，调用方应退回默认纹理
		// isSRGB是否要线性化的标志
		// 所有颜色纹理都应线性化，而法线、高度、金属贴图不用
		static Texture* load2DTexture(const std::string& path, TextureSettings* settings = nullptr);

		// 返回的 Cubemap 由调用方持有；六个面在后台并行解码，全部成功后才上传
		static Cubemap* loadCubemapTexture(const std::string& right, const std::string& left, const std::string& top, const std::string& bottom, const std::string& back, const std::string& front, CubemapSettings* settings = nullptr);

		// 等待已提交的后台解码任务结束（不包含主线程上传）
		static void waitForPendingLoads();
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
		// Default Textures
		static Texture* s_DefaultAlbedo;
		static Texture* s_DefaultNormal;
		static Texture* s_FullMetallic, * s_NoMetallic;
		static Texture* s_FullRoughness, * s_NoRoughness;
		static Texture* s_DefaultAO;
		static Texture* s_DefaultEmission;
	};


}
