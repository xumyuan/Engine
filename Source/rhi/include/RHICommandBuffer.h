#pragma once

#include "RHIResources.h"
#include "RHITypes.h"
#include <vector>
#include <cstring>
#include <cstdint>

namespace engine {
namespace rhi {

// 前向声明（即时执行模式需要）
class RHIDevice;

// ===================================================================
// 命令类型枚举
// 覆盖 RHIDevice 中"渲染命令"部分的所有接口
// ===================================================================
enum class CommandType : uint8_t {
    BeginFrame,
    EndFrame,
    BeginRenderPass,
    EndRenderPass,
    BindPipeline,
    BindRenderPrimitive,
    BindUniformBuffer,
    BindTexture,
    Draw,
    DrawArrays,
    SetViewport,
    SetScissor,
    SetPolygonMode,
    GenerateMipmaps,
    SetRTColorAttachment,
    CopyTexture,
    Blit,
    Resolve,
    PushDebugGroup,
    PopDebugGroup,
    Flush,
    Finish,
};

// ===================================================================
// 各命令的参数结构（POD 类型，保证可以放进 union）
// ===================================================================

struct CmdBeginFrame {
    SwapChainHandle swapChain;
};

struct CmdBeginRenderPass {
    RenderTargetHandle target;
    RenderPassParams   params;
};

struct CmdBindPipeline {
    PipelineState state;
};

struct CmdBindRenderPrimitive {
    RenderPrimitiveHandle handle;
};

struct CmdBindUniformBuffer {
    uint32_t     set;
    uint32_t     binding;
    BufferHandle handle;
    uint32_t     offset;
    uint32_t     size;
};

struct CmdBindTexture {
    uint32_t      set;
    uint32_t      binding;
    TextureHandle handle;
};

struct CmdDraw {
    uint32_t indexCount;
    uint32_t indexOffset;
    uint32_t instanceCount;
};

struct CmdDrawArrays {
    PrimitiveType primitive;
    uint32_t      vertexCount;
    uint32_t      firstVertex;
    uint32_t      instanceCount;
};

struct CmdSetViewport {
    uint32_t x, y, w, h;
};

struct CmdSetScissor {
    uint32_t x, y, w, h;
};

struct CmdSetPolygonMode {
    PolygonMode mode;
};

struct CmdGenerateMipmaps {
    TextureHandle handle;
};

struct CmdSetRTColorAttachment {
    RenderTargetHandle rt;
    uint8_t            attachmentIndex;
    TextureHandle      texture;
    uint8_t            level;
    uint8_t            layer;
};

struct CmdCopyTexture {
    TextureHandle src;
    TextureHandle dst;
    uint32_t      width;
    uint32_t      height;
};

struct CmdBlit {
    RenderTargetHandle src;
    RenderTargetHandle dst;
    uint32_t srcX, srcY, srcW, srcH;
    uint32_t dstX, dstY, dstW, dstH;
    uint8_t  mask;
};

struct CmdResolve {
    RenderTargetHandle src;
    RenderTargetHandle dst;
};

struct CmdPushDebugGroup {
    char name[64]; // 固定长度，避免堆分配
};

// ===================================================================
// 渲染命令节点（union + 类型标签）
// 使用 union 保证缓存友好，避免虚函数和堆分配开销
// ===================================================================
struct RenderCommand {
    CommandType type;

    union {
        CmdBeginFrame           beginFrame;
        CmdBeginRenderPass      beginRenderPass;
        CmdBindPipeline         bindPipeline;
        CmdBindRenderPrimitive  bindRenderPrimitive;
        CmdBindUniformBuffer    bindUniformBuffer;
        CmdBindTexture          bindTexture;
        CmdDraw                 draw;
        CmdDrawArrays           drawArrays;
        CmdSetViewport          setViewport;
        CmdSetScissor           setScissor;
        CmdSetPolygonMode       setPolygonMode;
        CmdGenerateMipmaps      generateMipmaps;
        CmdSetRTColorAttachment setRTColorAttachment;
        CmdCopyTexture          copyTexture;
        CmdBlit                 blit;
        CmdResolve              resolve;
        CmdPushDebugGroup       pushDebugGroup;
    };

