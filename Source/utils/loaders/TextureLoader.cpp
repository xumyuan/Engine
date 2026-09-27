#include "pch.h"
#include "TextureLoader.h"
#include "ImageDecoder.h"
#include "thread/ThreadPool.h"

#include <cassert>
#include <filesystem>
#include <mutex>

namespace engine {

	// Static declarations
	Texture* TextureLoader::s_DefaultAlbedo = nullptr;
	Texture* TextureLoader::s_DefaultNormal = nullptr;
	Texture* TextureLoader::s_FullMetallic = nullptr, * TextureLoader::s_NoMetallic = nullptr;
	Texture* TextureLoader::s_FullRoughness = nullptr, * TextureLoader::s_NoRoughness = nullptr;
	Texture* TextureLoader::s_DefaultAO = nullptr;
	Texture* TextureLoader::s_DefaultEmission = nullptr;

	namespace {

		struct PixelDeleter {
			void operator()(unsigned char* pixels) const noexcept { image::freePixels(pixels); }
		};

		struct DecodedImage {
			std::unique_ptr<unsigned char, PixelDeleter> pixels;
			int width = 0;
			int height = 0;
			ChannelLayout channels = ChannelLayout::RGBA;
			std::string error;
		};

		// 将 stbi 通道数转为 ChannelLayout
		ChannelLayout channelsFromCount(int numComponents) {
			switch (numComponents) {
			case 1: return ChannelLayout::R;
			case 2: return ChannelLayout::RG;
			case 3: return ChannelLayout::RGB;
			case 4: return ChannelLayout::RGBA;
			default: return ChannelLayout::RGBA;
			}
		}

		DecodedImage decodeImage(const std::string& path) {
			DecodedImage image;
			int numComponents = 0;
			image.pixels.reset(image::decodeFile(path.c_str(), &image.width, &image.height, &numComponents));
			if (!image.pixels) {
				const char* reason = image::failureReason();
				image.error = reason ? reason : "unknown";
			}
			image.channels = channelsFromCount(numComponents);
			return image;
		}

		std::string normalizePath(const std::string& path) {
			return std::filesystem::path(path).lexically_normal().generic_string();
		}

		// 只包含决定 GPU 资源内容与格式的设置；采样参数不参与，否则同一张图会因过滤方式不同重复占用显存
		std::string makeCacheKey(const std::string& normalizedPath, const TextureSettings& settings) {
			std::string key = normalizedPath;
			key += settings.IsSRGB ? "|srgb" : "|linear";
			key += settings.formatExplicitlySet ? "|fmt=" + std::to_string(static_cast<int>(settings.format)) : "|fmt=auto";
			key += settings.HasMips ? "|mips" : "|nomips";
			return key;
		}

		bool samplerSettingsDiffer(const TextureSettings& a, const TextureSettings& b) {
			return a.wrapS != b.wrapS || a.wrapT != b.wrapT
				|| a.minFilter != b.minFilter || a.magFilter != b.magFilter
				|| a.anisotropy != b.anisotropy || a.HasBorder != b.HasBorder || a.MipBias != b.MipBias;
		}

		// 有意不析构：未调用 shutdown() 时，静态析构阶段 RHI 设备已销毁，不能再释放纹理
		auto& s_Cache = *new std::unordered_map<std::string, std::unique_ptr<Texture>>();

		std::mutex s_TaskMutex;
		std::vector<std::function<void()>> s_MainThreadTasks;

		std::thread::id s_MainThreadId;

		bool isMainThread() {
			return s_MainThreadId == std::thread::id{} || s_MainThreadId == std::this_thread::get_id();
		}

		TaskGroup& pendingLoads() {
			static TaskGroup group(globalThreadPool());
			return group;
		}

		void postToMainThread(std::function<void()> task) {
			std::lock_guard lock(s_TaskMutex);
			s_MainThreadTasks.push_back(std::move(task));
		}
	}

