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

out vec2 FragColor;

in vec2 TexCoords;

#include "common/ibl_sampling.glsl"

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
vec2 IntegrateBRDF(float NdotV, float roughness);

void main(){
	vec2 intergratedBRDF = IntegrateBRDF(TexCoords.x,TexCoords.y);
	FragColor = intergratedBRDF;
}

vec2 IntegrateBRDF(float NdotV,float roughness){
	
	vec3 V;
	V.x = sqrt(1.0 - NdotV * NdotV);
	V.y = 0.0;
	V.z = NdotV;

	float a = 0.0;
	float b = 0.0;

	vec3 N =vec3(0.0,0.0,1.0);

	const uint SAMPLE_COUNT = 1024u;
	for(uint i = 0u;i < SAMPLE_COUNT; i++){
		//生成一个偏向于首选对齐方向的样本向量（重要性采样）。
		vec2 Xi = Hammersley(i,SAMPLE_COUNT);
		vec3 H = ImportanceSampleGGX(Xi,N,roughness); //采样方向
		//根据采样方向和观察方向确定的光照方向
		vec3 L = normalize(2.0 * dot(V, H) * H - V); 

		float NdotL = max(L.z, 0.0);
		float NdotH = max(H.z, 0.0);
		float VdotH = max(dot(V, H), 0.0);

		if(NdotL > 0.0)
		{
			float G = GeometrySmith(N, V, L, roughness);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0 - VdotH, 5.0);

			a += (1.0 - Fc) * G_Vis;
			b += Fc * G_Vis;
		}
	}
	a /= float(SAMPLE_COUNT);
	b /= float(SAMPLE_COUNT);
	return vec2(a, b);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	// note that we use a different k for IBL
	float a = roughness;
	float k = (a * a) / 2.0;

	float nom   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}