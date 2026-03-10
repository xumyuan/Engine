#include "ShaderCompilerService.h"
#include "OpenGLDevice.h"
#include "utils/FileUtils.h"

#include <GL/glew.h>
#include <spdlog/spdlog.h>

namespace engine {
namespace rhi {

ShaderCompilerService::ShaderCompilerService(OpenGLDevice& device)
    : mDevice(device) {
}

// ============================================================================
// 字符串 → ShaderStage
// ============================================================================

ShaderStage ShaderCompilerService::shaderStageFromString(const std::string& type) {
    if (type == "vertex")   return ShaderStage::Vertex;
    if (type == "fragment")  return ShaderStage::Fragment;
    if (type == "geometry")  return ShaderStage::Geometry;
    if (type == "hull")      return ShaderStage::TessControl;
    if (type == "domain")    return ShaderStage::TessEvaluation;
    if (type == "compute")   return ShaderStage::Compute;

    spdlog::error("[ShaderCompiler] Unknown shader type: '{}'", type);
    return ShaderStage::Vertex; // fallback
}

// ============================================================================
// 预处理 #shader-type 标记
// ============================================================================

std::unordered_map<ShaderStage, std::string>
ShaderCompilerService::preprocessShaderSource(const std::string& source) {
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

ProgramHandle ShaderCompilerService::loadAndCompile(const std::string& filePath) {
    std::string source = FileUtils::readFile(filePath);
    if (source.empty()) {
        spdlog::error("[ShaderCompiler] Failed to read shader file: {}", filePath);
        return ProgramHandle();
    }

    auto shaderSources = preprocessShaderSource(source);
    if (shaderSources.empty()) {
        spdlog::error("[ShaderCompiler] No shader stages found in: {}", filePath);
        return ProgramHandle();
    }

    return compileFromSources(filePath, shaderSources);
}

// ============================================================================
// 从已拆分的源码编译
// ============================================================================

ProgramHandle ShaderCompilerService::compileFromSources(
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
        spdlog::error("[ShaderCompiler] No valid shader sources for: {}", name);
        return ProgramHandle();
    }

    ProgramHandle handle = mDevice.createProgram(desc);
    if (!static_cast<bool>(handle)) {
        spdlog::error("[ShaderCompiler] Failed to compile/link program: {}", name);
    } else {
        spdlog::info("[ShaderCompiler] Program '{}' compiled successfully (handle={})",
                     name, handle.getId());
    }

    return handle;
}

} // namespace rhi
} // namespace engine
