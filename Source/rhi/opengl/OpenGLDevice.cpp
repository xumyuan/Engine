#include "OpenGLDevice.h"
#include <GL/glew.h>
#include <cassert>
#include <spdlog/spdlog.h>

namespace engine {
namespace rhi {

// ============================================================================
// 内部工具
// ============================================================================

static GLenum toGLInternalFormat(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::R8:              return GL_R8;
        case TextureFormat::RG8:             return GL_RG8;
        case TextureFormat::RGB8:            return GL_RGB8;
        case TextureFormat::RGBA8:           return GL_RGBA8;
        case TextureFormat::SRGB8_A8:        return GL_SRGB8_ALPHA8;
        case TextureFormat::R16F:            return GL_R16F;
        case TextureFormat::RG16F:           return GL_RG16F;
        case TextureFormat::RGBA16F:         return GL_RGBA16F;
        case TextureFormat::R32F:            return GL_R32F;
        case TextureFormat::RG32F:           return GL_RG32F;
        case TextureFormat::RGBA32F:         return GL_RGBA32F;
        case TextureFormat::Depth16:         return GL_DEPTH_COMPONENT16;
        case TextureFormat::Depth24:         return GL_DEPTH_COMPONENT24;
        case TextureFormat::Depth32F:        return GL_DEPTH_COMPONENT32F;
        case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
        case TextureFormat::Depth32FStencil8:return GL_DEPTH32F_STENCIL8;
        default:                             return GL_RGBA8;
    }
}

static GLenum toGLFormat(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::R8:
        case TextureFormat::R16F:
        case TextureFormat::R32F:            return GL_RED;
        case TextureFormat::RG8:
        case TextureFormat::RG16F:
        case TextureFormat::RG32F:           return GL_RG;
        case TextureFormat::RGB8:            return GL_RGB;
        case TextureFormat::RGBA8:
        case TextureFormat::SRGB8_A8:
        case TextureFormat::RGBA16F:
        case TextureFormat::RGBA32F:         return GL_RGBA;
        case TextureFormat::Depth16:
        case TextureFormat::Depth24:
        case TextureFormat::Depth32F:        return GL_DEPTH_COMPONENT;
        case TextureFormat::Depth24Stencil8:
        case TextureFormat::Depth32FStencil8:return GL_DEPTH_STENCIL;
        default:                             return GL_RGBA;
    }
}

static GLenum toGLPixelType(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::R16F:
        case TextureFormat::RG16F:
        case TextureFormat::RGBA16F:         return GL_HALF_FLOAT;
        case TextureFormat::R32F:
        case TextureFormat::RG32F:
        case TextureFormat::RGBA32F:
        case TextureFormat::Depth32F:        return GL_FLOAT;
        case TextureFormat::Depth24Stencil8: return GL_UNSIGNED_INT_24_8;
        case TextureFormat::Depth32FStencil8:return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        default:                             return GL_UNSIGNED_BYTE;
    }
}

static GLenum toGLTextureTarget(TextureType type, uint8_t samples) {
    if (samples > 1 && type == TextureType::Texture2D)
        return GL_TEXTURE_2D_MULTISAMPLE;
    switch (type) {
        case TextureType::Texture2D:       return GL_TEXTURE_2D;
        case TextureType::Texture2DArray:  return GL_TEXTURE_2D_ARRAY;
        case TextureType::Texture3D:       return GL_TEXTURE_3D;
        case TextureType::TextureCube:     return GL_TEXTURE_CUBE_MAP;
        case TextureType::TextureCubeArray:return GL_TEXTURE_CUBE_MAP_ARRAY;
        default:                           return GL_TEXTURE_2D;
    }
}

static GLenum toGLBufferTarget(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex:  return GL_ARRAY_BUFFER;
        case BufferUsage::Index:   return GL_ELEMENT_ARRAY_BUFFER;
        case BufferUsage::Uniform: return GL_UNIFORM_BUFFER;
        case BufferUsage::Storage: return GL_SHADER_STORAGE_BUFFER;
        case BufferUsage::Staging: return GL_COPY_READ_BUFFER;
        default:                   return GL_ARRAY_BUFFER;
    }
}