	Texture* TextureLoader::load2DTexture(const std::string& path, TextureSettings* settings) {
		assert(isMainThread() && "TextureLoader must be used on the main thread");

		const TextureSettings requested = (settings != nullptr) ? *settings : TextureSettings{};
		std::string normalizedPath = normalizePath(path);
		std::string key = makeCacheKey(normalizedPath, requested);

		if (auto it = s_Cache.find(key); it != s_Cache.end()) {
			if (samplerSettingsDiffer(it->second->getTextureSettings(), requested)) {
				spdlog::warn("texture '{}' requested again with different sampler settings, keeping the first ones", normalizedPath);
			}
			return it->second.get();
		}

		Texture* texture = (s_Cache[key] = std::make_unique<Texture>(requested)).get();

		pendingLoads().run([normalizedPath, texture]() {
			auto image = std::make_shared<DecodedImage>(decodeImage(normalizedPath));
			postToMainThread([normalizedPath, texture, image]() {
				if (!image->pixels) {
					spdlog::error("texture load fail - path:{0}, reason: {1}", normalizedPath, image->error);
					return;
				}
				texture->generate2DTexture(static_cast<unsigned>(image->width), static_cast<unsigned>(image->height),
					image->channels, image->pixels.get());
			});
		});
		return texture;
	}

	void TextureLoader::waitForPendingLoads() {
		pendingLoads().wait();
	}

	Cubemap* TextureLoader::loadCubemapTexture(const std::string& right, const std::string& left, const std::string& top, const std::string& bottom, const std::string& back, const std::string& front, CubemapSettings* settings) {
		assert(isMainThread() && "TextureLoader must be used on the main thread");

		Cubemap* cubemap = new Cubemap();
		if (settings != nullptr)
			cubemap->setCubemapSettings(*settings);

		std::array<std::string, 6> paths = { right, left, top, bottom, back, front };

		pendingLoads().run([cubemap, paths]() {
			auto faces = std::make_shared<std::array<DecodedImage, 6>>();
			for (size_t i = 0; i < paths.size(); ++i) {
				(*faces)[i] = decodeImage(paths[i]);
			}
			postToMainThread([cubemap, paths, faces]() {
				for (size_t i = 0; i < paths.size(); ++i) {
					if (!(*faces)[i].pixels) {
						spdlog::error("Couldn't load cubemap using 6 filepaths. Filepath error: {0}, reason: {1}",
							paths[i], (*faces)[i].error);
						return;
					}
				}
				for (uint8_t i = 0; i < 6; ++i) {
					const DecodedImage& face = (*faces)[i];
					cubemap->generateCubemapFace(i, static_cast<unsigned>(face.width), static_cast<unsigned>(face.height),
						face.channels, face.pixels.get());
				}
			});
		});

		return cubemap;
	}

	void TextureLoader::initializeDefaultTextures() {
		s_MainThreadId = std::this_thread::get_id();

		TextureSettings dataSettings;
		dataSettings.anisotropy = 1.0f;
		dataSettings.minFilter = rhi::FilterMode::Nearest;
		dataSettings.magFilter = rhi::FilterMode::Nearest;

		TextureSettings colorSettings = dataSettings;
		colorSettings.IsSRGB = true;

		s_DefaultAlbedo = load2DTexture("Assets/textures/default/defaultAlbedo.png", &colorSettings);
		s_DefaultNormal = load2DTexture("Assets/textures/default/defaultNormal.png", &dataSettings);
		s_FullMetallic = load2DTexture("Assets/textures/default/white.png", &dataSettings);
		s_NoMetallic = load2DTexture("Assets/textures/default/black.png", &dataSettings);
		s_FullRoughness = load2DTexture("Assets/textures/default/white.png", &dataSettings);
		s_NoRoughness = load2DTexture("Assets/textures/default/black.png", &dataSettings);
		s_DefaultAO = load2DTexture("Assets/textures/default/white.png", &dataSettings);
		s_DefaultEmission = load2DTexture("Assets/textures/default/black.png", &colorSettings);

		// 材质在纹理未就绪时退回默认纹理，默认纹理自身必须立即可用
		waitForPendingLoads();
		processMainThreadTasks();
	}

	void TextureLoader::processMainThreadTasks() {
		assert(isMainThread() && "TextureLoader must be used on the main thread");

		std::vector<std::function<void()>> tasks;
		{
			std::lock_guard lock(s_TaskMutex);
			tasks.swap(s_MainThreadTasks);
		}
		for (auto& task : tasks) {
			task();
			task = nullptr; // 上传完立即释放该任务持有的像素，避免整批像素同时驻留
		}
	}

	void TextureLoader::shutdown() {
		assert(isMainThread() && "TextureLoader must be used on the main thread");

		pendingLoads().wait();
		{
			std::lock_guard lock(s_TaskMutex);
			s_MainThreadTasks.clear();
		}
		s_Cache.clear();

		s_DefaultAlbedo = s_DefaultNormal = nullptr;
		s_FullMetallic = s_NoMetallic = nullptr;
		s_FullRoughness = s_NoRoughness = nullptr;
		s_DefaultAO = s_DefaultEmission = nullptr;
	}
}
