#ifndef ENGINE_FLUID_SSF_COMMON_GLSL
#define ENGINE_FLUID_SSF_COMMON_GLSL

#include "../common/depth_utils.glsl"

layout (std140, binding = 4) uniform SSFParams {
	vec4 particle;    // x = 粒子半径（世界单位），y = 厚度缩放
	vec4 blur;        // xy = 纹素步长方向 (1,0) 或 (0,1)，z = 世界空间滤波半径，w = 深度差衰减
	vec4 absorption;  // rgb = 吸收系数，w = 折射偏移强度
	vec4 shading;     // x = F0，y = 高光指数，z = 最大滤波半径（像素）
};

#endif
