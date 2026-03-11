#pragma once

#include <graphics/Shader.h>
#include <memory>

namespace engine {
namespace rhi {
	class RHIShaderCompiler;
}

	class ShaderLoader {
	public:
		static Shader* loadShader(const std::string& path);

		inline static void setShaderFilePath(const std::string& path) { s_ShaderFilePath = path; }

		// 初始化 ShaderLoader，设置着色器编译器
		static void initialize(std::unique_ptr<rhi::RHIShaderCompiler> compiler);

	private:
		static std::string s_ShaderFilePath;
		static std::unordered_map<std::size_t, Shader*> s_ShaderCache;
		static std::hash<std::string> s_Hasher;

		static std::unique_ptr<rhi::RHIShaderCompiler> s_Compiler;
	};

}
