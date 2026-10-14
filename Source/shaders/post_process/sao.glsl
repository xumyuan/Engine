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

layout(binding = 1) uniform sampler2D gDepth;

#include "../common/depth_utils.glsl"

// 与 UBOSAOParams 的 std140 布局一致
layout (std140, binding = 4) uniform SAOParams {
	vec4 sampleCount; // x = N, y = 1/(N-0.5), z = spiralTurns, w = invRadiusSquared
	vec2 angleIncCosSin;
	float minHorizonAngleSineSquared;
	float bias;
	float peak2;
	float projectionScaleRadius;
	float intensity;
	float power;
};

const float kPi = 3.14159265359;

void main()
{
    float depth = texture(gDepth, TexCoords).r;
    if (depth >= 1.0)
    {
        FragColor = 1.0;
        return;
    }

    vec3 origin = reconstructViewPosition(TexCoords, depth);
    vec3 normal = reconstructViewSpaceNormalHighQ(gDepth, TexCoords, depth, origin);

    float noise = interleavedGradientNoise(gl_FragCoord.xy);
    float angle = (2.0 * kPi * 2.4) * noise;
    vec2 tapPosition = vec2(cos(angle), sin(angle));
    mat2 angleStep = mat2(angleIncCosSin.x, angleIncCosSin.y,
            -angleIncCosSin.y, angleIncCosSin.x);

    // 单位球在该深度处的屏幕像素半径。视图空间 z 为负。
    float ssDiskRadius = -(projectionScaleRadius / origin.z);

    float occlusion = 0.0;
    int sampleCountN = int(sampleCount.x + 0.5);
    for (int i = 0; i < sampleCountN; ++i)
    {
        float radius = (float(i) + noise + 0.5) * sampleCount.y;
        vec3 tap = vec3(tapPosition, radius * radius);

        float ssRadius = max(1.0, tap.z * ssDiskRadius);
        vec2 uvSamplePos = TexCoords + (ssRadius * tap.xy) * texelSize;

        float sampleDepth = texture(gDepth, uvSamplePos).r;
        vec3 samplePos = reconstructViewPosition(uvSamplePos, sampleDepth);

        vec3 v = samplePos - origin;
        float vv = dot(v, v);
        float vn = dot(v, normal);

        float w = max(0.0, 1.0 - vv * sampleCount.w);
        w = w * w;
        w *= step(vv * minHorizonAngleSineSquared, vn * vn);

        float sampleOcclusion = max(0.0, vn + origin.z * bias) / (vv + peak2);
        occlusion += w * sampleOcclusion;

        tapPosition = angleStep * tapPosition;
    }

    // 参考实现输出的是遮蔽量，光照乘的是可见度。
    float obscurance = sqrt(max(occlusion * intensity, 0.0));
    FragColor = pow(clamp(1.0 - obscurance, 0.0, 1.0), power);
}
