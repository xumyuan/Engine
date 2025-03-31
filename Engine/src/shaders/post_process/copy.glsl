#shader-type vertex

#version 450 core

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;

out vec2 TexCoords;

void main() {
	gl_Position = vec4(position, 1.0);
	TexCoords = texCoord;
}

#shader-type fragment
#version 450 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D input_texture;

void main() {
	FragColor = texture(input_texture, TexCoords);
}