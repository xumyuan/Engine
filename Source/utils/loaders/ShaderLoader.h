#pragma once

#include <graphics/Shader.h>

namespace engine {
namespace rhi {
	class OpenGLDevice;
	class ShaderCompilerService;
}

	class ShaderLoader {
	public:
		static Shader* loadShader(const std::string& path);

		inline static void setShaderFilePath(const std::string& path) { s_ShaderFilePath = path; }

		// 初始化 ShaderLoader，设置 RHI 设备和编译器服务
		static void initialize(rhi::OpenGLDevice& device, rhi::ShaderCompilerService& compiler);

	private:
		static std::string s_ShaderFilePath;
		static std::unordered_map<std::size_t, Shader*> s_ShaderCache;
		static std::hash<std::string> s_Hasher;

		static rhi::OpenGLDevice* s_Device;
		static rhi::ShaderCompilerService* s_Compiler;
	};

}
