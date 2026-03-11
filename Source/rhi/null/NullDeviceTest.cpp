#include "rhi/null/NullDevice.h"
#include "rhi/include/RHIShaderCompiler.h"
#include "rhi/include/RHIShaderProgram.h"
#include <cassert>
#include <iostream>

namespace engine::rhi {

void testNullDevice() {
    std::cout << "=== NullDevice Test ===" << std::endl;

    NullDevice device;
    assert(device.initialize());
    assert(device.getBackend() == Backend::Null);
    assert(device.getMaxTextureSize() == 16384);
    assert(device.isTextureFormatSupported(TextureFormat::RGBA8));

    // 资源创建 —— Handle 有 explicit operator bool
    auto tex = device.createTexture({});
    auto buf = device.createBuffer({});
    auto prog = device.createProgram({});
    auto rt = device.createRenderTarget({});
    assert(static_cast<bool>(tex));
    assert(static_cast<bool>(buf));
    assert(static_cast<bool>(prog));
    assert(static_cast<bool>(rt));

    // Handle id 应该各不相同
    assert(tex.getId() != buf.getId());
    assert(buf.getId() != prog.getId());

    // ShaderCompiler 测试
    auto compiler = device.createShaderCompiler();
    assert(compiler != nullptr);
    auto shaderProg = compiler->loadAndCompile("test.glsl");
    assert(shaderProg != nullptr);
    assert(static_cast<bool>(shaderProg->getProgramHandle()));
    shaderProg->use();
    shaderProg->setUniform("test", 1.0f);
    shaderProg->unuse();

    // 渲染流程走通（不崩即通过）
    SwapChainHandle sc = device.createSwapChain(nullptr, 800, 600);
    assert(static_cast<bool>(sc));

    device.beginFrame(sc);
    device.beginRenderPass({}, {});
    device.bindPipeline({});
    device.setViewport(0, 0, 800, 600);
    device.setScissor(0, 0, 800, 600);
    device.draw(0, 0, 1);
    device.endRenderPass();
    device.endFrame();
    device.flush();
    device.finish();

    // 更新操作
    device.updateBuffer(buf, {});
    device.updateTexture(tex, 0, 0, 0, 1, 1, TextureFormat::RGBA8, nullptr, 0);

    // 销毁
    device.destroyTexture(tex);
    device.destroyBuffer(buf);
    device.destroyProgram(prog);
    device.destroyRenderTarget(rt);
    device.destroySwapChain(sc);

    device.terminate();
    std::cout << "=== All NullDevice tests passed ===" << std::endl;
}

} // namespace engine::rhi
