#include "pch.h"
#include "rhi/include/RHICommandBuffer.h"
#include "rhi/include/RHIDevice.h"

namespace engine {
namespace rhi {

void CommandBuffer::dispatchImmediate(const RenderCommand& cmd) {
    // 即时执行模式：如果未设置设备指针，仅录制不执行
    if (!m_ImmediateDevice) return;

    switch (cmd.type) {
        // --- 帧生命周期 ---
        case CommandType::BeginFrame:
            m_ImmediateDevice->beginFrame(cmd.beginFrame.swapChain);
            break;
        case CommandType::EndFrame:
            m_ImmediateDevice->endFrame();
            break;

        // --- RenderPass ---
        case CommandType::BeginRenderPass:
            m_ImmediateDevice->beginRenderPass(
                cmd.beginRenderPass.target,
                cmd.beginRenderPass.params);
            break;
        case CommandType::EndRenderPass:
            m_ImmediateDevice->endRenderPass();
            break;

        // --- 管线与资源绑定 ---
        case CommandType::BindPipeline:
            m_ImmediateDevice->bindPipeline(cmd.bindPipeline.state);
            break;
        case CommandType::BindRenderPrimitive:
            m_ImmediateDevice->bindRenderPrimitive(cmd.bindRenderPrimitive.handle);
            break;
        case CommandType::BindUniformBuffer:
            m_ImmediateDevice->bindUniformBuffer(
                cmd.bindUniformBuffer.set,
                cmd.bindUniformBuffer.binding,
                cmd.bindUniformBuffer.handle,
                cmd.bindUniformBuffer.offset,
                cmd.bindUniformBuffer.size);
            break;
        case CommandType::BindTexture:
            m_ImmediateDevice->bindTexture(
                cmd.bindTexture.set,
                cmd.bindTexture.binding,
                cmd.bindTexture.handle);
            break;

        // --- 绘制 ---
        case CommandType::Draw:
            m_ImmediateDevice->draw(
                cmd.draw.indexCount,
                cmd.draw.indexOffset,
                cmd.draw.instanceCount);
            break;
        case CommandType::DrawArrays:
            m_ImmediateDevice->drawArrays(
                cmd.drawArrays.primitive,
                cmd.drawArrays.vertexCount,
                cmd.drawArrays.firstVertex,
                cmd.drawArrays.instanceCount);
            break;

        // --- 状态设置 ---
        case CommandType::SetViewport:
            m_ImmediateDevice->setViewport(
                cmd.setViewport.x, cmd.setViewport.y,
                cmd.setViewport.w, cmd.setViewport.h);
            break;
        case CommandType::SetScissor:
            m_ImmediateDevice->setScissor(
                cmd.setScissor.x, cmd.setScissor.y,
                cmd.setScissor.w, cmd.setScissor.h);
            break;
        case CommandType::SetPolygonMode:
            m_ImmediateDevice->setPolygonMode(cmd.setPolygonMode.mode);
            break;

        // --- 纹理操作 ---
        case CommandType::GenerateMipmaps:
            m_ImmediateDevice->generateMipmaps(cmd.generateMipmaps.handle);
            break;

        // --- RenderTarget 动态附件 ---
        case CommandType::SetRTColorAttachment:
            m_ImmediateDevice->setRenderTargetColorAttachment(
                cmd.setRTColorAttachment.rt,
                cmd.setRTColorAttachment.attachmentIndex,
                cmd.setRTColorAttachment.texture,
                cmd.setRTColorAttachment.level,
                cmd.setRTColorAttachment.layer);
            break;

        // --- 拷贝与 Blit ---
        case CommandType::CopyTexture:
            m_ImmediateDevice->copyTexture(
                cmd.copyTexture.src,
                cmd.copyTexture.dst,
                cmd.copyTexture.width,
                cmd.copyTexture.height);
            break;
        case CommandType::Blit:
            m_ImmediateDevice->blit(
                cmd.blit.src, cmd.blit.dst,
                cmd.blit.srcX, cmd.blit.srcY,
                cmd.blit.srcW, cmd.blit.srcH,
                cmd.blit.dstX, cmd.blit.dstY,
                cmd.blit.dstW, cmd.blit.dstH,
                cmd.blit.mask);
            break;
        case CommandType::Resolve:
            m_ImmediateDevice->resolve(cmd.resolve.src, cmd.resolve.dst);
            break;

        // --- 调试标记 ---
        case CommandType::PushDebugGroup:
            m_ImmediateDevice->pushDebugGroup(cmd.pushDebugGroup.name);
            break;
        case CommandType::PopDebugGroup:
            m_ImmediateDevice->popDebugGroup();
            break;

        // --- 同步 ---
        case CommandType::Flush:
            m_ImmediateDevice->flush();
            break;
        case CommandType::Finish:
            m_ImmediateDevice->finish();
            break;

        // --- 步骤 5: 高层操作命令 ---
        case CommandType::SetUniformInt:
            m_ImmediateDevice->setUniform(
                cmd.setUniformInt.program,
                cmd.setUniformInt.name,
                cmd.setUniformInt.value);
            break;
        case CommandType::SetUniformFloat:
            m_ImmediateDevice->setUniform(
                cmd.setUniformFloat.program,
                cmd.setUniformFloat.name,
                cmd.setUniformFloat.value);
            break;
        case CommandType::SetUniformVec2:
            m_ImmediateDevice->setUniform(
                cmd.setUniformVec2.program,
                cmd.setUniformVec2.name,
                cmd.setUniformVec2.value);
            break;
        case CommandType::SetUniformVec3:
            m_ImmediateDevice->setUniform(
                cmd.setUniformVec3.program,
                cmd.setUniformVec3.name,
                cmd.setUniformVec3.value);
            break;
        case CommandType::SetUniformVec4:
            m_ImmediateDevice->setUniform(
                cmd.setUniformVec4.program,
                cmd.setUniformVec4.name,
                cmd.setUniformVec4.value);
            break;
        case CommandType::SetUniformMat3:
            m_ImmediateDevice->setUniform(
                cmd.setUniformMat3.program,
                cmd.setUniformMat3.name,
                cmd.setUniformMat3.value);
            break;
        case CommandType::SetUniformMat4:
            m_ImmediateDevice->setUniform(
                cmd.setUniformMat4.program,
                cmd.setUniformMat4.name,
                cmd.setUniformMat4.value);
            break;
        case CommandType::BindTextureUnit:
            m_ImmediateDevice->bindTextureToUnit(
                cmd.bindTextureUnit.handle,
                cmd.bindTextureUnit.unit);
            break;
        case CommandType::UpdateBuffer: {
            BufferDataDesc bufData;
            bufData.data = cmd.updateBuffer.data;
            bufData.size = cmd.updateBuffer.dataSize;
            bufData.offset = 0;
            m_ImmediateDevice->updateBuffer(
                cmd.updateBuffer.handle, bufData);
            break;
        }
        case CommandType::BindUBO:
            m_ImmediateDevice->bindUniformBuffer(
                0,
                cmd.bindUBO.binding,
                cmd.bindUBO.handle,
                0,
                cmd.bindUBO.size);
            break;
        case CommandType::BindDefaultFramebuffer:
            m_ImmediateDevice->bindDefaultFramebuffer(
                cmd.bindDefaultFramebuffer.width,
                cmd.bindDefaultFramebuffer.height);
            break;
        case CommandType::Clear:
            m_ImmediateDevice->clearFramebuffer(cmd.clear.clearFlags);
            break;
    }
}

} // namespace rhi
} // namespace engine
