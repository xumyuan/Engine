#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;
uniform float time;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

uniform mat3 normalMatrix;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * model * (vec4(position, 1.0f));

	FragPos = vec3(model * vec4(position, 1.0f));
	TexCoords = texCoords;
	// 用发现矩阵来保证法向量的变换正确性
	Normal = normalMatrix * normal;
}