static GLenum toGLCullFace(CullMode mode) {
    switch (mode) {
        case CullMode::Front: return GL_FRONT;
        case CullMode::Back:  return GL_BACK;
        default:              return GL_BACK;
    }
}

static GLenum toGLCompareFunc(CompareOp op) {
    switch (op) {
        case CompareOp::Never:        return GL_NEVER;
        case CompareOp::Less:         return GL_LESS;
        case CompareOp::Equal:        return GL_EQUAL;
        case CompareOp::LessEqual:    return GL_LEQUAL;
        case CompareOp::Greater:      return GL_GREATER;
        case CompareOp::NotEqual:     return GL_NOTEQUAL;
        case CompareOp::GreaterEqual: return GL_GEQUAL;
        case CompareOp::Always:       return GL_ALWAYS;
        default:                      return GL_LESS;
    }
}

static GLenum toGLBlendFactor(BlendFactor f) {
    switch (f) {
        case BlendFactor::Zero:             return GL_ZERO;
        case BlendFactor::One:              return GL_ONE;
        case BlendFactor::SrcColor:         return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::SrcAlpha:         return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstColor:         return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::DstAlpha:         return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
        default:                            return GL_ONE;
    }
}

static GLenum toGLPrimitive(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Triangles:     return GL_TRIANGLES;
        case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveType::Lines:         return GL_LINES;
        case PrimitiveType::LineStrip:     return GL_LINE_STRIP;
        case PrimitiveType::Points:        return GL_POINTS;
        default:                           return GL_TRIANGLES;
    }
}

