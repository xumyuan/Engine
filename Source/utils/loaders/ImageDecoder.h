#pragma once

namespace engine::image {

	// 可在任意线程并发调用。全局的 stbi_load 来自 SOIL 自带的旧版 stb_image，不能并发调用
	unsigned char* decodeFile(const char* path, int* width, int* height, int* channels);
	void freePixels(unsigned char* pixels);

	// 当前线程最近一次 decodeFile 失败的原因
	const char* failureReason();

}
