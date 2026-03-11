#pragma once

#include "rhi/include/RHIShaderCompiler.h"
#include "NullShaderProgram.h"

namespace engine {
namespace rhi {

// Null 后端的着色器编译器实现：直接返回 NullShaderProgram，不做任何编译
class NullShaderCompiler final : public RHIShaderCompiler {
public:
    ~NullShaderCompiler() override = default;

    std::unique_ptr<RHIShaderProgram> loadAndCompile(const std::string&) override {
        return std::make_unique<NullShaderProgram>(ProgramHandle(mNextHandle++));
    }

    std::unique_ptr<RHIShaderProgram> compileFromSources(
            const std::string&,
            const std::unordered_map<ShaderStage, std::string>&) override {
        return std::make_unique<NullShaderProgram>(ProgramHandle(mNextHandle++));
    }

private:
    uint32_t mNextHandle = 1000; // 从高位开始避免与 NullDevice 句柄冲突
};

} // namespace rhi
} // namespace engine
