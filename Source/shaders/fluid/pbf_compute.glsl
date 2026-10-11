#shader-type compute
#version 430 core

layout(local_size_x = 128) in;
layout(std140, binding = 0) uniform SimulationParams {
    uvec4 counts;
    uvec4 grid;
    vec4 boundaryMin;
    vec4 boundaryMax;
    vec4 gravityDt;
    vec4 fluid;
};

// Scalar packing preserves the existing 12-byte Float3 vertex layout.
layout(std430, binding = 0) buffer Positions { float positions[]; };
layout(std430, binding = 1) buffer Predicted { vec4 predicted[]; };
layout(std430, binding = 2) buffer Velocities { vec4 velocities[]; };
layout(std430, binding = 3) buffer Scratch { vec4 scratch[]; };
layout(std430, binding = 4) buffer Lambdas { float lambdas[]; };
layout(std430, binding = 5) buffer Heads { int heads[]; };
layout(std430, binding = 6) buffer Links { int links[]; };

const float PI = 3.141592653589793;

vec3 loadPosition(uint i) {
    return vec3(positions[3u * i], positions[3u * i + 1u], positions[3u * i + 2u]);
}

vec3 confine(vec3 p) {
    return clamp(p, boundaryMin.xyz + fluid.w, boundaryMax.xyz - fluid.w);
}

ivec3 cellOf(vec3 p) {
    return clamp(ivec3(floor((p - boundaryMin.xyz) / fluid.x)), ivec3(0), ivec3(grid.xyz) - 1);
}

uint cellIndex(ivec3 cell) {
    return uint(cell.x) + grid.x * (uint(cell.y) + grid.y * uint(cell.z));
}

float poly6(vec3 r) {
    float h = fluid.x;
    float q = max(0.0, 1.0 - dot(r, r) / (h * h));
    return 315.0 / (64.0 * PI * h * h * h) * q * q * q;
}

vec3 spikyGradient(vec3 r) {
    float distance = length(r);
    float h = fluid.x;
    if (distance < 1e-6 || distance >= h) return vec3(0.0);
    float q = 1.0 - distance / h;
    return -45.0 / (PI * h * h * h * h) * q * q * r / distance;
}

void main() {
    uint i = gl_GlobalInvocationID.x;
    uint phase = counts.y;
    if (phase == 1u) {
        if (i < counts.z) heads[i] = -1;
        return;
    }
    if (i >= counts.x) return;

    if (phase == 0u) {
        vec3 v = velocities[i].xyz + gravityDt.xyz * gravityDt.w;
        predicted[i] = vec4(confine(loadPosition(i) + v * gravityDt.w), 0.0);
        return;
    }
    vec3 p = predicted[i].xyz;
    if (phase == 2u) {
        // No capacity limit per cell; a separate dispatch consumes the completed lists.
        links[i] = atomicExchange(heads[cellIndex(cellOf(p))], int(i));
        return;
    }
    if (phase == 5u) {
        predicted[i] = vec4(confine(p + scratch[i].xyz), 0.0);
        return;
    }
    if (phase == 6u) {
        velocities[i] = vec4((p - loadPosition(i)) / gravityDt.w, 0.0);
        return;
    }
    if (phase == 8u) {
        positions[3u * i] = p.x;
        positions[3u * i + 1u] = p.y;
        positions[3u * i + 2u] = p.z;
        velocities[i] = scratch[i];
        return;
    }

    float density = poly6(vec3(0.0));
    float gradientSum = 0.0;
    vec3 gradientI = vec3(0.0);
    vec3 correction = vec3(0.0);
    float referenceWeight = poly6(vec3(0.1 * fluid.x, 0.0, 0.0));
    ivec3 cell = cellOf(p);
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                ivec3 neighbor = cell + ivec3(x, y, z);
                if (any(lessThan(neighbor, ivec3(0))) || any(greaterThanEqual(neighbor, ivec3(grid.xyz)))) continue;
                for (int j = heads[cellIndex(neighbor)]; j != -1; j = links[j]) {
                    if (uint(j) == i) continue;
                    vec3 r = p - predicted[j].xyz;
                    if (dot(r, r) >= fluid.x * fluid.x) continue;
                    float weight = poly6(r);
                    if (phase == 3u) {
                        density += weight;
                        vec3 g = spikyGradient(r) * fluid.y;
                        gradientI -= g;
                        gradientSum += dot(g, g);
                    } else if (phase == 4u) {
                        float ratio = weight / referenceWeight;
                        float pressure = -0.1 * ratio * ratio * ratio * ratio;
                        correction += (lambdas[i] + lambdas[j] + pressure) * spikyGradient(r);
                    } else if (phase == 7u) {
                        correction += weight * (velocities[j].xyz - velocities[i].xyz);
                    }
                }
            }
        }
    }
    if (phase == 3u) {
        float constraint = density * fluid.y - 1.0;
        lambdas[i] = -constraint / (dot(gradientI, gradientI) + gradientSum + 1000.0);
    } else if (phase == 4u) {
        scratch[i] = vec4(correction * fluid.y, 0.0);
    } else if (phase == 7u) {
        scratch[i] = vec4(velocities[i].xyz + correction * fluid.z * fluid.y, 0.0);
    }
}
