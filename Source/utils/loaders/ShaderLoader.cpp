#include "pch.h"
#include "ShaderLoader.h"
#include "rhi/opengl/OpenGLDevice.h"
#include "rhi/opengl/ShaderCompilerService.h"

namespace engine {

	// Static declarations
	std::unordered_map<std::size_t, Shader*> ShaderLoader::s_ShaderCache;
	std::hash<std::string> ShaderLoader::s_Hasher;
	std::string ShaderLoader::s_ShaderFilePath;
	rhi::OpenGLDevice* ShaderLoader::s_Device = nullptr;
	rhi::ShaderCompilerService* ShaderLoader::s_Compiler = nullptr;

	void ShaderLoader::initialize(rhi::OpenGLDevice& device, rhi::ShaderCompilerService& compiler) {
		s_Device = &device;
		s_Compiler = &compiler;
	}

	Shader* ShaderLoader::loadShader(const std::string& path) {
		std::string shaderPath = s_ShaderFilePath + path;
		std::size_t hash = s_Hasher(shaderPath);

		// Check the cache
		auto iter = s_ShaderCache.find(hash);
		if (iter != s_ShaderCache.end()) {
			return iter->second;
		}

		// Load the shader via RHI path
		assert(s_Device && s_Compiler && "ShaderLoader::initialize() must be called before loadShader()");
		Shader* shader = new Shader(shaderPath, *s_Compiler, *s_Device);

		s_ShaderCache.insert(std::pair<std::size_t, Shader*>(hash, shader));
		return s_ShaderCache[hash];
	}

}
