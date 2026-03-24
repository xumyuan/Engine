#pragma once

#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include <vector>
#include <mutex>
#include <cassert>
#include <algorithm>
#include <functional>

namespace engine {
namespace rhi {

// ===================================================================
// 步骤 6: 排序键生成工具
// 排序键用于在同一个 RenderPass 内，按 PipelineState 对 Draw Batch 排序，
// 以减少 GPU 状态切换开销。
//
// 排序键编码（64-bit）：
//   [63..56] passIndex (8 bit)   — RenderPass 编号（保持 pass 间顺序不变）
//   [55..24] programId (32 bit)  — ProgramHandle ID（相同 shader 的命令聚在一起）
//   [23..0]  commandIndex (24 bit) — 原始命令序号（保持同 program 内的顺序）
// ===================================================================
namespace SortKey {
    inline uint64_t encode(uint8_t passIndex, uint32_t programId, uint32_t commandIndex) {
        return (static_cast<uint64_t>(passIndex) << 56)
             | (static_cast<uint64_t>(programId & 0xFFFFFFFF) << 24)
             | (static_cast<uint64_t>(commandIndex & 0x00FFFFFF));
    }
}

// ===================================================================
// CommandQueue - 命令队列
//
// 负责接收 CommandBuffer 的提交，并在渲染线程上按提交顺序执行所有命令。
// 提交操作是线程安全的（通过 mutex 保护），执行操作应在渲染线程上调用。
//
// 支持命令排序优化：sortAndFlush() 按排序键排序后执行。
//
// 用法：
//   CommandQueue queue(device);
//   queue.submit(shadowPassCmdBuf);
//   queue.submit(lightingPassCmdBuf);
//   queue.submit(postProcessCmdBuf);
//   queue.flush();  // 按提交顺序执行所有命令
//   // 或者：
//   queue.sortAndFlush();  // 排序优化后执行
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
    void submit(const CommandBuffer& cmdBuf) {
        if (cmdBuf.empty()) return;
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_PendingBuffers.push_back(&cmdBuf);
    }

    // 在渲染线程上按提交顺序执行所有挂起的命令
    void flush() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (const auto* cmdBuf : m_PendingBuffers) {
            executeCommandBuffer(*cmdBuf);
        }
        m_PendingBuffers.clear();
    }

    // ===================================================================
    // 步骤 6: 排序优化后执行
    // 将所有 pending CommandBuffer 的命令合并，按 sortKey 排序后执行。
    // 排序键由 pass 在录制命令时设置（通过 setSortKey）。
    // 如果 sortKey 都为 0（默认），则等价于普通 flush()。
    // ===================================================================
    void sortAndFlush() {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // 合并所有命令
        m_MergedCommands.clear();
        for (const auto* cmdBuf : m_PendingBuffers) {
            const auto& cmds = cmdBuf->getCommands();
            m_MergedCommands.insert(m_MergedCommands.end(), cmds.begin(), cmds.end());
        }

        // 只有当有排序键时才排序（检查首个非零排序键）
        bool hasSortKeys = false;
        for (const auto& cmd : m_MergedCommands) {
            if (cmd.sortKey != 0) { hasSortKeys = true; break; }
        }

        if (hasSortKeys) {
            std::stable_sort(m_MergedCommands.begin(), m_MergedCommands.end(),
                [](const RenderCommand& a, const RenderCommand& b) {
                    return a.sortKey < b.sortKey;
                });
        }

        // 执行合并后的命令
        for (const auto& cmd : m_MergedCommands) {
            dispatch(cmd);
        }

        m_PendingBuffers.clear();
        m_MergedCommands.clear();
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

    // 更换底层设备
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

            // --- 步骤 5: 高层操作命令 ---
            case CommandType::SetUniformInt:
                m_Device->setUniform(
                    cmd.setUniformInt.program,
                    cmd.setUniformInt.name,
                    cmd.setUniformInt.value);
                break;
            case CommandType::SetUniformFloat:
                m_Device->setUniform(
                    cmd.setUniformFloat.program,
                    cmd.setUniformFloat.name,
                    cmd.setUniformFloat.value);
                break;
            case CommandType::SetUniformVec2:
                m_Device->setUniform(
                    cmd.setUniformVec2.program,
                    cmd.setUniformVec2.name,
                    cmd.setUniformVec2.value);
                break;
            case CommandType::SetUniformVec3:
                m_Device->setUniform(
                    cmd.setUniformVec3.program,
                    cmd.setUniformVec3.name,
                    cmd.setUniformVec3.value);
                break;
            case CommandType::SetUniformVec4:
                m_Device->setUniform(
                    cmd.setUniformVec4.program,
                    cmd.setUniformVec4.name,
                    cmd.setUniformVec4.value);
                break;
            case CommandType::SetUniformMat3:
                m_Device->setUniform(
                    cmd.setUniformMat3.program,
                    cmd.setUniformMat3.name,
                    cmd.setUniformMat3.value);
                break;
            case CommandType::SetUniformMat4:
                m_Device->setUniform(
                    cmd.setUniformMat4.program,
                    cmd.setUniformMat4.name,
                    cmd.setUniformMat4.value);
                break;
            case CommandType::BindTextureUnit:
                m_Device->bindTextureToUnit(
                    cmd.bindTextureUnit.handle,
                    cmd.bindTextureUnit.unit);
                break;
            case CommandType::UpdateBuffer: {
                BufferDataDesc bufData;
                bufData.data = cmd.updateBuffer.data;
                bufData.size = cmd.updateBuffer.dataSize;
                bufData.offset = 0;
                m_Device->updateBuffer(
                    cmd.updateBuffer.handle, bufData);
                break;
            }
            case CommandType::BindUBO:
                m_Device->bindUniformBuffer(
                    0,
                    cmd.bindUBO.binding,
                    cmd.bindUBO.handle,
                    0,
                    cmd.bindUBO.size);
                break;
            case CommandType::BindDefaultFramebuffer:
                m_Device->bindDefaultFramebuffer(
                    cmd.bindDefaultFramebuffer.width,
                    cmd.bindDefaultFramebuffer.height);
                break;
            case CommandType::Clear:
                m_Device->clearFramebuffer(cmd.clear.clearFlags);
                break;
        }
    }

    RHIDevice* m_Device = nullptr;
    std::vector<const CommandBuffer*> m_PendingBuffers;
    std::vector<RenderCommand> m_MergedCommands;  // 步骤 6: 排序合并用
    mutable std::mutex m_Mutex;
};

} // namespace rhi
} // namespace engine
