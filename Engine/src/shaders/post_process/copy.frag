#version 450 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D input_texture;

void main() {
	FragColor = texture(input_texture, TexCoords);
}