#shader-type vertex
#version 450 core

// 屏幕空间流体第 2 步：加法混合累加每个像素上的流体厚度

layout(location = 0) in vec3 position;

#include "ssf_common.glsl"

out vec3 eyeCenter;

void main() {
	vec4 eyePos = view * vec4(position, 1.0);
	eyeCenter = eyePos.xyz;
	gl_PointSize = particle.x * screenSize.y * projection[1][1] / -eyePos.z;
	gl_Position = projection * eyePos;
}

#shader-type fragment
#version 450 core

#include "ssf_common.glsl"

layout(binding = 0) uniform sampler2D sceneDepth;

in vec3 eyeCenter;

layout(location = 0) out float thickness;

void main() {
	vec2 p = gl_PointCoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float r2 = dot(p, p);
	if (r2 > 1.0) discard;

	float nz = sqrt(1.0 - r2);
	float frontDistance = -(eyeCenter.z + nz * particle.x);
	float sceneDistance = linearizeDepth(texelFetch(sceneDepth, ivec2(gl_FragCoord.xy), 0).r);
	if (frontDistance > sceneDistance) discard;

	// 视线穿过球体的弦长
	thickness = 2.0 * nz * particle.x * particle.y;
}
