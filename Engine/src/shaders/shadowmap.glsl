 #shader-type vertex

 #version 450 core

layout (location = 0) in vec3 position;

uniform mat4 lightSpaceViewProjectionMatrix;
uniform mat4 model;

void main() {
	gl_Position = lightSpaceViewProjectionMatrix * model * vec4(position, 1.0f);
}


#shader-type fragment
#version 450 core

void main() {}