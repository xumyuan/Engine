#pragma once

#include "RHIHandle.h"
#include "RHIResources.h"
#include "RHITypes.h"
#include <memory>

namespace engine {
namespace rhi {
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
    virtual void updateTexture(TextureHandle handle, uint32_t level,
            uint32_t xoffset, uint32_t yoffset,
            uint32_t width, uint32_t height,
            const void* data, uint32_t dataSize) = 0;

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
    virtual void setViewport(uint32_t x, uint32_t y,
            uint32_t w, uint32_t h) = 0;
    virtual void setScissor(uint32_t x, uint32_t y,
            uint32_t w, uint32_t h) = 0;
    virtual void setPolygonMode(PolygonMode mode) = 0;

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