#version 450 core

#define FXAA_REDUCE_MIN (1.0 / 128.0)
#define FXAA_REDUCE_MUL (1.0 / 8.0)
#define FXAA_SPAN_MAX 8.0

in vec2 TexCoords;

out vec4 FragColour;

uniform sampler2D input_texture;

uniform vec2 texel_size;

void main() {
	// 采样当前及周围四个点的像素值，并计算亮度值
	vec3 calculateLuma = vec3(0.299, 0.587, 0.114);
	vec3 rgbM  = texture2D(input_texture, TexCoords).xyz;
	vec3 rgbNW = texture2D(input_texture, TexCoords + (vec2(-1.0,-1.0)) * texel_size).xyz;
	vec3 rgbNE = texture2D(input_texture, TexCoords + (vec2(1.0,-1.0)) * texel_size).xyz;
	vec3 rgbSW = texture2D(input_texture, TexCoords + (vec2(-1.0,1.0)) * texel_size).xyz;
	vec3 rgbSE = texture2D(input_texture, TexCoords + (vec2(1.0,1.0)) * texel_size).xyz;

	float lumaM  = dot(rgbM,  calculateLuma);
	float lumaNW = dot(rgbNW, calculateLuma);
	float lumaNE = dot(rgbNE, calculateLuma);
	float lumaSW = dot(rgbSW, calculateLuma);
	float lumaSE = dot(rgbSE, calculateLuma);
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE))); 

	// 计算亮度差距的最大的方向
	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
	//通过对 dir 进行缩放，可以限制抗锯齿处理的范围，防止在一些情况下出现过度模糊或者过度锐化的现象。这是为了平衡抗锯齿效果和图像细节的保留。
	float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

	float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	//将 dir 向量限制在一个范围内，并乘以 texel_size，以将其转换为纹理坐标。
	dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * texel_size;

	// Perform the samples and calculate the new texel colour
	vec3 rgbA = 0.5 * (texture2D(input_texture, TexCoords + dir * ((1.0 / 3.0) - 0.5)).xyz + texture2D(input_texture, TexCoords + dir * ((2.0 / 3.0) - 0.5)).xyz);

	vec3 rgbB = rgbA * 0.5 + 0.25 * (texture2D(input_texture, TexCoords + dir * - 0.5).xyz + texture2D(input_texture, TexCoords + dir * 0.5).xyz);
	float lumaB = dot(rgbB, calculateLuma);
	if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
		FragColour = vec4(rgbA, 1.0);
	} else {
		FragColour = vec4(rgbB, 1.0);
	}
}