#shader-type vertex
#version 450 core

// 屏幕空间流体第 4 步：由平滑深度重建法线，计算反射 / 折射 / 吸收 / 高光，并与场景颜色合成

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;

out vec2 TexCoords;

void main() {
	gl_Position = vec4(position, 1.0);
	TexCoords = texCoord;
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

struct DirLight {
	vec4 direction;     // xyz = direction, w = intensity
	vec4 lightColour;
};

// 只声明需要的前缀成员；完整布局见 UniformBufferData.h 的 UBOLighting
layout (std140, binding = 2) uniform Lighting {
	ivec4 numDirPointSpotLights;
	DirLight dirLights[3];
};

layout (std140, binding = 4) uniform SSFParams {
	vec4 particle;
	vec4 blur;
	vec4 absorption;  // rgb = 吸收系数，w = 折射偏移强度
	vec4 shading;     // x = F0，y = 高光指数
};

layout(binding = 0) uniform sampler2D sceneColor;
layout(binding = 1) uniform sampler2D fluidDepth;
layout(binding = 2) uniform sampler2D fluidThickness;
layout(binding = 3) uniform samplerCube environmentMap;

in vec2 TexCoords;

out vec4 FragColor;

vec3 eyePosition(ivec2 coord, float depth) {
	vec2 ndc = (vec2(coord) + 0.5) / screenSize * 2.0 - 1.0;
	return vec3(ndc.x * depth / projection[0][0], ndc.y * depth / projection[1][1], -depth);
}

// 取中心两侧深度差较小的一侧做差分，避免跨越流体边缘产生错误法线
vec3 pickDerivative(vec3 center, ivec2 coord, ivec2 offset) {
	ivec2 maxCoord = ivec2(screenSize) - 1;
	ivec2 fwd = clamp(coord + offset, ivec2(0), maxCoord);
	ivec2 bwd = clamp(coord - offset, ivec2(0), maxCoord);
	float dF = texelFetch(fluidDepth, fwd, 0).r;
	float dB = texelFetch(fluidDepth, bwd, 0).r;

	vec3 forward = dF > 0.0 ? eyePosition(fwd, dF) - center : vec3(0.0);
	vec3 backward = dB > 0.0 ? center - eyePosition(bwd, dB) : vec3(0.0);
	if (dF <= 0.0) return backward;
	if (dB <= 0.0) return forward;
	return abs(forward.z) < abs(backward.z) ? forward : backward;
}

void main() {
	ivec2 coord = ivec2(gl_FragCoord.xy);
	vec3 background = texture(sceneColor, TexCoords).rgb;

	float depth = texelFetch(fluidDepth, coord, 0).r;
	if (depth <= 0.0) {
		FragColor = vec4(background, 1.0);
		return;
	}

	vec3 P = eyePosition(coord, depth);
	vec3 ddx = pickDerivative(P, coord, ivec2(1, 0));
	vec3 ddy = pickDerivative(P, coord, ivec2(0, 1));
	vec3 N = cross(ddx, ddy);
	N = dot(N, N) > 1e-12 ? normalize(N) : vec3(0.0, 0.0, 1.0);
	vec3 V = normalize(-P);

	float thickness = texelFetch(fluidThickness, coord, 0).r;

	// 反射：世界空间采样环境贴图
	vec3 worldN = mat3(viewInverse) * N;
	vec3 worldIncident = mat3(viewInverse) * -V;
	vec3 reflection = texture(environmentMap, reflect(worldIncident, worldN)).rgb;

	// 折射：按法线偏移采样场景颜色，厚度越大偏移越明显；吸收遵循 Beer-Lambert
	vec2 refractUV = clamp(TexCoords + N.xy * absorption.w * min(thickness, 1.0), vec2(0.0), vec2(1.0));
	vec3 transmittance = exp(-absorption.rgb * thickness);
	vec3 refraction = texture(sceneColor, refractUV).rgb * transmittance;

	float F0 = shading.x;
	float fresnel = F0 + (1.0 - F0) * pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 5.0);

	vec3 specular = vec3(0.0);
	if (numDirPointSpotLights.x > 0) {
		vec3 L = normalize(mat3(view) * -dirLights[0].direction.xyz);
		vec3 H = normalize(L + V);
		specular = dirLights[0].lightColour.rgb * pow(max(dot(N, H), 0.0), shading.y) * fresnel;
	}

	FragColor = vec4(mix(refraction, reflection, fresnel) + specular, 1.0);
}
