#include "pch.h"
#include "ShaderLoader.h"
#include "rhi/include/RHIShaderCompiler.h"

namespace engine {

	// Static declarations
	std::unordered_map<std::size_t, Shader*> ShaderLoader::s_ShaderCache;
	std::hash<std::string> ShaderLoader::s_Hasher;
	std::string ShaderLoader::s_ShaderFilePath;
	std::unique_ptr<rhi::RHIShaderCompiler> ShaderLoader::s_Compiler = nullptr;

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

		s_ShaderCache.insert(std::pair<std::size_t, Shader*>(hash, shader));
		return s_ShaderCache[hash];
	}

}
