 #shader-type vertex

 #version 450 core

layout (location = 0) in vec3 position;

layout (std140, binding = 1) uniform PerObject {
	mat4 model;
	mat3 normalMatrix;  // std140: 3 x vec4
};

layout (std140, binding = 4) uniform ShadowmapParams {
	mat4 lightSpaceViewProjectionMatrix;
};

void main() {
	gl_Position = lightSpaceViewProjectionMatrix * model * vec4(position, 1.0f);
}


#shader-type fragment
#version 450 core

void main() {}
