#pragma once

#include <graphics/Shader.h>

namespace engine {

	class ShaderLoader {
	public:
		static Shader* loadShader(const std::string& path);

		inline static void setShaderFilePath(const std::string& path) { s_ShaderFilePath = path; }
	private:
		static std::string s_ShaderFilePath;
		static std::unordered_map<std::size_t, Shader*> s_ShaderCache;
		static std::hash<std::string> s_Hasher;
	};

}