#pragma once

#include "RHIHandle.h"
#include <glm/glm.hpp>
#include <string>

namespace engine {
namespace rhi {

// RHI 着色器程序抽象接口
// 封装一个已编译链接的着色器程序，提供：
//   - 激活/停用
//   - Uniform 设置
//   - Program handle 查询
//
// 各后端（OpenGL/Vulkan/...）提供具体实现
class RHIShaderProgram {
public:
    virtual ~RHIShaderProgram() = default;

    // ---------- Program 激活 ----------
    virtual void use() = 0;
    virtual void unuse() = 0;

    // ---------- Uniform 设置（调用前需确保 use() 已调用） ----------
    virtual void setUniform(const char* name, float value) = 0;
    virtual void setUniform(const char* name, int value) = 0;
    virtual void setUniform(const char* name, const glm::vec2& value) = 0;
    virtual void setUniform(const char* name, const glm::vec3& value) = 0;
    virtual void setUniform(const char* name, const glm::vec4& value) = 0;
    virtual void setUniform(const char* name, const glm::ivec4& value) = 0;
    virtual void setUniform(const char* name, const glm::mat3& value) = 0;
    virtual void setUniform(const char* name, const glm::mat4& value) = 0;

    // std::string 重载（转发到 const char* 版本）
    void setUniform(const std::string& name, float value)            { setUniform(name.c_str(), value); }
    void setUniform(const std::string& name, int value)              { setUniform(name.c_str(), value); }
    void setUniform(const std::string& name, const glm::vec2& value) { setUniform(name.c_str(), value); }
    void setUniform(const std::string& name, const glm::vec3& value) { setUniform(name.c_str(), value); }
    void setUniform(const std::string& name, const glm::vec4& value) { setUniform(name.c_str(), value); }
    void setUniform(const std::string& name, const glm::ivec4& value){ setUniform(name.c_str(), value); }
    void setUniform(const std::string& name, const glm::mat3& value) { setUniform(name.c_str(), value); }
    void setUniform(const std::string& name, const glm::mat4& value) { setUniform(name.c_str(), value); }

    // ---------- 查询 ----------
    virtual ProgramHandle getProgramHandle() const = 0;
};

} // namespace rhi
} // namespace engine
