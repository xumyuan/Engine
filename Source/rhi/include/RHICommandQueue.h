#pragma once

#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include <vector>
#include <mutex>
#include <cassert>

namespace engine {
namespace rhi {

// ===================================================================
// CommandQueue - 命令队列
//
// 负责接收 CommandBuffer 的提交，并在渲染线程上按提交顺序执行所有命令。
// 提交操作是线程安全的（通过 mutex 保护），执行操作应在渲染线程上调用。
//
// 用法：
//   CommandQueue queue(device);
//   queue.submit(shadowPassCmdBuf);
//   queue.submit(lightingPassCmdBuf);
//   queue.submit(postProcessCmdBuf);
//   queue.flush();  // 按提交顺序执行所有命令
//
// ===================================================================
class CommandQueue {
public:
    explicit CommandQueue(RHIDevice* device)
        : m_Device(device) {
        assert(device != nullptr && "CommandQueue 需要有效的 RHIDevice 实例");
        m_PendingBuffers.reserve(16);
    }

    // 提交一个 CommandBuffer（线程安全，可从不同线程调用）
    // 注意：提交后到 flush() 调用前，CommandBuffer 的内容不应被修改
    void submit(const CommandBuffer& cmdBuf) {
        if (cmdBuf.empty()) return;
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_PendingBuffers.push_back(&cmdBuf);
    }

    // 在渲染线程上按提交顺序执行所有挂起的命令
    // 执行完毕后清空挂起列表
    void flush() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (const auto* cmdBuf : m_PendingBuffers) {
            executeCommandBuffer(*cmdBuf);
        }
        m_PendingBuffers.clear();
    }

    // 获取当前挂起的 CommandBuffer 数量
    size_t pendingCount() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_PendingBuffers.size();
    }

    // 清空所有挂起的 CommandBuffer（不执行）
    void clear() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_PendingBuffers.clear();
    }

    // 更换底层设备（一般不需要，但支持热切换后端时有用）
    void setDevice(RHIDevice* device) {
        assert(device != nullptr);
        m_Device = device;
    }

private:
    // 执行单个 CommandBuffer 中的所有命令
    void executeCommandBuffer(const CommandBuffer& cmdBuf) {
        for (const auto& cmd : cmdBuf.getCommands()) {
            dispatch(cmd);
        }
    }

    // 命令分发：将 RenderCommand 映射到 RHIDevice 对应的方法调用
    void dispatch(const RenderCommand& cmd) {
        switch (cmd.type) {
            // --- 帧生命周期 ---
            case CommandType::BeginFrame:
                m_Device->beginFrame(cmd.beginFrame.swapChain);
                break;
            case CommandType::EndFrame:
                m_Device->endFrame();
                break;

            // --- RenderPass ---
            case CommandType::BeginRenderPass:
                m_Device->beginRenderPass(
                    cmd.beginRenderPass.target,
                    cmd.beginRenderPass.params);
                break;
            case CommandType::EndRenderPass:
                m_Device->endRenderPass();
                break;

            // --- 管线与资源绑定 ---
            case CommandType::BindPipeline:
                m_Device->bindPipeline(cmd.bindPipeline.state);
                break;
            case CommandType::BindRenderPrimitive:
                m_Device->bindRenderPrimitive(cmd.bindRenderPrimitive.handle);
                break;
            case CommandType::BindUniformBuffer:
                m_Device->bindUniformBuffer(
                    cmd.bindUniformBuffer.set,
                    cmd.bindUniformBuffer.binding,
                    cmd.bindUniformBuffer.handle,
                    cmd.bindUniformBuffer.offset,
                    cmd.bindUniformBuffer.size);
                break;
            case CommandType::BindTexture:
                m_Device->bindTexture(
                    cmd.bindTexture.set,
                    cmd.bindTexture.binding,
                    cmd.bindTexture.handle);
                break;

            // --- 绘制 ---
            case CommandType::Draw:
                m_Device->draw(
                    cmd.draw.indexCount,
                    cmd.draw.indexOffset,
                    cmd.draw.instanceCount);
                break;
            case CommandType::DrawArrays:
                m_Device->drawArrays(
                    cmd.drawArrays.primitive,
                    cmd.drawArrays.vertexCount,
                    cmd.drawArrays.firstVertex,
                    cmd.drawArrays.instanceCount);
                break;

            // --- 状态设置 ---
            case CommandType::SetViewport:
                m_Device->setViewport(
                    cmd.setViewport.x, cmd.setViewport.y,
                    cmd.setViewport.w, cmd.setViewport.h);
                break;
            case CommandType::SetScissor:
                m_Device->setScissor(
                    cmd.setScissor.x, cmd.setScissor.y,
                    cmd.setScissor.w, cmd.setScissor.h);
                break;
            case CommandType::SetPolygonMode:
                m_Device->setPolygonMode(cmd.setPolygonMode.mode);
                break;

            // --- 纹理操作 ---
            case CommandType::GenerateMipmaps:
                m_Device->generateMipmaps(cmd.generateMipmaps.handle);
                break;

            // --- RenderTarget 动态附件 ---
            case CommandType::SetRTColorAttachment:
                m_Device->setRenderTargetColorAttachment(
                    cmd.setRTColorAttachment.rt,
                    cmd.setRTColorAttachment.attachmentIndex,
                    cmd.setRTColorAttachment.texture,
                    cmd.setRTColorAttachment.level,
                    cmd.setRTColorAttachment.layer);
                break;

            // --- 拷贝与 Blit ---
            case CommandType::CopyTexture:
                m_Device->copyTexture(
                    cmd.copyTexture.src,
                    cmd.copyTexture.dst,
                    cmd.copyTexture.width,
                    cmd.copyTexture.height);
                break;
            case CommandType::Blit:
                m_Device->blit(
                    cmd.blit.src, cmd.blit.dst,
                    cmd.blit.srcX, cmd.blit.srcY,
                    cmd.blit.srcW, cmd.blit.srcH,
                    cmd.blit.dstX, cmd.blit.dstY,
                    cmd.blit.dstW, cmd.blit.dstH,
                    cmd.blit.mask);
                break;
            case CommandType::Resolve:
                m_Device->resolve(cmd.resolve.src, cmd.resolve.dst);
                break;

            // --- 调试标记 ---
            case CommandType::PushDebugGroup:
                m_Device->pushDebugGroup(cmd.pushDebugGroup.name);
                break;
            case CommandType::PopDebugGroup:
                m_Device->popDebugGroup();
                break;

            // --- 同步 ---
            case CommandType::Flush:
                m_Device->flush();
                break;
            case CommandType::Finish:
                m_Device->finish();
                break;
        }
    }

    RHIDevice* m_Device = nullptr;
    std::vector<const CommandBuffer*> m_PendingBuffers;
    mutable std::mutex m_Mutex;
};

} // namespace rhi
} // namespace engine
