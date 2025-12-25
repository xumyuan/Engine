#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;

out vec2 TexCoords;

void main()
{
    TexCoords = texCoord;
    gl_Position = vec4(position, 1.0);
}

#shader-type fragment
#version 450 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D gNormal;
uniform sampler2D gDepth;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform int kernelSize;
uniform float radius;
uniform float bias;
uniform float power;
uniform vec2 screenSize;

uniform mat4 projection;
uniform mat4 projectionInverse;
uniform mat4 view;

// 从深度重建视图空间位置
vec3 reconstructViewPosition(vec2 uv, float depth)
{
    // NDC 坐标
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    // 视图空间坐标
    vec4 viewPos = projectionInverse * clipPos;
    return viewPos.xyz / viewPos.w;
}

void main()
{
    // 噪声纹理平铺
    vec2 noiseScale = screenSize / 4.0;
    
    // 获取深度
    float depth = texture(gDepth, TexCoords).r;
    
    // 跳过天空/远处
    if (depth >= 1.0)
    {
        FragColor = 1.0;
        return;
    }
    
    // 重建视图空间位置
    vec3 fragPos = reconstructViewPosition(TexCoords, depth);
    
    // 获取法线（世界空间 -> 视图空间）
    vec3 normal = texture(gNormal, TexCoords).rgb;
    normal = normalize((view * vec4(normal, 0.0)).xyz);
    
    // 获取随机旋转向量
    vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;
    
    // Gramm-Schmidt 正交化，构建 TBN 矩阵
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // 计算遮蔽
    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i)
    {
        // 获取采样点位置（切线空间 -> 视图空间）
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;
        
        // 投影采样点到屏幕空间
        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        
        // 获取采样点处的深度
        float sampleDepth = texture(gDepth, offset.xy).r;
        vec3 sampleViewPos = reconstructViewPosition(offset.xy, sampleDepth);
        
        // 范围检查和遮蔽计算
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / float(kernelSize));
    FragColor = pow(occlusion, power);
}
