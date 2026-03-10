#pragma once

namespace engine {
namespace rhi {
// backend
enum class Backend : uint8_t {
  Null = 0,
  OpenGL,
  Vulkan,
  Metal,
  D3D12,
};

// texture
enum class TextureFormat : uint16_t {
  R8,
  RG8,
  RGB8,
  RGBA8,
  SRGB8_A8,
  R16F,
  RG16F,
  RGBA16F,
  R32F,
  RG32F,
  RGBA32F,
  Depth16,
  Depth24,
  Depth32F,
  Depth24Stencil8,
  Depth32FStencil8,
};

enum class TextureType : uint8_t {
  Texture2D,
  Texture2DArray,
  Texture3D,
  TextureCube,
  TextureCubeArray,
};

enum class TextureUsage : uint16_t {
  None = 0x0000,
  Sampled = 0x0001,         // 可被 shader 采样
  ColorAttachment = 0x0002, // 颜色附件
  DepthAttachment = 0x0004, // 深度附件
  Storage = 0x0008,         // 存储纹理（compute）
  TransferSrc = 0x0010,     // 拷贝源
  TransferDst = 0x0020,     // 拷贝目标
  Default = Sampled | TransferDst,
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
  return TextureUsage(uint16_t(a) | uint16_t(b));
}
inline bool operator&(TextureUsage a, TextureUsage b) {
  return (uint16_t(a) & uint16_t(b)) != 0;
}

// ===== 缓冲区相关 =====
enum class BufferUsage : uint8_t {
  Vertex,
  Index,
  Uniform,
  Storage,
  Staging,
};

enum class IndexType : uint8_t {
  UInt16,
  UInt32,
};

// ===== 图元类型 =====
enum class PrimitiveType : uint8_t {
  Triangles,
  TriangleStrip,
  Lines,
  LineStrip,
  Points,
};

// ===== 着色器 =====
enum class ShaderStage : uint8_t {
  Vertex,
  Fragment,
  Geometry,
  TessControl,
  TessEvaluation,
  Compute,
};

// ===== 混合/深度/光栅状态 =====
enum class BlendFactor : uint8_t {
  Zero,
  One,
  SrcColor,
  OneMinusSrcColor,
  SrcAlpha,
  OneMinusSrcAlpha,
  DstColor,
  OneMinusDstColor,
  DstAlpha,
  OneMinusDstAlpha,
};

enum class CompareOp : uint8_t {
  Never,
  Less,
  Equal,
  LessEqual,
  Greater,
  NotEqual,
  GreaterEqual,
  Always,
};

enum class CullMode : uint8_t {
  None,
  Front,
  Back,
};

enum class PolygonMode : uint8_t {
  Fill,
  Line,
  Point,
};

// ===== 常量 =====
static constexpr uint32_t MAX_VERTEX_ATTRIBUTES = 16;
static constexpr uint32_t MAX_DESCRIPTOR_SETS = 4;
static constexpr uint32_t MAX_COLOR_ATTACHMENTS = 8;

} // namespace rhi
} // namespace engine