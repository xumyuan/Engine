#pragma once

#include "rhi/include/RHIShaderCompiler.h"
#include "rhi/include/RHITypes.h"
#include <string>
#include <unordered_map>

namespace engine {
namespace rhi {

class OpenGLDevice;

// OpenGL 后端的着色器编译器实现
// 负责从 .glsl 文件或已拆分的源码编译链接 GL program，
// 并返回 OpenGLShaderProgram 实例
class OpenGLShaderCompiler final : public RHIShaderCompiler {
public:
    explicit OpenGLShaderCompiler(OpenGLDevice& device);
    ~OpenGLShaderCompiler() override = default;

    // ---------- RHIShaderCompiler 接口 ----------
    std::unique_ptr<RHIShaderProgram> loadAndCompile(const std::string& filePath) override;
    std::unique_ptr<RHIShaderProgram> compileFromSources(
            const std::string& name,
            const std::unordered_map<ShaderStage, std::string>& shaderSources) override;

    // ---------- 工具方法 ----------

    // 从 .glsl 文件内容解析 #shader-type 标记，拆分出各阶段源码，并展开 #include 指令
    static std::unordered_map<ShaderStage, std::string>
    preprocessShaderSource(const std::string& source, const std::string& filePath = "");

    // 递归展开单个着色器阶段源码中的 #include "..." 指令
    static std::string
    resolveIncludes(const std::string& source, const std::string& filePath);

    // 字符串 → ShaderStage
    static ShaderStage shaderStageFromString(const std::string& type);

private:
    OpenGLDevice& mDevice;
};

} // namespace rhi
} // namespace engine
