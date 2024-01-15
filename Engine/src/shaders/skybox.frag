#version 450 core

out vec4 FragColor;

in vec3 SampleDirection;

uniform samplerCube skyboxCubemap;

void main() {
	FragColor = texture(skyboxCubemap, SampleDirection);
}