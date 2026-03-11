#include "OpenGLShaderCompiler.h"
#include "OpenGLDevice.h"
#include "OpenGLShaderProgram.h"
#include "utils/FileUtils.h"

#include <spdlog/spdlog.h>

namespace engine {
namespace rhi {

OpenGLShaderCompiler::OpenGLShaderCompiler(OpenGLDevice& device)
    : mDevice(device) {
}

// ============================================================================
// 字符串 → ShaderStage
// ============================================================================

ShaderStage OpenGLShaderCompiler::shaderStageFromString(const std::string& type) {
    if (type == "vertex")    return ShaderStage::Vertex;
    if (type == "fragment")  return ShaderStage::Fragment;
    if (type == "geometry")  return ShaderStage::Geometry;
    if (type == "hull")      return ShaderStage::TessControl;
    if (type == "domain")    return ShaderStage::TessEvaluation;
    if (type == "compute")   return ShaderStage::Compute;

    spdlog::error("[OpenGLShaderCompiler] Unknown shader type: '{}'", type);
    return ShaderStage::Vertex; // fallback
}

// ============================================================================
// 预处理 #shader-type 标记
// ============================================================================

std::unordered_map<ShaderStage, std::string>
OpenGLShaderCompiler::preprocessShaderSource(const std::string& source) {
    std::unordered_map<ShaderStage, std::string> shaderSources;

    const char* shaderTypeToken = "#shader-type";
    size_t shaderTypeTokenLength = strlen(shaderTypeToken);
    size_t pos = source.find(shaderTypeToken);

    while (pos != std::string::npos) {
        size_t eol = source.find_first_of("\r\n", pos);
        if (eol == std::string::npos) break;

        size_t begin = pos + shaderTypeTokenLength + 1;
        std::string shaderType = source.substr(begin, eol - begin);

        size_t nextLinePos = source.find_first_not_of("\r\n", eol);
        pos = source.find(shaderTypeToken, nextLinePos);

        std::string code;
        if (nextLinePos != std::string::npos) {
            if (pos != std::string::npos) {
                code = source.substr(nextLinePos, pos - nextLinePos);
            } else {
                code = source.substr(nextLinePos);
            }
        }

        ShaderStage stage = shaderStageFromString(shaderType);
        shaderSources[stage] = std::move(code);
    }

    return shaderSources;
}

// ============================================================================
// 从文件加载并编译
// ============================================================================

std::unique_ptr<RHIShaderProgram> OpenGLShaderCompiler::loadAndCompile(const std::string& filePath) {
    std::string source = FileUtils::readFile(filePath);
    if (source.empty()) {
        spdlog::error("[OpenGLShaderCompiler] Failed to read shader file: {}", filePath);
        return nullptr;
    }

    auto shaderSources = preprocessShaderSource(source);
    if (shaderSources.empty()) {
        spdlog::error("[OpenGLShaderCompiler] No shader stages found in: {}", filePath);
        return nullptr;
    }

    return compileFromSources(filePath, shaderSources);
}

// ============================================================================
// 从已拆分的源码编译
// ============================================================================

std::unique_ptr<RHIShaderProgram> OpenGLShaderCompiler::compileFromSources(
        const std::string& name,
        const std::unordered_map<ShaderStage, std::string>& shaderSources) {

    // 构造 ProgramDesc 传给 OpenGLDevice::createProgram
    ProgramDesc desc;
    desc.name = name;

    for (auto& [stage, source] : shaderSources) {
        if (source.empty()) continue;

        ShaderSource ss;
        ss.stage = stage;
        // 将 GLSL 文本转为 vector<uint8_t>
        ss.code.assign(source.begin(), source.end());
        ss.entryPoint = "main";
        desc.shaders.push_back(std::move(ss));
    }

    if (desc.shaders.empty()) {
        spdlog::error("[OpenGLShaderCompiler] No valid shader sources for: {}", name);
        return nullptr;
    }

    ProgramHandle handle = mDevice.createProgram(desc);
    if (!static_cast<bool>(handle)) {
        spdlog::error("[OpenGLShaderCompiler] Failed to compile/link program: {}", name);
        return nullptr;
    }

    spdlog::info("[OpenGLShaderCompiler] Program '{}' compiled successfully (handle={})",
                 name, handle.getId());

    // 获取 GL program ID 并创建 OpenGLShaderProgram
    GLuint glProgramId = mDevice.getGLProgramId(handle);
    return std::make_unique<OpenGLShaderProgram>(handle, glProgramId, mDevice);
}

} // namespace rhi
} // namespace engine
