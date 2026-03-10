#pragma once

#include "rhi/include/RHIHandle.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace engine {
namespace rhi {

// OpenGL Program 操作封装
// 提供 uniform 设置、program 激活等功能，与 RHIDevice 解耦
// 短期保留 setUniform 兼容接口，中期逐步迁移为 UBO
class OpenGLProgram {
public:
    // 通过 ProgramHandle 和 OpenGLDevice 获取 GL program ID 来构造
    explicit OpenGLProgram(GLuint glProgramId);
    ~OpenGLProgram() = default;

    // 不可复制，可移动
    OpenGLProgram(const OpenGLProgram&) = delete;
    OpenGLProgram& operator=(const OpenGLProgram&) = delete;
    OpenGLProgram(OpenGLProgram&& other) noexcept;
    OpenGLProgram& operator=(OpenGLProgram&& other) noexcept;

    // ---------- Program 激活 ----------
    void use() const;
    static void unuse();

    // ---------- Uniform 设置（过渡期兼容接口） ----------
    // 注意：调用前需确保该 program 已被 use()
    void setUniform(const char* name, float value);
    void setUniform(const char* name, int value);
    void setUniform(const char* name, const glm::vec2& value);
    void setUniform(const char* name, const glm::vec3& value);
    void setUniform(const char* name, const glm::vec4& value);
    void setUniform(const char* name, const glm::ivec4& value);
    void setUniform(const char* name, const glm::mat3& value);
    void setUniform(const char* name, const glm::mat4& value);

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
    GLuint getGLProgramId() const { return mGLProgramId; }
    GLint getUniformLocation(const char* name);

    // 清空 uniform location 缓存（program 重新链接后需要调用）
    void invalidateUniformCache();

private:
    GLuint mGLProgramId = 0;

    // uniform location 缓存，避免每次都调用 glGetUniformLocation
    std::unordered_map<std::string, GLint> mUniformLocationCache;
};

} // namespace rhi
} // namespace engine
