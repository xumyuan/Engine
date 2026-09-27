#pragma once

#include "rhi/include/RHIShaderProgram.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace engine {

	class Shader {
	private:
		std::unique_ptr<rhi::RHIShaderProgram> m_Program;
		std::string m_ShaderFilePath;

	public:
		// 接管一个已编译的 RHIShaderProgram
		Shader(const std::string& path, std::unique_ptr<rhi::RHIShaderProgram> program);
		~Shader();

		void enable() const;
		void disable() const;

		// 获取 RHI ProgramHandle
		inline rhi::ProgramHandle getProgramHandle() const {
			return m_Program ? m_Program->getProgramHandle() : rhi::ProgramHandle();
		}
	};

}
