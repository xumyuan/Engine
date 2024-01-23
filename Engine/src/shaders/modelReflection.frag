#version 450 core

in vec3 FragPos;
in vec3 Normal;

out vec4 colour;

uniform vec3 cameraPos;
uniform samplerCube environmentMap;

void main() {
	vec3 viewDir = normalize(FragPos - cameraPos);
	vec3 reflectedDir = reflect(viewDir, normalize(Normal));

	colour = texture(environmentMap, reflectedDir);
}