#pragma once

#include "rhi/include/RHIHandle.h"
#include "rhi/include/RHIResources.h"
#include "rhi/include/RHITypes.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace engine {
namespace rhi {

class OpenGLDevice;

// 着色器编译服务
// 负责：
//   1. 从 .glsl 文件解析 #shader-type 标记，拆分出各阶段源码
//   2. 将源码编译为 GL shader 对象
//   3. 链接成 GL program
//   4. 通过 OpenGLDevice::createProgram 返回 ProgramHandle
//
// 这个类把原来 Shader 类中的编译逻辑抽离到 RHI 层，
// 使上层 Shader 类不再直接调用 GL API。
class ShaderCompilerService {
public:
    explicit ShaderCompilerService(OpenGLDevice& device);
    ~ShaderCompilerService() = default;

    // 从文件路径加载 .glsl 着色器，自动解析 #shader-type 并编译链接
    // 返回 ProgramHandle，失败返回无效 handle
    ProgramHandle loadAndCompile(const std::string& filePath);

    // 从已拆分的源码编译链接（用于高级用法）
    // shaderSources: stage -> GLSL source code
    ProgramHandle compileFromSources(const std::string& name,
            const std::unordered_map<ShaderStage, std::string>& shaderSources);

    // ---------- 工具方法 ----------

    // 从 .glsl 文件内容解析 #shader-type 标记，拆分出各阶段源码
    static std::unordered_map<ShaderStage, std::string>
    preprocessShaderSource(const std::string& source);

    // 字符串 → ShaderStage
    static ShaderStage shaderStageFromString(const std::string& type);

private:
    OpenGLDevice& mDevice;
};

} // namespace rhi
} // namespace engine
