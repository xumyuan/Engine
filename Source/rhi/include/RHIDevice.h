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

    virtual void flush() = 0;
    virtual void finish() = 0;

    // ---------- 工厂方法 ----------
    static std::unique_ptr<RHIDevice> create(Backend backend);
};
} // namespace rhi
} // namespace engine