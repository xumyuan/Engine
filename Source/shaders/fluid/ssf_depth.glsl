#shader-type vertex
#version 450 core

// 屏幕空间流体第 1 步：把粒子当作球体渲染，输出最近流体表面到相机的线性距离

layout(location = 0) in vec3 position;

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
	vec4 particle;    // x = 粒子半径（世界单位），y = 厚度缩放
	vec4 blur;
	vec4 absorption;
	vec4 shading;
};

out vec3 eyeCenter;

void main() {
	vec4 eyePos = view * vec4(position, 1.0);
	eyeCenter = eyePos.xyz;
	// 球投影到屏幕上的直径（像素）= 2r * (H / 2) * projection[1][1] / 距离
	gl_PointSize = particle.x * screenSize.y * projection[1][1] / -eyePos.z;
	gl_Position = projection * eyePos;
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
	vec4 blur;
	vec4 absorption;
	vec4 shading;
};

// 场景深度（G-buffer），与本 pass 分辨率一致；见 TextureBindings.h 的 SSFSceneDepth
layout(binding = 0) uniform sampler2D sceneDepth;

in vec3 eyeCenter;

layout(location = 0) out float fluidDepth;

float linearDepth(float depth) {
	float ndcZ = depth * 2.0 - 1.0;
	return projection[3][2] / (ndcZ + projection[2][2]);
}

void main() {
	vec2 p = gl_PointCoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float r2 = dot(p, p);
	if (r2 > 1.0) discard;

	vec3 normal = vec3(p, sqrt(1.0 - r2));
	vec3 surface = eyeCenter + normal * particle.x;
	float distance = -surface.z;

	float sceneDistance = linearDepth(texelFetch(sceneDepth, ivec2(gl_FragCoord.xy), 0).r);
	if (distance > sceneDistance) discard;

	vec4 clipPos = projection * vec4(surface, 1.0);
	gl_FragDepth = clipPos.z / clipPos.w * 0.5 + 0.5;
	fluidDepth = distance;
}
