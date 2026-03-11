#shader-type vertex

#version 450 core

layout (location = 0) in vec3 position;

out vec3 SampleDirection;

layout (std140, binding = 0) uniform PerFrame {
	mat4 view;
	mat4 projection;
	mat4 viewInverse;
	mat4 projectionInverse;
	vec4 viewPos;
	vec2 screenSize;
	vec2 texelSize;
};

void main() {
	// 去除视图矩阵的平移部分，保证天空盒包围相机不会移动
	vec4 pos = projection * mat4(mat3(view)) * vec4(position, 1.0f);
	SampleDirection = position; // A skymap can be sampled by its vertex positions (since it is centered around the origin)
	gl_Position = pos.xyww; // 保证深度为1
}



#shader-type fragment
#version 450 core

out vec4 FragColor;

in vec3 SampleDirection;

uniform samplerCube skyboxCubemap;

void main() {
	FragColor = texture(skyboxCubemap, SampleDirection);
}