    RenderCommand() : type(CommandType::EndRenderPass), beginRenderPass{} {}
};

// ===================================================================
// CommandBuffer - 命令录制器
//
// 用法：
//   CommandBuffer cmdBuf;
//   cmdBuf.beginRenderPass(target, params);
//   cmdBuf.bindPipeline(state);
//   cmdBuf.draw(indexCount, 0);
//   cmdBuf.endRenderPass();
//   // ... 提交到 CommandQueue 执行
//
// 线程安全性：
//   单个 CommandBuffer 不是线程安全的，但不同线程可以各自持有独立的
//   CommandBuffer 并行录制命令。
// ===================================================================
class CommandBuffer {
public:
    CommandBuffer() {
        m_Commands.reserve(256);
    }

    // ---------- 即时执行模式 ----------
    // 设置即时执行设备后，每条录制的命令会同时立即分发到设备执行。
    // 这是当前过渡阶段的临时方案：部分高层操作（setUniform、Texture::bind）
    // 仍直接调用 OpenGL，需要 bindPipeline 等命令立即生效。
    // 传入 nullptr 可关闭即时执行模式（回到纯录制模式）。
    void setImmediateDevice(RHIDevice* device) { m_ImmediateDevice = device; }
    RHIDevice* getImmediateDevice() const { return m_ImmediateDevice; }

    // ---------- 状态管理 ----------
    void reset() { m_Commands.clear(); }
    bool empty() const { return m_Commands.empty(); }
    size_t size() const { return m_Commands.size(); }

    // 获取只读命令列表（供 CommandQueue 遍历执行）
    const std::vector<RenderCommand>& getCommands() const { return m_Commands; }

    // ---------- 帧生命周期 ----------

    void beginFrame(SwapChainHandle swapChain) {
        auto& cmd = emplaceCommand(CommandType::BeginFrame);
        cmd.beginFrame.swapChain = swapChain;
        dispatchImmediate(cmd);
    }

    void endFrame() {
        auto& cmd = emplaceCommand(CommandType::EndFrame);
        dispatchImmediate(cmd);
    }

    // ---------- RenderPass ----------

    void beginRenderPass(RenderTargetHandle target, const RenderPassParams& params) {
        auto& cmd = emplaceCommand(CommandType::BeginRenderPass);
        cmd.beginRenderPass.target = target;
        cmd.beginRenderPass.params = params;
        dispatchImmediate(cmd);
    }

    void endRenderPass() {
        auto& cmd = emplaceCommand(CommandType::EndRenderPass);
        dispatchImmediate(cmd);
    }

    // ---------- 管线与资源绑定 ----------

    void bindPipeline(const PipelineState& state) {
        auto& cmd = emplaceCommand(CommandType::BindPipeline);
        cmd.bindPipeline.state = state;
        dispatchImmediate(cmd);
    }

    void bindRenderPrimitive(RenderPrimitiveHandle handle) {
        auto& cmd = emplaceCommand(CommandType::BindRenderPrimitive);
        cmd.bindRenderPrimitive.handle = handle;
        dispatchImmediate(cmd);
    }

    void bindUniformBuffer(uint32_t set, uint32_t binding,
                           BufferHandle handle, uint32_t offset, uint32_t size) {
        auto& cmd = emplaceCommand(CommandType::BindUniformBuffer);
        cmd.bindUniformBuffer = { set, binding, handle, offset, size };
        dispatchImmediate(cmd);
    }

    void bindTexture(uint32_t set, uint32_t binding, TextureHandle handle) {
        auto& cmd = emplaceCommand(CommandType::BindTexture);
        cmd.bindTexture = { set, binding, handle };
        dispatchImmediate(cmd);
    }

    // ---------- 绘制 ----------

    void draw(uint32_t indexCount, uint32_t indexOffset, uint32_t instanceCount = 1) {
        auto& cmd = emplaceCommand(CommandType::Draw);
        cmd.draw = { indexCount, indexOffset, instanceCount };
        dispatchImmediate(cmd);
    }

    void drawArrays(PrimitiveType primitive, uint32_t vertexCount,
                    uint32_t firstVertex = 0, uint32_t instanceCount = 1) {
        auto& cmd = emplaceCommand(CommandType::DrawArrays);
        cmd.drawArrays = { primitive, vertexCount, firstVertex, instanceCount };
        dispatchImmediate(cmd);
    }

