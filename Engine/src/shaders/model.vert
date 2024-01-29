#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out mat3 TBN;
out vec3 FragPos;
out vec2 TexCoords;

uniform mat3 normalMatrix;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	vec3 T = normalize(normalMatrix * tangent);
	vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * normal);
    TBN = mat3(T, B, N);

	FragPos = vec3(model * vec4(position, 1.0f));
	TexCoords = texCoords;

	gl_Position = projection * view * model * (vec4(position, 1.0f));
}