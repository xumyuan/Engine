#pragma once

#include "RHIHandle.h"
#include "RHIResources.h"
#include "RHITypes.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace engine {
namespace rhi {

class RHIShaderCompiler;

class RHIDevice {
    public:
    virtual ~RHIDevice() = default;

    // ---------- 生命周期 ----------
    virtual bool initialize() = 0;
    virtual void terminate() = 0;

    // ---------- 能力查询（同步） ----------
    virtual Backend getBackend() const noexcept = 0;
    virtual bool isTextureFormatSupported(TextureFormat format) const noexcept = 0;
    virtual uint32_t getMaxTextureSize() const noexcept = 0;

    // ---------- 资源创建 ----------
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual void generateMipmaps(const TextureHandle handle) = 0;
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual ProgramHandle createProgram(const ProgramDesc& desc) = 0;
    virtual RenderTargetHandle createRenderTarget(const RenderTargetDesc& desc) = 0;
    virtual RenderPrimitiveHandle createRenderPrimitive(
            const VertexLayout& layout,
            BufferHandle vertexBuffers[],
            uint8_t vertexBufferCount,
            BufferHandle indexBuffer,
            IndexType indexType) = 0;
    virtual SwapChainHandle createSwapChain(void* nativeWindow,
            uint32_t width, uint32_t height) = 0;

    // ---------- 资源销毁 ----------
    virtual void destroyTexture(TextureHandle handle) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;
    virtual void destroyProgram(ProgramHandle handle) = 0;
    virtual void destroyRenderTarget(RenderTargetHandle handle) = 0;
    virtual void destroyRenderPrimitive(RenderPrimitiveHandle handle) = 0;
    virtual void destroySwapChain(SwapChainHandle handle) = 0;

    // ---------- 资源更新 ----------
    virtual void updateBuffer(BufferHandle handle, const BufferDataDesc& data) = 0;

    // srcFormat: 源数据的实际像素格式（用于确定通道数和类型）
    // 当 srcFormat 与纹理内部格式不同时（如 RGB8 数据上传到 RGBA8 纹理），驱动会自动转换
    virtual void updateTexture(TextureHandle handle, uint32_t level,
            uint32_t xoffset, uint32_t yoffset,
            uint32_t width, uint32_t height,
            TextureFormat srcFormat,
            const void* data, uint32_t dataSize) = 0;

    // 更新 cubemap 某一面的数据（face: 0~5 对应 +X,-X,+Y,-Y,+Z,-Z）
    // srcFormat: 源数据的实际像素格式
    virtual void updateCubemapFace(TextureHandle handle, uint8_t face,
            uint32_t level, uint32_t width, uint32_t height,
            TextureFormat srcFormat,
            const void* data, uint32_t dataSize) = 0;

    // 更新纹理采样器参数
    virtual void updateTextureSampler(TextureHandle handle,
            const TextureDesc& desc) = 0;

    // ---------- Shader 工厂 ----------
    // 创建着色器编译器（各后端提供具体实现）
    virtual std::unique_ptr<RHIShaderCompiler> createShaderCompiler() = 0;

    // ---------- 渲染命令 ----------
    virtual void beginFrame(SwapChainHandle swapChain) = 0;
    virtual void endFrame() = 0;

    virtual void beginRenderPass(RenderTargetHandle target,
            const RenderPassParams& params) = 0;
    virtual void endRenderPass() = 0;

    virtual void bindPipeline(const PipelineState& state) = 0;
    virtual void bindRenderPrimitive(RenderPrimitiveHandle rph) = 0;
    virtual void bindUniformBuffer(uint32_t set, uint32_t binding,
            BufferHandle handle, uint32_t offset, uint32_t size) = 0;
    virtual void bindTexture(uint32_t set, uint32_t binding,
            TextureHandle handle) = 0;

