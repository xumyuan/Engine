#include "pch.h"
#include "ShaderLoader.h"
#include "rhi/include/RHIShaderCompiler.h"
#include "rhi/include/RHIDevice.h"

#include <chrono>

namespace engine {

	// Static declarations
	std::unordered_map<std::size_t, Shader*> ShaderLoader::s_ShaderCache;
	std::hash<std::string> ShaderLoader::s_Hasher;
	std::string ShaderLoader::s_ShaderFilePath;
	std::unique_ptr<rhi::RHIShaderCompiler> ShaderLoader::s_Compiler = nullptr;
	std::vector<std::pair<std::string, rhi::ProgramHandle>> ShaderLoader::s_PendingPrograms;

	void ShaderLoader::initialize(std::unique_ptr<rhi::RHIShaderCompiler> compiler) {
		s_Compiler = std::move(compiler);
	}

	Shader* ShaderLoader::loadShader(const std::string& path) {
		std::string shaderPath = s_ShaderFilePath + path;
		std::size_t hash = s_Hasher(shaderPath);

		// Check the cache
		auto iter = s_ShaderCache.find(hash);
		if (iter != s_ShaderCache.end()) {
			return iter->second;
		}

		// Compile shader via RHI ShaderCompiler
		assert(s_Compiler && "ShaderLoader::initialize() must be called before loadShader()");
		auto program = s_Compiler->loadAndCompile(shaderPath);
		Shader* shader = new Shader(shaderPath, std::move(program));

		if (rhi::ProgramHandle handle = shader->getProgramHandle()) {
			s_PendingPrograms.emplace_back(shaderPath, handle);
		}

		s_ShaderCache.insert(std::pair<std::size_t, Shader*>(hash, shader));
		return s_ShaderCache[hash];
	}

	bool ShaderLoader::finishPendingCompiles() {
		if (s_PendingPrograms.empty()) {
			return true;
		}

		rhi::RHIDevice* device = getRHIDevice();
		assert(device && "setRHIDevice() must be called before finishPendingCompiles()");

		// 所有 program 都已提交，驱动在后台并行编译；逐个等待的总耗时约等于最慢的那个
		auto start = std::chrono::steady_clock::now();
		size_t failed = 0;
		for (auto& [path, handle] : s_PendingPrograms) {
			if (!device->waitProgram(handle)) {
				spdlog::error("[ShaderLoader] Shader failed: {}", path);
				++failed;
			}
		}
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

		spdlog::info("[ShaderLoader] {} shader(s) finished in {} ms, {} failed",
			s_PendingPrograms.size(), elapsed.count(), failed);
		s_PendingPrograms.clear();
		return failed == 0;
	}

}