static GLenum toGLIndexType(IndexType type) {
    return type == IndexType::UInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

static GLenum toGLAttribType(VertexAttribType type) {
    switch (type) {
        case VertexAttribType::Float:
        case VertexAttribType::Float2:
        case VertexAttribType::Float3:
        case VertexAttribType::Float4:     return GL_FLOAT;
        case VertexAttribType::Int:
        case VertexAttribType::Int2:
        case VertexAttribType::Int3:
        case VertexAttribType::Int4:       return GL_INT;
        case VertexAttribType::UByte4Norm: return GL_UNSIGNED_BYTE;
        default:                           return GL_FLOAT;
    }
}

static GLint attribComponentCount(VertexAttribType type) {
    switch (type) {
        case VertexAttribType::Float:  return 1;
        case VertexAttribType::Float2: return 2;
        case VertexAttribType::Float3: return 3;
        case VertexAttribType::Float4: return 4;
        case VertexAttribType::Int:    return 1;
        case VertexAttribType::Int2:   return 2;
        case VertexAttribType::Int3:   return 3;
        case VertexAttribType::Int4:   return 4;
        case VertexAttribType::UByte4Norm: return 4;
        default: return 1;
    }
}

static bool isIntegerAttrib(VertexAttribType type) {
    switch (type) {
        case VertexAttribType::Int:
        case VertexAttribType::Int2:
        case VertexAttribType::Int3:
        case VertexAttribType::Int4: return true;
        default: return false;
    }
}

// ============================================================================
// 生命周期
// ============================================================================

bool OpenGLDevice::initialize() {
    // 假设 GLEW 已由外部（Window）初始化完成
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    mMaxTextureSize = static_cast<uint32_t>(maxSize);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    spdlog::info("[RHI-OpenGL] initialized, max texture size: {}", mMaxTextureSize);
    return true;
}

void OpenGLDevice::terminate() {
    // 销毁所有残留资源
    for (auto& [id, data] : mTextures)       { glDeleteTextures(1, &data.glId); }
    for (auto& [id, data] : mBuffers)        { glDeleteBuffers(1, &data.glId); }
    for (auto& [id, data] : mPrograms)       { glDeleteProgram(data.glId); }
    for (auto& [id, data] : mRenderTargets)  { glDeleteFramebuffers(1, &data.fboId); }
    for (auto& [id, data] : mRenderPrimitives) { glDeleteVertexArrays(1, &data.vaoId); }

    mTextures.clear();
    mBuffers.clear();
    mPrograms.clear();
    mRenderTargets.clear();
    mRenderPrimitives.clear();
    mSwapChains.clear();

    spdlog::info("[RHI-OpenGL] terminated");
}

HandleBase::HandleId OpenGLDevice::allocHandle() {
    return mNextHandle++;
}

// ============================================================================
// 能力查询
// ============================================================================

bool OpenGLDevice::isTextureFormatSupported(TextureFormat /*format*/) const noexcept {
    // TODO: 通过 glGetInternalformativ 做精确查询
    return true;
}

uint32_t OpenGLDevice::getMaxTextureSize() const noexcept {
    return mMaxTextureSize;
}

// ============================================================================
// 资源创建
// ============================================================================

TextureHandle OpenGLDevice::createTexture(const TextureDesc& desc) {
    GLuint tex = 0;
    glGenTextures(1, &tex);

    GLenum target = toGLTextureTarget(desc.type, desc.samples);
    glBindTexture(target, tex);

    GLenum internalFmt = toGLInternalFormat(desc.format);
    GLenum fmt         = toGLFormat(desc.format);
    GLenum pixelType   = toGLPixelType(desc.format);

    if (desc.samples > 1) {
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                desc.samples, internalFmt, desc.width, desc.height, GL_TRUE);
    } else if (desc.type == TextureType::Texture2D) {
        for (uint8_t level = 0; level < desc.levels; ++level) {
            uint32_t w = std::max(1u, desc.width >> level);
            uint32_t h = std::max(1u, desc.height >> level);
            glTexImage2D(GL_TEXTURE_2D, level, internalFmt, w, h, 0, fmt, pixelType, nullptr);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                desc.levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else if (desc.type == TextureType::TextureCube) {
        for (int face = 0; face < 6; ++face) {
            for (uint8_t level = 0; level < desc.levels; ++level) {
                uint32_t w = std::max(1u, desc.width >> level);
                uint32_t h = std::max(1u, desc.height >> level);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                        level, internalFmt, w, h, 0, fmt, pixelType, nullptr);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                desc.levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    // TODO: Texture2DArray, Texture3D

    glBindTexture(target, 0);

    auto hid = allocHandle();
    mTextures[hid] = { tex, desc };
    return TextureHandle(hid);
}

void OpenGLDevice::generateMipmaps(const TextureHandle handle) {
    auto it = mTextures.find(handle.getId());
    if (it == mTextures.end()) return;

    const auto& data = it->second;
    GLenum target = (data.desc.type == TextureType::TextureCube)
                    ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

    glBindTexture(target, data.glId);
    glGenerateMipmap(target);
    glBindTexture(target, 0);
}

BufferHandle OpenGLDevice::createBuffer(const BufferDesc& desc) {
    GLuint buf = 0;
    glGenBuffers(1, &buf);

    GLenum target = toGLBufferTarget(desc.usage);
    GLenum glUsage = desc.dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glBindBuffer(target, buf);
    if (desc.size > 0) {
        glBufferData(target, desc.size, nullptr, glUsage);
    }
    glBindBuffer(target, 0);

    auto hid = allocHandle();
    mBuffers[hid] = { buf, target, desc };
    return BufferHandle(hid);
}

ProgramHandle OpenGLDevice::createProgram(const ProgramDesc& desc) {
    GLuint program = glCreateProgram();

    std::vector<GLuint> shaders;
    for (auto& src : desc.shaders) {
        GLenum type = GL_VERTEX_SHADER;
        switch (src.stage) {
            case ShaderStage::Vertex:   type = GL_VERTEX_SHADER; break;
            case ShaderStage::Fragment: type = GL_FRAGMENT_SHADER; break;
            case ShaderStage::Compute:  type = GL_COMPUTE_SHADER; break;
        }

        GLuint shader = glCreateShader(type);
        // 假设 code 存的是 GLSL 源码文本
        const char* code = reinterpret_cast<const char*>(src.code.data());
        GLint length = static_cast<GLint>(src.code.size());
        glShaderSource(shader, 1, &code, &length);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[1024];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            spdlog::error("[RHI-OpenGL] Shader compile error ({}): {}", desc.name, log);
            glDeleteShader(shader);
            glDeleteProgram(program);
            return ProgramHandle(); // 返回无效句柄
        }

        glAttachShader(program, shader);
        shaders.push_back(shader);
    }

    glLinkProgram(program);
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        spdlog::error("[RHI-OpenGL] Program link error ({}): {}", desc.name, log);
    }

    // 清理 shader 对象
    for (auto s : shaders) {
        glDeleteShader(s);
    }

    if (!success) {
        glDeleteProgram(program);
        return ProgramHandle();
    }

    auto hid = allocHandle();
    mPrograms[hid] = { program };
    return ProgramHandle(hid);
}

RenderTargetHandle OpenGLDevice::createRenderTarget(const RenderTargetDesc& desc) {
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // 挂载颜色附件
    std::vector<GLenum> drawBuffers;
    for (uint8_t i = 0; i < desc.colorCount; ++i) {
        auto it = mTextures.find(desc.colorAttachments[i].getId());
        if (it != mTextures.end()) {
            GLenum attachment = GL_COLOR_ATTACHMENT0 + i;
            GLenum target = toGLTextureTarget(it->second.desc.type, it->second.desc.samples);
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, target,
                    it->second.glId, desc.colorLevels[i]);
            drawBuffers.push_back(attachment);
        }
    }

    if (!drawBuffers.empty()) {
        glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
    }

    // 挂载深度附件
    if (static_cast<bool>(desc.depthAttachment)) {
        auto it = mTextures.find(desc.depthAttachment.getId());
        if (it != mTextures.end()) {
            GLenum target = toGLTextureTarget(it->second.desc.type, it->second.desc.samples);
            GLenum attachPoint = GL_DEPTH_ATTACHMENT;
            // 如果格式包含 stencil
            if (it->second.desc.format == TextureFormat::Depth24Stencil8 ||
                it->second.desc.format == TextureFormat::Depth32FStencil8) {
                attachPoint = GL_DEPTH_STENCIL_ATTACHMENT;
            }
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachPoint, target,
                    it->second.glId, desc.depthLevel);
        }
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("[RHI-OpenGL] Framebuffer incomplete: 0x{:X}", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto hid = allocHandle();
    mRenderTargets[hid] = { fbo, desc };
    return RenderTargetHandle(hid);
}

RenderPrimitiveHandle OpenGLDevice::createRenderPrimitive(
        const VertexLayout& layout,
        BufferHandle vertexBuffers[],
        uint8_t vertexBufferCount,
        BufferHandle indexBuffer,
        IndexType indexType) {

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // 绑定顶点属性
    for (uint8_t i = 0; i < layout.attributeCount; ++i) {
        auto& attr = layout.attributes[i];
        if (attr.bufferIndex >= vertexBufferCount) continue;

        auto it = mBuffers.find(vertexBuffers[attr.bufferIndex].getId());
        if (it == mBuffers.end()) continue;

        glBindBuffer(GL_ARRAY_BUFFER, it->second.glId);
        glEnableVertexAttribArray(i);

        GLint count = attribComponentCount(attr.type);
        GLenum type = toGLAttribType(attr.type);
        GLsizei stride = layout.strides[attr.bufferIndex];

        if (isIntegerAttrib(attr.type)) {
            glVertexAttribIPointer(i, count, type, stride,
                    reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.offset)));
        } else {
            GLboolean normalized = (attr.type == VertexAttribType::UByte4Norm) ? GL_TRUE : GL_FALSE;
            glVertexAttribPointer(i, count, type, normalized, stride,
                    reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.offset)));
        }
    }

    // 绑定索引缓冲
    if (static_cast<bool>(indexBuffer)) {
        auto it = mBuffers.find(indexBuffer.getId());
        if (it != mBuffers.end()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.glId);
        }
    }

    glBindVertexArray(0);

    auto hid = allocHandle();
    mRenderPrimitives[hid] = { vao, indexBuffer, indexType };
    return RenderPrimitiveHandle(hid);
}

