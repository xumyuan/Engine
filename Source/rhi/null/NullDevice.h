#pragma once
#include "rhi/include/RHIDevice.h"

namespace engine {
namespace rhi {

// Null 后端：所有操作空实现，用于测试 RHI API 正确性
class NullDevice final : public RHIDevice {
public:
    bool initialize() override { return true; }
    void terminate() override {}

    Backend getBackend() const noexcept override { return Backend::Null; }
    bool isTextureFormatSupported(TextureFormat) const noexcept override { return true; }
    uint32_t getMaxTextureSize() const noexcept override { return 16384; }

    TextureHandle createTexture(const TextureDesc&) override {
        return TextureHandle(mNextHandle++);
    }
    void generateMipmaps(const TextureHandle handle) override {}
    BufferHandle createBuffer(const BufferDesc&) override {
        return BufferHandle(mNextHandle++);
    }
    ProgramHandle createProgram(const ProgramDesc&) override {
        return ProgramHandle(mNextHandle++);
    }
    RenderTargetHandle createRenderTarget(const RenderTargetDesc&) override {
        return RenderTargetHandle(mNextHandle++);
    }
    RenderPrimitiveHandle createRenderPrimitive(
            const VertexLayout&, BufferHandle[], uint8_t,
            BufferHandle, IndexType) override {
        return RenderPrimitiveHandle(mNextHandle++);
    }
    SwapChainHandle createSwapChain(void*, uint32_t, uint32_t) override {
        return SwapChainHandle(mNextHandle++);
    }

    void destroyTexture(TextureHandle) override {}
    void destroyBuffer(BufferHandle) override {}
    void destroyProgram(ProgramHandle) override {}
    void destroyRenderTarget(RenderTargetHandle) override {}
    void destroyRenderPrimitive(RenderPrimitiveHandle) override {}
    void destroySwapChain(SwapChainHandle) override {}

    void updateBuffer(BufferHandle, const BufferDataDesc&) override {}
    void updateTexture(TextureHandle, uint32_t, uint32_t, uint32_t,
            uint32_t, uint32_t, TextureFormat, const void*, uint32_t) override {}
    void updateCubemapFace(TextureHandle, uint8_t, uint32_t,
            uint32_t, uint32_t, TextureFormat, const void*, uint32_t) override {}
    void updateTextureSampler(TextureHandle, const TextureDesc&) override {}

    void beginFrame(SwapChainHandle) override {}
    void endFrame() override {}
    void beginRenderPass(RenderTargetHandle, const RenderPassParams&) override {}
    void endRenderPass() override {}
    void bindPipeline(const PipelineState&) override {}
    void bindRenderPrimitive(RenderPrimitiveHandle) override {}
    void bindUniformBuffer(uint32_t, uint32_t, BufferHandle, uint32_t, uint32_t) override {}
    void bindTexture(uint32_t, uint32_t, TextureHandle) override {}
    void draw(uint32_t, uint32_t, uint32_t) override {}
    void setViewport(uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void setScissor(uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void setPolygonMode(PolygonMode) override {}
    void resolve(RenderTargetHandle, RenderTargetHandle) override {}
    void blit(RenderTargetHandle, RenderTargetHandle,
            uint32_t, uint32_t, uint32_t, uint32_t,
            uint32_t, uint32_t, uint32_t, uint32_t, uint8_t) override {}
    void setRenderTargetColorAttachment(RenderTargetHandle, uint8_t,
            TextureHandle, uint8_t, uint8_t) override {}
    void copyTexture(TextureHandle, TextureHandle, uint32_t, uint32_t) override {}

    void pushDebugGroup(const char*) override {}
    void popDebugGroup() override {}

    void flush() override {}
    void finish() override {}

private:
    uint32_t mNextHandle = 1;
};

} // namespace rhi
} // namespace engine