    virtual void draw(uint32_t indexCount, uint32_t indexOffset,
            uint32_t instanceCount = 1) = 0;
    virtual void drawArrays(PrimitiveType primitive, uint32_t vertexCount,
            uint32_t firstVertex = 0, uint32_t instanceCount = 1) = 0;
    virtual void setViewport(uint32_t x, uint32_t y,
            uint32_t w, uint32_t h) = 0;
    virtual void setScissor(uint32_t x, uint32_t y,
            uint32_t w, uint32_t h) = 0;
    virtual void setPolygonMode(PolygonMode mode) = 0;

    // ---------- RenderTarget 动态附件管理 ----------
    // 运行时替换渲染目标的颜色附件（用于将纹理/cubemap 面挂到已有 RT 上渲染）
    // attachmentIndex: 颜色附件索引 (0 ~ MAX_COLOR_ATTACHMENTS-1)
    // texture: 要挂载的纹理 (无效 handle 表示解绑)
    // level: mip 层级
    // layer: 数组层/cubemap 面 (对 2D 纹理传 0；cubemap 面传 0~5)
    virtual void setRenderTargetColorAttachment(RenderTargetHandle rt,
            uint8_t attachmentIndex, TextureHandle texture,
            uint8_t level = 0, uint8_t layer = 0) = 0;

    // 纹理间拷贝（替代 glCopyImageSubData）
    virtual void copyTexture(TextureHandle src, TextureHandle dst,
            uint32_t width, uint32_t height) = 0;

    // ---------- Blit / Resolve ----------
    // MSAA resolve：将多重采样 RT 解析到单采样 RT
    virtual void resolve(RenderTargetHandle src, RenderTargetHandle dst) = 0;

    // Blit：在两个 RT 之间拷贝指定附件（颜色/深度/模板）
    enum BlitMask : uint8_t {
        BlitColor   = 0x01,
        BlitDepth   = 0x02,
        BlitStencil = 0x04,
    };
    virtual void blit(RenderTargetHandle src, RenderTargetHandle dst,
            uint32_t srcX, uint32_t srcY, uint32_t srcW, uint32_t srcH,
            uint32_t dstX, uint32_t dstY, uint32_t dstW, uint32_t dstH,
            uint8_t mask = BlitColor) = 0;

    // ---------- Uniform 设置（通过 ProgramHandle） ----------
    // 这些方法在命令队列 dispatch 时调用，替代直接访问 RHIShaderProgram
    // 实现中需要根据 ProgramHandle 找到对应的 GL program 并设置 uniform
    virtual void setUniform(ProgramHandle program, const char* name, int value) = 0;
    virtual void setUniform(ProgramHandle program, const char* name, float value) = 0;
    virtual void setUniform(ProgramHandle program, const char* name, const glm::vec2& value) = 0;
    virtual void setUniform(ProgramHandle program, const char* name, const glm::vec3& value) = 0;
    virtual void setUniform(ProgramHandle program, const char* name, const glm::vec4& value) = 0;
    virtual void setUniform(ProgramHandle program, const char* name, const glm::mat3& value) = 0;
    virtual void setUniform(ProgramHandle program, const char* name, const glm::mat4& value) = 0;

    // 将纹理绑定到指定纹理单元（等价于 bindTexture(0, unit, handle)，语义更明确）
    virtual void bindTextureToUnit(TextureHandle handle, uint32_t unit) {
        bindTexture(0, unit, handle);
    }

    // ---------- 帧缓冲/清除 ----------
    // 绑定默认帧缓冲（等价于 Window::bind()）
    virtual void bindDefaultFramebuffer(uint32_t width, uint32_t height) = 0;
    // 清除帧缓冲（flags: 1=color, 2=depth, 4=stencil 的位掩码）
    virtual void clearFramebuffer(uint8_t clearFlags) = 0;

    // ---------- 调试标记 ----------
    virtual void pushDebugGroup(const char* name) = 0;
    virtual void popDebugGroup() = 0;

    // ---------- 同步 ----------
    virtual void flush() = 0;
    virtual void finish() = 0;

    // ---------- 工厂方法 ----------
    static std::unique_ptr<RHIDevice> create(Backend backend);
};
} // namespace rhi
} // namespace engine