SwapChainHandle OpenGLDevice::createSwapChain(void* nativeWindow,
        uint32_t width, uint32_t height) {
    auto hid = allocHandle();
    mSwapChains[hid] = { nativeWindow, width, height };
    return SwapChainHandle(hid);
}

// ============================================================================
// 资源销毁
// ============================================================================

void OpenGLDevice::destroyTexture(TextureHandle handle) {
    auto it = mTextures.find(handle.getId());
    if (it != mTextures.end()) {
        glDeleteTextures(1, &it->second.glId);
        mTextures.erase(it);
    }
}

void OpenGLDevice::destroyBuffer(BufferHandle handle) {
    auto it = mBuffers.find(handle.getId());
    if (it != mBuffers.end()) {
        glDeleteBuffers(1, &it->second.glId);
        mBuffers.erase(it);
    }
}

void OpenGLDevice::destroyProgram(ProgramHandle handle) {
    auto it = mPrograms.find(handle.getId());
    if (it != mPrograms.end()) {
        glDeleteProgram(it->second.glId);
        mPrograms.erase(it);
    }
}

void OpenGLDevice::destroyRenderTarget(RenderTargetHandle handle) {
    auto it = mRenderTargets.find(handle.getId());
    if (it != mRenderTargets.end()) {
        glDeleteFramebuffers(1, &it->second.fboId);
        mRenderTargets.erase(it);
    }
}

