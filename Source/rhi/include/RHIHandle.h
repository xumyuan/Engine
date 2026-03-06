#pragma once
#include <type_traits>

namespace engine {
namespace rhi {
// 前向声明所有硬件资源类型
struct HwTexture;
struct HwBuffer;
struct HwProgram;
struct HwRenderTarget;
struct HwRenderPrimitive;
struct HwSwapChain;
struct HwDescriptorSet;

class HandleBase {
public:
    using HandleId = uint32_t;
    static constexpr HandleId INVALID = UINT32_MAX;

    HandleBase() noexcept = default;
    explicit HandleBase(HandleId id) noexcept : mId(id) {}

    explicit operator bool() const noexcept { return mId != INVALID; }
    HandleId getId() const noexcept { return mId; }
    void clear() noexcept { mId = INVALID; }

    bool operator==(HandleBase const& rhs) const noexcept { return mId == rhs.mId; }
    bool operator!=(HandleBase const& rhs) const noexcept { return mId != rhs.mId; }
protected:
    HandleId mId = INVALID;
};

// ===== 类型安全句柄模板 =====
template<typename T>
struct Handle : public HandleBase {
    Handle() noexcept = default;
    explicit Handle(HandleId id) noexcept : HandleBase(id) {}

    // 支持派生类 Handle 的隐式转换
    template<typename U, typename = std::enable_if_t<std::is_base_of_v<T, U>>>
    Handle(Handle<U> const& other) noexcept : HandleBase(other.getId()) {}
};

// ===== 类型别名 =====
using TextureHandle       = Handle<HwTexture>;
using BufferHandle        = Handle<HwBuffer>;
using ProgramHandle       = Handle<HwProgram>;
using RenderTargetHandle  = Handle<HwRenderTarget>;
using RenderPrimitiveHandle = Handle<HwRenderPrimitive>;
using SwapChainHandle     = Handle<HwSwapChain>;
using DescriptorSetHandle = Handle<HwDescriptorSet>;

} // namespace rhi
} // namespace engine