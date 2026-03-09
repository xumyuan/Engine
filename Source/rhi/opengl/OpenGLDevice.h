#pragma once
#include "rhi/include/RHIDevice.h"
#include <unordered_map>

namespace engine {
namespace rhi {

// OpenGL 后端内部资源记录
struct GLTextureData {
    uint32_t glId = 0;
    TextureDesc desc;
};

struct GLBufferData {
    uint32_t glId = 0;
    uint32_t glTarget = 0; // GL_ARRAY_BUFFER / GL_ELEMENT_ARRAY_BUFFER / GL_UNIFORM_BUFFER
    BufferDesc desc;
};

struct GLProgramData {
    uint32_t glId = 0;
};

struct GLRenderTargetData {
    uint32_t fboId = 0;
    RenderTargetDesc desc;
};

struct GLRenderPrimitiveData {
    uint32_t vaoId = 0;
    BufferHandle indexBuffer;
    IndexType indexType = IndexType::UInt32;
};

struct GLSwapChainData {
    void* nativeWindow = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
};

class OpenGLDevice final : public RHIDevice {
public:
    bool initialize() override;
    void terminate() override;

    // ---------- 能力查询 ----------
    Backend getBackend() const noexcept override { return Backend::OpenGL; }
    bool isTextureFormatSupported(TextureFormat format) const noexcept override;
    uint32_t getMaxTextureSize() const noexcept override;

    // ---------- 资源创建 ----------
    TextureHandle createTexture(const TextureDesc& desc) override;
    BufferHandle createBuffer(const BufferDesc& desc) override;
    void generateMipmaps(const TextureHandle handle) override;
    
    ProgramHandle createProgram(const ProgramDesc& desc) override;
    RenderTargetHandle createRenderTarget(const RenderTargetDesc& desc) override;
    RenderPrimitiveHandle createRenderPrimitive(
            const VertexLayout& layout,
            BufferHandle vertexBuffers[],
            uint8_t vertexBufferCount,
            BufferHandle indexBuffer,
            IndexType indexType) override;
    SwapChainHandle createSwapChain(void* nativeWindow,
            uint32_t width, uint32_t height) override;

    // ---------- 资源销毁 ----------
    void destroyTexture(TextureHandle handle) override;
    void destroyBuffer(BufferHandle handle) override;
    void destroyProgram(ProgramHandle handle) override;
    void destroyRenderTarget(RenderTargetHandle handle) override;
    void destroyRenderPrimitive(RenderPrimitiveHandle handle) override;
    void destroySwapChain(SwapChainHandle handle) override;

    // ---------- 资源更新 ----------
    void updateBuffer(BufferHandle handle, const BufferDataDesc& data) override;
    void updateTexture(TextureHandle handle, uint32_t level,
            uint32_t xoffset, uint32_t yoffset,
            uint32_t width, uint32_t height,
            const void* data, uint32_t dataSize) override;

    // ---------- 渲染命令 ----------
    void beginFrame(SwapChainHandle swapChain) override;
    void endFrame() override;
    void beginRenderPass(RenderTargetHandle target, const RenderPassParams& params) override;
    void endRenderPass() override;

    void bindPipeline(const PipelineState& state) override;
    void bindRenderPrimitive(RenderPrimitiveHandle rph) override;
    void bindUniformBuffer(uint32_t set, uint32_t binding,
            BufferHandle handle, uint32_t offset, uint32_t size) override;
    void bindTexture(uint32_t set, uint32_t binding, TextureHandle handle) override;

    void draw(uint32_t indexCount, uint32_t indexOffset, uint32_t instanceCount = 1) override;
    void setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
    void setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
    void setPolygonMode(PolygonMode mode) override;

    void resolve(RenderTargetHandle src, RenderTargetHandle dst) override;
    void blit(RenderTargetHandle src, RenderTargetHandle dst,
            uint32_t srcX, uint32_t srcY, uint32_t srcW, uint32_t srcH,
            uint32_t dstX, uint32_t dstY, uint32_t dstW, uint32_t dstH,
            uint8_t mask = BlitColor) override;

    void pushDebugGroup(const char* name) override;
    void popDebugGroup() override;

    void flush() override;
    void finish() override;

    // ---------- OpenGL 特有：内部查询 ----------
    uint32_t getGLTextureId(TextureHandle h) const;
    uint32_t getGLBufferId(BufferHandle h) const;
    uint32_t getGLProgramId(ProgramHandle h) const;

private:
    HandleBase::HandleId allocHandle();

    // Handle → GL 资源映射
    std::unordered_map<HandleBase::HandleId, GLTextureData>         mTextures;
    std::unordered_map<HandleBase::HandleId, GLBufferData>          mBuffers;
    std::unordered_map<HandleBase::HandleId, GLProgramData>         mPrograms;
    std::unordered_map<HandleBase::HandleId, GLRenderTargetData>    mRenderTargets;
    std::unordered_map<HandleBase::HandleId, GLRenderPrimitiveData> mRenderPrimitives;
    std::unordered_map<HandleBase::HandleId, GLSwapChainData>       mSwapChains;

    HandleBase::HandleId mNextHandle = 1;
    uint32_t mMaxTextureSize = 0;

    // 当前绑定状态跟踪
    PipelineState mCurrentPipeline;
    RenderTargetHandle mCurrentRenderTarget;
    SwapChainHandle mCurrentSwapChain;
};

} // namespace rhi
} // namespace engine
