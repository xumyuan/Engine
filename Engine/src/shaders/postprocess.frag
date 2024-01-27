#version 450 core

out vec4 FragColor;

in vec2 TexCoord;

struct Material {
	sampler2D texture_diffuse;
};

uniform Material material;

uniform vec2 readOffset;

void main() {
	// 后处理卷积核
	vec2 readOffsets[9] = vec2[] (
		vec2(-readOffset.x, readOffset.y),
		vec2(0.0, readOffset.y),
		vec2(readOffset.x, readOffset.y),
		vec2(-readOffset.x, 0.0),
		vec2(0.0, 0.0),
		vec2(readOffset.x, 0.0),
		vec2(-readOffset.x, -readOffset.y),
		vec2(0.0, -readOffset.y),
		vec2(readOffset.x, -readOffset.y)
	);

	// 模糊卷积核
	//float kernel[9] = float[] (
	//	1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
	//	2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
	//	1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
	//);

	// 锐化卷积核
	//float kernel[9] = float[] (
	//	-1, -1, -1,
	//	-1,  9, -1,
	//	-1, -1, -1
	//);

	// 边缘高亮
	//float kernel[9] = float[] (
	//	-2, -2, -2,
	//	-2, 15, -2,
	//	-2, -2, -2
	//);

	// 无处理
	float kernel[9] = float[] (
		0, 0, 0,
		0, 1, 0,
		0, 0, 0
	);

	vec3 colour = vec3(0.0);

	// Apply the kernel
	for(int i = 0; i < 9; ++i) {
		colour += texture(material.texture_diffuse, TexCoord + readOffsets[i]).rgb * kernel[i];
	}

	FragColor = vec4(colour, 1.0);
}