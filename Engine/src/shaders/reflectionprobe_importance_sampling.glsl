#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;

out vec3 SampleDirection;

uniform mat4 view;
uniform mat4 projection;

void main() {
	SampleDirection = position;

	// 去除视图矩阵平移部分
	gl_Position = projection * mat4(mat3(view)) * vec4(position, 1.0f); 
}

#shader-type fragment
#version 450 core

out vec4 FragColor;

in vec3 SampleDirection;

uniform samplerCube environmentMap;
uniform float roughness;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits);
vec2 Hammersley(uint i, uint N);
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness);

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
			prefilteredColor += texture(environmentMap, L).rgb * NdotL;
			totalWeight += NdotL;
		}
		
	}

	prefilteredColor = prefilteredColor/totalWeight;

	FragColor = vec4(prefilteredColor,1.0);
}

float RadicalInverse_VdC(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness){
	float a = roughness*roughness;

	float phi = 2.0 * PI * Xi.x;
	// 这里粗糙度越小，costheta越接近1 即角度越接近0，采样越集中
	float cosTheta = sqrt((1.0-Xi.y)/(1.0+(a*a-1.0)*Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// 从切线空间转换到世界空间
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}