void OpenGLDevice::destroyRenderPrimitive(RenderPrimitiveHandle handle) {
    auto it = mRenderPrimitives.find(handle.getId());
    if (it != mRenderPrimitives.end()) {
        glDeleteVertexArrays(1, &it->second.vaoId);
        mRenderPrimitives.erase(it);
    }
}

void OpenGLDevice::destroySwapChain(SwapChainHandle handle) {
    mSwapChains.erase(handle.getId());
}

// ============================================================================
// 资源更新
// ============================================================================

void OpenGLDevice::updateBuffer(BufferHandle handle, const BufferDataDesc& data) {
    auto it = mBuffers.find(handle.getId());
    if (it == mBuffers.end()) return;

    glBindBuffer(it->second.glTarget, it->second.glId);
    glBufferSubData(it->second.glTarget, data.offset, data.size, data.data);
    glBindBuffer(it->second.glTarget, 0);
}

void OpenGLDevice::updateTexture(TextureHandle handle, uint32_t level,
        uint32_t xoffset, uint32_t yoffset,
        uint32_t width, uint32_t height,
        const void* data, uint32_t /*dataSize*/) {
    auto it = mTextures.find(handle.getId());
    if (it == mTextures.end()) return;

    GLenum target = toGLTextureTarget(it->second.desc.type, it->second.desc.samples);
    GLenum fmt = toGLFormat(it->second.desc.format);
    GLenum pixelType = toGLPixelType(it->second.desc.format);

    glBindTexture(target, it->second.glId);
    glTexSubImage2D(target, level, xoffset, yoffset, width, height, fmt, pixelType, data);
    glBindTexture(target, 0);
}

// ============================================================================
// 渲染命令
// ============================================================================

void OpenGLDevice::beginFrame(SwapChainHandle swapChain) {
    mCurrentSwapChain = swapChain;
}

