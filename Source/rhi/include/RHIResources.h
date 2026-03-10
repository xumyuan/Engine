#pragma once
#include "RHIHandle.h"
#include "RHITypes.h"

namespace engine {
namespace rhi {

// ===== 采样器过滤 =====
enum class FilterMode : uint8_t {
  Nearest,
  Linear,
  NearestMipmapNearest,
  LinearMipmapNearest,
  NearestMipmapLinear,
  LinearMipmapLinear,
};

enum class WrapMode : uint8_t {
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder,
};

// ===== 纹理描述 =====
struct TextureDesc {
  TextureType type = TextureType::Texture2D;
  TextureFormat format = TextureFormat::RGBA8;
  TextureUsage usage = TextureUsage::Default;
  uint32_t width = 1;
  uint32_t height = 1;
  uint32_t depth = 1;  // 3D 纹理深度或数组层数
  uint8_t levels = 1;  // mipmap 层数
  uint8_t samples = 1; // MSAA 采样数

  // 采样器参数（创建时可选设定）
  FilterMode minFilter = FilterMode::LinearMipmapLinear;
  FilterMode magFilter = FilterMode::Linear;
  WrapMode wrapS = WrapMode::Repeat;
  WrapMode wrapT = WrapMode::Repeat;
  WrapMode wrapR = WrapMode::Repeat;
  float anisotropy = 1.0f;
  bool hasBorder = false;
  float borderColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  int lodBias = 0;
};

// ===== 缓冲区描述 =====
struct BufferDesc {
    BufferUsage usage    = BufferUsage::Vertex;
    uint32_t    size     = 0;      // 字节数
    bool        dynamic  = false;  // 是否频繁更新
};

// ===== 顶点属性 =====
enum class VertexAttribType : uint8_t {
  Float,
  Float2,
  Float3,
  Float4,
  Int,
  Int2,
  Int3,
  Int4,
  UByte4Norm,
};

struct VertexAttribute {
    uint8_t         bufferIndex = 0;    // 引用哪个 vertex buffer
    uint32_t        offset      = 0;    // 在顶点中的偏移
    VertexAttribType type       = VertexAttribType::Float3;
};

struct VertexLayout {
    VertexAttribute attributes[MAX_VERTEX_ATTRIBUTES];
    uint32_t        strides[MAX_VERTEX_ATTRIBUTES] = {};
    uint8_t         attributeCount = 0;
    uint8_t         bufferCount    = 0;
};

// ===== 着色器描述 =====
struct ShaderSource {
    ShaderStage             stage = ShaderStage::Vertex;
    std::vector<uint8_t>    code;   // SPIR-V / GLSL / MSL / DXIL
    std::string             entryPoint = "main";
};

struct ProgramDesc {
    std::string                 name;
    std::vector<ShaderSource>   shaders;
};

// ===== 渲染目标描述 =====
struct RenderTargetDesc {
    TextureHandle   colorAttachments[MAX_COLOR_ATTACHMENTS];
    uint8_t         colorLevels[MAX_COLOR_ATTACHMENTS] = {};
    uint8_t         colorLayers[MAX_COLOR_ATTACHMENTS] = {};
    uint8_t         colorCount = 0;
    TextureHandle   depthAttachment;
    uint8_t         depthLevel = 0;
    uint32_t        width  = 0;
    uint32_t        height = 0;
    uint8_t         samples = 1;
};

// ===== 渲染通道参数 =====
struct RenderPassParams {
    float       clearColor[4]  = {0, 0, 0, 1};
    float       clearDepth     = 1.0f;
    uint8_t     clearStencil   = 0;
    bool        clearColorFlag = true;
    bool        clearDepthFlag = true;
    bool        clearStencilFlag = true;    // 默认清除 stencil（与 depth 保持一致）
    struct { uint32_t x, y, w, h; } viewport = {};
};

// ===== Stencil 操作 =====
enum class StencilOp : uint8_t {
    Keep,           // 保持当前值
    Zero,           // 置零
    Replace,        // 替换为参考值
    IncrClamp,      // 自增（饱和）
    DecrClamp,      // 自减（饱和）
    Invert,         // 按位取反
    IncrWrap,       // 自增（回绕）
    DecrWrap,       // 自减（回绕）
};

struct StencilState {
    CompareOp   func        = CompareOp::Always;   // 比较函数
    StencilOp   stencilFail = StencilOp::Keep;     // stencil 测试失败
    StencilOp   depthFail   = StencilOp::Keep;     // stencil 通过但 depth 失败
    StencilOp   depthPass   = StencilOp::Keep;     // 两者都通过
    uint8_t     readMask    = 0xFF;
    uint8_t     writeMask   = 0xFF;
    uint8_t     ref         = 0;                    // 参考值
};

// ===== 管线状态 =====
struct PipelineState {
    ProgramHandle   program;
    PrimitiveType   primitiveType = PrimitiveType::Triangles;

    // 光栅化
    CullMode        cullMode    = CullMode::Back;
    PolygonMode     polygonMode = PolygonMode::Fill;
    bool            depthTest = true;
    bool            depthWrite = true;
    CompareOp       depthFunc = CompareOp::Less;
    bool            multisample = false;

    // 模板测试
    bool            stencilEnable = false;
    StencilState    stencilFront;   // 正面的 stencil 状态
    StencilState    stencilBack;    // 背面的 stencil 状态（可以和正面不同）

    // 混合
    bool            blendEnable = false;
    BlendFactor     srcColorBlend = BlendFactor::One;
    BlendFactor     dstColorBlend = BlendFactor::Zero;
    BlendFactor     srcAlphaBlend = BlendFactor::One;
    BlendFactor     dstAlphaBlend = BlendFactor::Zero;
};

// ===== 数据描述 =====
struct BufferDataDesc {
    const void* data   = nullptr;
    uint32_t    size   = 0;
    uint32_t    offset = 0;
};

} // namespace rhi
} // namespace engine