#version 450 core

layout (location = 0) in vec3 position;

out vec3 SampleDirection;

uniform mat4 view;
uniform mat4 projection;

void main() {
	SampleDirection = position;

	// 切断视图矩阵的平移部分（因此立方体不会平移，因为我们希望立方体围绕探针的原点）
	gl_Position = projection * mat4(mat3(view)) * vec4(position, 1.0f); 
}