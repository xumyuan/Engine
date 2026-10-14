#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;

out vec3 SampleDirection;

#include "common/per_frame.glsl"

void main() {
	SampleDirection = position;

	// 去除视图矩阵平移部分
	gl_Position = projection * mat4(mat3(view)) * vec4(position, 1.0f); 
}

#shader-type fragment
#version 450 core

out vec4 FragColor;

in vec3 SampleDirection;

layout(binding = 0) uniform samplerCube sceneCaptureCubemap;

layout (std140, binding = 4) uniform ProbeParams {
	float roughness;
};

#include "common/ibl_sampling.glsl"

void main(){
	vec3 N = normalize(SampleDirection);
	vec3 R = N;
	vec3 V = R;
	
	const uint SAMPLE_COUNT = 1024u;
	float totalWeight = 0.0;

	vec3 prefilteredColor = vec3(0.0);
	for(uint i = 0; i < SAMPLE_COUNT; ++i){
		vec2 Xi = Hammersley(i, SAMPLE_COUNT);
		// 这里采用重要性采样，获取了更靠近反射方向的向量
		vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
		vec3 L  = normalize(2.0 * dot(V, H) * H - V);
	
		float NdotL = max(dot(N, L), 0.0);
		if(NdotL > 0.0) {
			prefilteredColor += texture(sceneCaptureCubemap, L).rgb * NdotL;
			totalWeight += NdotL;
		}
		
	}

	prefilteredColor = prefilteredColor/totalWeight;

	FragColor = vec4(prefilteredColor,1.0);
}
