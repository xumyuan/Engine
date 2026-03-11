#pragma once

#include "rhi/include/RHIShaderProgram.h"
#include <GL/glew.h>
#include <unordered_map>

namespace engine {
namespace rhi {

class RHIDevice;

// OpenGL 后端的着色器程序实现
// 封装 GL program 对象，提供 uniform 设置和 program 激活
// 析构时自动通过 RHIDevice 销毁底层 GL program
class OpenGLShaderProgram final : public RHIShaderProgram {
public:
    // programHandle: RHI 句柄（供上层查询）
    // glProgramId:   GL program 对象 ID
    // device:        创建此 program 的 RHIDevice（析构时用于资源回收）
    OpenGLShaderProgram(ProgramHandle programHandle, GLuint glProgramId, RHIDevice& device);
    ~OpenGLShaderProgram() override;

    // 不可复制，可移动
    OpenGLShaderProgram(const OpenGLShaderProgram&) = delete;
    OpenGLShaderProgram& operator=(const OpenGLShaderProgram&) = delete;
    OpenGLShaderProgram(OpenGLShaderProgram&& other) noexcept;
    OpenGLShaderProgram& operator=(OpenGLShaderProgram&& other) noexcept;

    // ---------- RHIShaderProgram 接口 ----------
    void use() override;
    void unuse() override;

    void setUniform(const char* name, float value) override;
    void setUniform(const char* name, int value) override;
    void setUniform(const char* name, const glm::vec2& value) override;
    void setUniform(const char* name, const glm::vec3& value) override;
    void setUniform(const char* name, const glm::vec4& value) override;
    void setUniform(const char* name, const glm::ivec4& value) override;
    void setUniform(const char* name, const glm::mat3& value) override;
    void setUniform(const char* name, const glm::mat4& value) override;

    ProgramHandle getProgramHandle() const override { return mProgramHandle; }

    // ---------- OpenGL 特有查询 ----------
    GLuint getGLProgramId() const { return mGLProgramId; }

    // 清空 uniform location 缓存（program 重新链接后需要调用）
    void invalidateUniformCache();

private:
    GLint getUniformLocation(const char* name);

    ProgramHandle mProgramHandle;
    GLuint mGLProgramId = 0;
    RHIDevice* mDevice = nullptr;

    // uniform location 缓存，避免每次都调用 glGetUniformLocation
    std::unordered_map<std::string, GLint> mUniformLocationCache;
};

} // namespace rhi
} // namespace engine
