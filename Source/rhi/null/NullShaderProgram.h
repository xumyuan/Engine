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

    void setUniform(const char*, float) override {}
    void setUniform(const char*, int) override {}
    void setUniform(const char*, const glm::vec2&) override {}
    void setUniform(const char*, const glm::vec3&) override {}
    void setUniform(const char*, const glm::vec4&) override {}
    void setUniform(const char*, const glm::ivec4&) override {}
    void setUniform(const char*, const glm::mat3&) override {}
    void setUniform(const char*, const glm::mat4&) override {}

    ProgramHandle getProgramHandle() const override { return mHandle; }

private:
    ProgramHandle mHandle;
};

} // namespace rhi
} // namespace engine
