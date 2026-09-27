// 本文件不使用预编译头（见 CMakeLists.txt）：pch.h 以外部链接声明了 stbi_*，与 STB_IMAGE_STATIC 冲突
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4505)
#endif

#include <stb_image.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "ImageDecoder.h"

namespace engine::image {

	unsigned char* decodeFile(const char* path, int* width, int* height, int* channels) {
		return stbi_load(path, width, height, channels, 0);
	}

	void freePixels(unsigned char* pixels) {
		stbi_image_free(pixels);
	}

	const char* failureReason() {
		return stbi_failure_reason();
	}

}
