#pragma once

#include "rhi/include/RHIShaderProgram.h"

namespace engine {
namespace rhi {

// Null 后端的着色器程序实现：所有操作空实现，用于测试
class NullShaderProgram final : public RHIShaderProgram {
public:
    explicit NullShaderProgram(ProgramHandle handle) : mHandle(handle) {}
    ~NullShaderProgram() override = default;

    void use() override {}
    void unuse() override {}


    ProgramHandle getProgramHandle() const override { return mHandle; }

private:
    ProgramHandle mHandle;
};

} // namespace rhi
} // namespace engine
