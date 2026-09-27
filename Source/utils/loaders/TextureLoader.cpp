#include "pch.h"
#include "TextureLoader.h"
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

		struct StbiDeleter {
			void operator()(unsigned char* pixels) const noexcept { stbi_image_free(pixels); }
		};

		struct DecodedImage {
			std::unique_ptr<unsigned char, StbiDeleter> pixels;
			int width = 0;
			int height = 0;
			ChannelLayout channels = ChannelLayout::RGBA;

			explicit operator bool() const { return pixels != nullptr; }
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
			image.pixels.reset(stbi_load(path.c_str(), &image.width, &image.height, &numComponents, 0));
			image.channels = channelsFromCount(numComponents);
			return image;
		}

		// 只包含决定 GPU 资源内容与格式的字段；采样参数不参与，否则同一张图会因过滤方式不同重复占用显存
		struct TextureKey {
			std::string path;
			bool srgb = false;
			bool explicitFormat = false;
			rhi::TextureFormat format = rhi::TextureFormat::RGBA8;
			bool mips = true;

			bool operator==(const TextureKey&) const = default;
		};

		struct TextureKeyHash {
			size_t operator()(const TextureKey& key) const noexcept {
				size_t h = std::hash<std::string>{}(key.path);
				auto mix = [&h](size_t v) { h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); };
				mix(key.srgb);
				mix(key.explicitFormat);
				mix(static_cast<size_t>(key.format));
				mix(key.mips);
				return h;
			}
		};

		TextureKey makeKey(const std::string& path, const TextureSettings& settings) {
			TextureKey key;
			key.path = std::filesystem::path(path).lexically_normal().generic_string();
			key.srgb = settings.IsSRGB;
			key.explicitFormat = settings.formatExplicitlySet;
			if (settings.formatExplicitlySet) {
				key.format = settings.format;
			}
			key.mips = settings.HasMips;
			return key;
		}

		bool samplerSettingsDiffer(const TextureSettings& a, const TextureSettings& b) {
			return a.wrapS != b.wrapS || a.wrapT != b.wrapT
				|| a.minFilter != b.minFilter || a.magFilter != b.magFilter
				|| a.anisotropy != b.anisotropy || a.HasBorder != b.HasBorder || a.MipBias != b.MipBias;
		}

		enum class LoadState : uint8_t { Loading, Ready, Failed };

		struct TextureEntry {
			std::unique_ptr<Texture> texture;
			LoadState state = LoadState::Loading;
		};

		struct TextureCache {
			std::unordered_map<TextureKey, TextureEntry, TextureKeyHash> entries;

			// 未调用 shutdown() 时 RHI 设备可能已销毁，此时析构 Texture 会访问悬空设备，只能放弃释放
			~TextureCache() {
				for (auto& [key, entry] : entries) {
					(void)entry.texture.release();
				}
			}
		};

		TextureCache s_Cache;

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

		void finishTextureLoad(const TextureKey& key, Texture* texture, const DecodedImage& image) {
			auto it = s_Cache.entries.find(key);
			if (it == s_Cache.entries.end() || it->second.texture.get() != texture) {
				return;
			}
			if (!image) {
				spdlog::error("texture load fail - path:{0}", key.path);
				it->second.state = LoadState::Failed;
				return;
			}
			texture->generate2DTexture(static_cast<unsigned>(image.width), static_cast<unsigned>(image.height),
				image.channels, image.pixels.get());
			it->second.state = LoadState::Ready;
		}

		struct CubemapRequest {
			Cubemap* cubemap = nullptr;
			std::array<std::string, 6> paths;
			std::array<DecodedImage, 6> faces;
			std::atomic<int> remaining{ 6 };
		};

		void finishCubemapLoad(const CubemapRequest& request) {
			for (size_t i = 0; i < request.faces.size(); ++i) {
				if (!request.faces[i]) {
					spdlog::error("Couldn't load cubemap using 6 filepaths. Filepath error: {0}", request.paths[i]);
					return;
				}
			}
			for (uint8_t i = 0; i < 6; ++i) {
				const DecodedImage& face = request.faces[i];
				request.cubemap->generateCubemapFace(i, static_cast<unsigned>(face.width), static_cast<unsigned>(face.height),
					face.channels, face.pixels.get());
			}
		}
	}

	Texture* TextureLoader::load2DTexture(const std::string& path, TextureSettings* settings) {
		assert(isMainThread() && "TextureLoader must be used on the main thread");

		const TextureSettings requested = (settings != nullptr) ? *settings : TextureSettings{};
		TextureKey key = makeKey(path, requested);

		if (auto it = s_Cache.entries.find(key); it != s_Cache.entries.end()) {
			Texture* cached = it->second.texture.get();
			if (samplerSettingsDiffer(cached->getTextureSettings(), requested)) {
				spdlog::warn("texture '{}' requested again with different sampler settings, keeping the first ones", key.path);
			}
			return cached;
		}

		TextureEntry& entry = s_Cache.entries[key];
		entry.texture = std::make_unique<Texture>(requested);
		Texture* texture = entry.texture.get();

		try {
			pendingLoads().run([key, texture]() {
				auto image = std::make_shared<DecodedImage>(decodeImage(key.path));
				postToMainThread([key, texture, image]() {
					finishTextureLoad(key, texture, *image);
				});
			});
		}
		catch (...) {
			s_Cache.entries.erase(key);
			throw;
		}
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

		auto request = std::make_shared<CubemapRequest>();
		request->cubemap = cubemap;
		request->paths = { right, left, top, bottom, back, front };

		for (size_t i = 0; i < request->paths.size(); ++i) {
			pendingLoads().run([request, i]() {
				request->faces[i] = decodeImage(request->paths[i]);
				if (request->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
					postToMainThread([request]() { finishCubemapLoad(*request); });
				}
			});
		}

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

		try {
			pendingLoads().wait();
		}
		catch (const std::exception& e) {
			spdlog::error("texture loading task failed: {}", e.what());
		}
		catch (...) {
			spdlog::error("texture loading task failed with unknown exception");
		}

		{
			std::lock_guard lock(s_TaskMutex);
			s_MainThreadTasks.clear();
		}
		s_Cache.entries.clear();

		s_DefaultAlbedo = s_DefaultNormal = nullptr;
		s_FullMetallic = s_NoMetallic = nullptr;
		s_FullRoughness = s_NoRoughness = nullptr;
		s_DefaultAO = s_DefaultEmission = nullptr;
	}
}
