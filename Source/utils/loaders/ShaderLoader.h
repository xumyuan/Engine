#pragma once

#include <graphics/Shader.h>
#include <memory>

namespace engine {
namespace rhi {
	class RHIShaderCompiler;
}

	class ShaderLoader {
	public:
		// 提交编译后立即返回，驱动可能仍在后台编译
		static Shader* loadShader(const std::string& path);

		// 等待已提交的 shader 全部编译完成并输出错误日志，返回是否全部成功
		// 未调用时 shader 会在首次使用时逐个阻塞等待，结果相同但失去并行
		static bool finishPendingCompiles();

		inline static void setShaderFilePath(const std::string& path) { s_ShaderFilePath = path; }

		// 初始化 ShaderLoader，设置着色器编译器
		static void initialize(std::unique_ptr<rhi::RHIShaderCompiler> compiler);

	private:
		static std::string s_ShaderFilePath;
		static std::unordered_map<std::size_t, Shader*> s_ShaderCache;
		static std::hash<std::string> s_Hasher;

		static std::unique_ptr<rhi::RHIShaderCompiler> s_Compiler;
		static std::vector<std::pair<std::string, rhi::ProgramHandle>> s_PendingPrograms;
	};

}