    // ---------- 状态设置 ----------

    void setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        auto& cmd = emplaceCommand(CommandType::SetViewport);
        cmd.setViewport = { x, y, w, h };
        dispatchImmediate(cmd);
    }

    void setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        auto& cmd = emplaceCommand(CommandType::SetScissor);
        cmd.setScissor = { x, y, w, h };
        dispatchImmediate(cmd);
    }

    void setPolygonMode(PolygonMode mode) {
        auto& cmd = emplaceCommand(CommandType::SetPolygonMode);
        cmd.setPolygonMode.mode = mode;
        dispatchImmediate(cmd);
    }

    // ---------- 纹理操作 ----------

    void generateMipmaps(TextureHandle handle) {
        auto& cmd = emplaceCommand(CommandType::GenerateMipmaps);
        cmd.generateMipmaps.handle = handle;
        dispatchImmediate(cmd);
    }

    // ---------- RenderTarget 动态附件管理 ----------

    void setRenderTargetColorAttachment(RenderTargetHandle rt,
                                         uint8_t attachmentIndex,
                                         TextureHandle texture,
                                         uint8_t level = 0,
                                         uint8_t layer = 0) {
        auto& cmd = emplaceCommand(CommandType::SetRTColorAttachment);
        cmd.setRTColorAttachment = { rt, attachmentIndex, texture, level, layer };
        dispatchImmediate(cmd);
    }

    // ---------- 拷贝与 Blit ----------

    void copyTexture(TextureHandle src, TextureHandle dst,
                     uint32_t width, uint32_t height) {
        auto& cmd = emplaceCommand(CommandType::CopyTexture);
        cmd.copyTexture = { src, dst, width, height };
        dispatchImmediate(cmd);
    }

    void blit(RenderTargetHandle src, RenderTargetHandle dst,
              uint32_t srcX, uint32_t srcY, uint32_t srcW, uint32_t srcH,
              uint32_t dstX, uint32_t dstY, uint32_t dstW, uint32_t dstH,
              uint8_t mask = 0x01 /*BlitColor*/) {
        auto& cmd = emplaceCommand(CommandType::Blit);
        cmd.blit = { src, dst, srcX, srcY, srcW, srcH, dstX, dstY, dstW, dstH, mask };
        dispatchImmediate(cmd);
    }

    void resolve(RenderTargetHandle src, RenderTargetHandle dst) {
        auto& cmd = emplaceCommand(CommandType::Resolve);
        cmd.resolve = { src, dst };
        dispatchImmediate(cmd);
    }

    // ---------- 调试标记 ----------

    void pushDebugGroup(const char* name) {
        auto& cmd = emplaceCommand(CommandType::PushDebugGroup);
        std::strncpy(cmd.pushDebugGroup.name, name,
                     sizeof(cmd.pushDebugGroup.name) - 1);
        cmd.pushDebugGroup.name[sizeof(cmd.pushDebugGroup.name) - 1] = '\0';
        dispatchImmediate(cmd);
    }

    void popDebugGroup() {
        auto& cmd = emplaceCommand(CommandType::PopDebugGroup);
        dispatchImmediate(cmd);
    }

    // ---------- 同步 ----------

    void flush() {
        auto& cmd = emplaceCommand(CommandType::Flush);
        dispatchImmediate(cmd);
    }

    void finish() {
        auto& cmd = emplaceCommand(CommandType::Finish);
        dispatchImmediate(cmd);
    }

private:
    // 在 vector 末尾就地构造一个命令节点并返回引用
    RenderCommand& emplaceCommand(CommandType type) {
        m_Commands.emplace_back();
        auto& cmd = m_Commands.back();
        cmd.type = type;
        return cmd;
    }

    // 即时分发：如果设置了即时执行设备，立即执行刚录制的命令
    void dispatchImmediate(const RenderCommand& cmd);

    std::vector<RenderCommand> m_Commands;
    RHIDevice* m_ImmediateDevice = nullptr;  // 即时执行设备指针（nullptr = 纯录制模式）
};

} // namespace rhi
} // namespace engine
