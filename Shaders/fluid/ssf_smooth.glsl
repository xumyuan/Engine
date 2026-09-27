#shader-type vertex
#version 450 core

// 屏幕空间流体第 3 步：可分离双边滤波平滑深度，由 C++ 按水平 / 竖直方向交替调用

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;

void main() {
	gl_Position = vec4(position, 1.0);
}

#shader-type fragment
#version 450 core

layout (std140, binding = 0) uniform PerFrame {
	mat4 view;
	mat4 projection;
	mat4 viewInverse;
	mat4 projectionInverse;
	vec4 viewPos;
	vec2 screenSize;
	vec2 texelSize;
};

layout (std140, binding = 4) uniform SSFParams {
	vec4 particle;
	vec4 blur;        // xy = 纹素步长方向 (1,0) 或 (0,1)，z = 世界空间滤波半径，w = 深度差衰减
	vec4 absorption;
	vec4 shading;     // z = 最大滤波半径（像素）
};

// 0 表示该像素没有流体
layout(binding = 0) uniform sampler2D depthInput;

layout(location = 0) out float smoothedDepth;

void main() {
	ivec2 center = ivec2(gl_FragCoord.xy);
	float centerDepth = texelFetch(depthInput, center, 0).r;
	if (centerDepth <= 0.0) {
		smoothedDepth = 0.0;
		return;
	}

	// 世界空间半径投影到屏幕：远处的流体用更小的像素半径
	float radiusPx = clamp(blur.z * 0.5 * screenSize.y * projection[1][1] / centerDepth, 1.0, shading.z);
	int radius = int(ceil(radiusPx));
	float sigma = max(radiusPx * 0.5, 0.5);
	float invTwoSigma2 = 1.0 / (2.0 * sigma * sigma);

	ivec2 stepDir = ivec2(blur.xy);
	ivec2 maxCoord = ivec2(screenSize) - 1;

	float sum = 0.0;
	float weightSum = 0.0;
	for (int i = -radius; i <= radius; ++i) {
		ivec2 coord = clamp(center + stepDir * i, ivec2(0), maxCoord);
		float sampleDepth = texelFetch(depthInput, coord, 0).r;
		if (sampleDepth <= 0.0) continue;

		float spatial = exp(-float(i * i) * invTwoSigma2);
		float delta = (sampleDepth - centerDepth) * blur.w;
		float range = exp(-delta * delta);
		float w = spatial * range;
		sum += sampleDepth * w;
		weightSum += w;
	}

	smoothedDepth = sum / weightSum;
}
