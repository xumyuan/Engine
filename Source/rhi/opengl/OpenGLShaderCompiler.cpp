#include "OpenGLShaderCompiler.h"
#include "OpenGLDevice.h"
#include "OpenGLShaderProgram.h"
#include "utils/FileUtils.h"

#include <filesystem>
#include <unordered_set>
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
// 递归展开 #include "..."
// ============================================================================

namespace {

constexpr int kMaxIncludeDepth = 32;

// 扫描单行：更新多行注释状态，并判断该行是否为有效的 #include "path" 指令
bool tryParseIncludeLine(std::string_view line, bool& inBlockComment, std::string& outIncludePath) {
    size_t i = 0;
    const size_t n = line.size();

    // 跳过前导空白与注释，定位首个有效字符
    while (i < n) {
        if (inBlockComment) {
            size_t endComment = line.find("*/", i);
            if (endComment == std::string_view::npos) {
                return false;
            }
            inBlockComment = false;
            i = endComment + 2;
            continue;
        }
        if (line[i] == ' ' || line[i] == '\t' || line[i] == '\r') {
            ++i;
            continue;
        }
        if (i + 1 < n && line[i] == '/' && line[i + 1] == '/') {
            return false; // 整行剩余部分为单行注释
        }
        if (i + 1 < n && line[i] == '/' && line[i + 1] == '*') {
            inBlockComment = true;
            i += 2;
            continue;
        }
        break;
    }

    // 记录首个有效字符位置，同时把本行剩余部分的块注释状态更新完
    size_t firstTokenPos = i;
    while (i < n) {
        if (inBlockComment) {
            size_t endComment = line.find("*/", i);
            if (endComment == std::string_view::npos) break;
            inBlockComment = false;
            i = endComment + 2;
        } else if (i + 1 < n && line[i] == '/' && line[i + 1] == '/') {
            break;
        } else if (i + 1 < n && line[i] == '/' && line[i + 1] == '*') {
            inBlockComment = true;
            i += 2;
        } else {
            ++i;
        }
    }

    if (firstTokenPos >= n || line[firstTokenPos] != '#') {
        return false;
    }

    size_t pos = firstTokenPos + 1;
    while (pos < n && (line[pos] == ' ' || line[pos] == '\t')) {
        ++pos;
    }

    constexpr std::string_view kIncludeToken = "include";
    if (pos + kIncludeToken.size() > n ||
        line.substr(pos, kIncludeToken.size()) != kIncludeToken) {
        return false;
    }
    pos += kIncludeToken.size();

    while (pos < n && (line[pos] == ' ' || line[pos] == '\t')) {
        ++pos;
    }

    if (pos >= n || line[pos] != '"') {
        return false;
    }
    size_t openQuote = pos;
    size_t closeQuote = line.find('"', openQuote + 1);
    if (closeQuote == std::string_view::npos || closeQuote <= openQuote + 1) {
        return false;
    }

    outIncludePath.assign(line.substr(openQuote + 1, closeQuote - openQuote - 1));
    return true;
}

std::string resolveIncludesImpl(const std::string& source,
                                const std::filesystem::path& currentDir,
                                std::unordered_set<std::string>& includedFiles,
                                int depth) {
    if (depth > kMaxIncludeDepth) {
        spdlog::error("[OpenGLShaderCompiler] #include recursion depth exceeded {} in '{}'",
                      kMaxIncludeDepth, currentDir.string());
        return {};
    }

    // 若源码中根本没有 #include，直接返回，避免无谓拷贝
    if (source.find("#include") == std::string::npos) {
        return source;
    }

    std::string output;
    output.reserve(source.size() + 512);

    bool inBlockComment = false;
    size_t lineStart = 0;
    size_t lineNum = 1;

    while (lineStart < source.size()) {
        size_t lineEnd = source.find('\n', lineStart);
        bool hasNewline = (lineEnd != std::string::npos);
        size_t count = hasNewline ? (lineEnd - lineStart) : (source.size() - lineStart);
        std::string_view lineView(source.data() + lineStart, count);

        std::string includeRelPath;
        if (tryParseIncludeLine(lineView, inBlockComment, includeRelPath)) {
            std::filesystem::path resolved = currentDir.empty()
                ? std::filesystem::path(includeRelPath)
                : (currentDir / includeRelPath);
            resolved = resolved.lexically_normal();

            if (!std::filesystem::exists(resolved)) {
                std::filesystem::path fallback =
                    (std::filesystem::path("Source/shaders") / includeRelPath).lexically_normal();
                if (std::filesystem::exists(fallback)) {
                    resolved = std::move(fallback);
                }
            }

            std::string canonicalKey = resolved.generic_string();
            if (!includedFiles.contains(canonicalKey)) {
                if (!std::filesystem::exists(resolved)) {
                    spdlog::error("[OpenGLShaderCompiler] Failed to resolve #include \"{}\" (from '{}')",
                                  includeRelPath, currentDir.string());
                } else {
                    includedFiles.insert(canonicalKey);
                    std::string includedContent = FileUtils::readFile(resolved.string());
                    std::string expanded = resolveIncludesImpl(
                        includedContent, resolved.parent_path(), includedFiles, depth + 1);

                    output += "#line 1\n";
                    output += expanded;
                    if (!expanded.empty() && expanded.back() != '\n') {
                        output += '\n';
                    }
                    output += "#line " + std::to_string(lineNum + 1) + "\n";
                }
            } else {
                // 同一 stage 已包含过该文件，输出空行保持行号不变
                if (hasNewline) output += '\n';
            }
        } else {
            output.append(lineView.data(), lineView.size());
            if (hasNewline) output += '\n';
        }

        if (!hasNewline) break;
        lineStart = lineEnd + 1;
        ++lineNum;
    }

    return output;
}

} // namespace

std::string OpenGLShaderCompiler::resolveIncludes(const std::string& source,
                                                  const std::string& filePath) {
    std::filesystem::path currentDir = filePath.empty()
        ? std::filesystem::path("Source/shaders")
        : std::filesystem::path(filePath).parent_path();
    std::unordered_set<std::string> includedFiles;
    return resolveIncludesImpl(source, currentDir, includedFiles, 0);
}

// ============================================================================
// 预处理 #shader-type 标记
// ============================================================================

std::unordered_map<ShaderStage, std::string>
OpenGLShaderCompiler::preprocessShaderSource(const std::string& source,
                                             const std::string& filePath) {
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
        shaderSources[stage] = resolveIncludes(code, filePath);
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

    auto shaderSources = preprocessShaderSource(source, filePath);
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

    // 编译结果在 program 首次使用或 RHIDevice::waitProgram 时确定并输出日志
    ProgramHandle handle = mDevice.createProgram(desc);
    if (!static_cast<bool>(handle)) {
        spdlog::error("[OpenGLShaderCompiler] Failed to create program: {}", name);
        return nullptr;
    }

    // 获取 GL program ID 并创建 OpenGLShaderProgram
    GLuint glProgramId = mDevice.getGLProgramId(handle);
    return std::make_unique<OpenGLShaderProgram>(handle, glProgramId, mDevice);
}

} // namespace rhi
} // namespace engine