void OpenGLDevice::endFrame() {
    // SwapBuffers 由外部 Window/GLFW 管理
    mCurrentSwapChain.clear();
}

void OpenGLDevice::beginRenderPass(RenderTargetHandle target,
        const RenderPassParams& params) {
    mCurrentRenderTarget = target;

    if (static_cast<bool>(target)) {
        auto it = mRenderTargets.find(target.getId());
        if (it != mRenderTargets.end()) {
            glBindFramebuffer(GL_FRAMEBUFFER, it->second.fboId);
        }
    } else {
        // 空 handle = 默认帧缓冲（屏幕）
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if (params.viewport.w > 0 && params.viewport.h > 0) {
        glViewport(params.viewport.x, params.viewport.y,
                   params.viewport.w, params.viewport.h);
    }

    GLbitfield clearBits = 0;
    if (params.clearColorFlag) {
        glClearColor(params.clearColor[0], params.clearColor[1],
                     params.clearColor[2], params.clearColor[3]);
        clearBits |= GL_COLOR_BUFFER_BIT;
    }
    if (params.clearDepthFlag) {
        glClearDepth(params.clearDepth);
        clearBits |= GL_DEPTH_BUFFER_BIT;
    }
    if (clearBits) {
        glClear(clearBits);
    }
}

void OpenGLDevice::endRenderPass() {
    mCurrentRenderTarget.clear();
}

void OpenGLDevice::bindPipeline(const PipelineState& state) {
    mCurrentPipeline = state;

    // Shader
    if (static_cast<bool>(state.program)) {
        auto it = mPrograms.find(state.program.getId());
        if (it != mPrograms.end()) {
            glUseProgram(it->second.glId);
        }
    }

    // 深度
    if (state.depthTest) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(toGLCompareFunc(state.depthFunc));
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);

    // 面剔除
    if (state.cullMode != CullMode::None) {
        glEnable(GL_CULL_FACE);
        glCullFace(toGLCullFace(state.cullMode));
    } else {
        glDisable(GL_CULL_FACE);
    }

    // 混合
    if (state.blendEnable) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(
                toGLBlendFactor(state.srcColorBlend),
                toGLBlendFactor(state.dstColorBlend),
                toGLBlendFactor(state.srcAlphaBlend),
                toGLBlendFactor(state.dstAlphaBlend));
    } else {
        glDisable(GL_BLEND);
    }
}

void OpenGLDevice::bindRenderPrimitive(RenderPrimitiveHandle rph) {
    auto it = mRenderPrimitives.find(rph.getId());
    if (it != mRenderPrimitives.end()) {
        glBindVertexArray(it->second.vaoId);
    }
}

void OpenGLDevice::bindUniformBuffer(uint32_t /*set*/, uint32_t binding,
        BufferHandle handle, uint32_t offset, uint32_t size) {
    auto it = mBuffers.find(handle.getId());
    if (it != mBuffers.end()) {
        glBindBufferRange(GL_UNIFORM_BUFFER, binding, it->second.glId, offset, size);
    }
}

void OpenGLDevice::bindTexture(uint32_t /*set*/, uint32_t binding, TextureHandle handle) {
    auto it = mTextures.find(handle.getId());
    if (it != mTextures.end()) {
        glActiveTexture(GL_TEXTURE0 + binding);
        GLenum target = toGLTextureTarget(it->second.desc.type, it->second.desc.samples);
        glBindTexture(target, it->second.glId);
    }
}

void OpenGLDevice::draw(uint32_t indexCount, uint32_t indexOffset, uint32_t instanceCount) {
    GLenum mode = toGLPrimitive(mCurrentPipeline.primitiveType);

    if (indexCount > 0) {
        // 索引绘制
        GLenum idxType = GL_UNSIGNED_INT; // TODO: 从当前绑定的 RenderPrimitive 获取
        size_t byteOffset = indexOffset * (idxType == GL_UNSIGNED_SHORT ? 2 : 4);
        if (instanceCount > 1) {
            glDrawElementsInstanced(mode, indexCount, idxType,
                    reinterpret_cast<const void*>(byteOffset), instanceCount);
        } else {
            glDrawElements(mode, indexCount, idxType,
                    reinterpret_cast<const void*>(byteOffset));
        }
    }
}

