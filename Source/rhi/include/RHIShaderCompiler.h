#pragma once

#include "RHITypes.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace engine {
namespace rhi {

class RHIShaderProgram;

// RHI 着色器编译器抽象接口
// 负责：
//   1. 从 .glsl 文件解析 #shader-type 标记，拆分出各阶段源码
//   2. 编译、链接着色器
//   3. 返回 RHIShaderProgram 实例
//
// 各后端（OpenGL/Vulkan/...）提供具体实现
class RHIShaderCompiler {
public:
    virtual ~RHIShaderCompiler() = default;

    // 从文件路径加载 .glsl 着色器，自动解析 #shader-type 并编译链接
    // 返回 RHIShaderProgram，失败返回 nullptr
    virtual std::unique_ptr<RHIShaderProgram> loadAndCompile(const std::string& filePath) = 0;

    // 从已拆分的源码编译链接
    // shaderSources: stage -> GLSL source code
    // 返回 RHIShaderProgram，失败返回 nullptr
    virtual std::unique_ptr<RHIShaderProgram> compileFromSources(
            const std::string& name,
            const std::unordered_map<ShaderStage, std::string>& shaderSources) = 0;
};

} // namespace rhi
} // namespace engine