void OpenGLDevice::setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    glViewport(x, y, w, h);
}

void OpenGLDevice::setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, w, h);
}

void OpenGLDevice::resolve(RenderTargetHandle src, RenderTargetHandle dst) {
    // resolve = blit 全部颜色附件，不缩放
    GLuint srcFbo = 0, dstFbo = 0;
    uint32_t w = 0, h = 0;

    if (static_cast<bool>(src)) {
        auto it = mRenderTargets.find(src.getId());
        if (it != mRenderTargets.end()) {
            srcFbo = it->second.fboId;
            w = it->second.desc.width;
            h = it->second.desc.height;
        }
    }
    if (static_cast<bool>(dst)) {
        auto it = mRenderTargets.find(dst.getId());
        if (it != mRenderTargets.end()) {
            dstFbo = it->second.fboId;
            // 如果 src 尺寸为 0，用 dst 的
            if (w == 0) { w = it->second.desc.width; h = it->second.desc.height; }
        }
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo);
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLDevice::blit(RenderTargetHandle src, RenderTargetHandle dst,
        uint32_t srcX, uint32_t srcY, uint32_t srcW, uint32_t srcH,
        uint32_t dstX, uint32_t dstY, uint32_t dstW, uint32_t dstH,
        uint8_t mask) {
    GLuint srcFbo = 0, dstFbo = 0;

    if (static_cast<bool>(src)) {
        auto it = mRenderTargets.find(src.getId());
        if (it != mRenderTargets.end()) srcFbo = it->second.fboId;
    }
    if (static_cast<bool>(dst)) {
        auto it = mRenderTargets.find(dst.getId());
        if (it != mRenderTargets.end()) dstFbo = it->second.fboId;
    }

    GLbitfield glMask = 0;
    if (mask & BlitColor)   glMask |= GL_COLOR_BUFFER_BIT;
    if (mask & BlitDepth)   glMask |= GL_DEPTH_BUFFER_BIT;
    if (mask & BlitStencil) glMask |= GL_STENCIL_BUFFER_BIT;

    // 颜色缩放用 LINEAR，深度/模板必须 NEAREST
    GLenum filter = (glMask == GL_COLOR_BUFFER_BIT &&
                     srcW != dstW) ? GL_LINEAR : GL_NEAREST;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo);
    glBlitFramebuffer(srcX, srcY, srcX + srcW, srcY + srcH,
                      dstX, dstY, dstX + dstW, dstY + dstH,
                      glMask, filter);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLDevice::pushDebugGroup(const char* name) {
    if (GLEW_VERSION_4_3 || glewIsSupported("GL_KHR_debug")) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
    }
}

void OpenGLDevice::popDebugGroup() {
    if (GLEW_VERSION_4_3 || glewIsSupported("GL_KHR_debug")) {
        glPopDebugGroup();
    }
}

void OpenGLDevice::flush() {
    glFlush();
}

void OpenGLDevice::finish() {
    glFinish();
}

// ============================================================================
// 内部查询
// ============================================================================

uint32_t OpenGLDevice::getGLTextureId(TextureHandle h) const {
    auto it = mTextures.find(h.getId());
    return it != mTextures.end() ? it->second.glId : 0;
}

uint32_t OpenGLDevice::getGLBufferId(BufferHandle h) const {
    auto it = mBuffers.find(h.getId());
    return it != mBuffers.end() ? it->second.glId : 0;
}

uint32_t OpenGLDevice::getGLProgramId(ProgramHandle h) const {
    auto it = mPrograms.find(h.getId());
    return it != mPrograms.end() ? it->second.glId : 0;
}

} // namespace rhi
} // namespace engine
