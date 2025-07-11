#version 450 core

layout(std430, binding = 0) buffer PositionBuffer {
    vec4 positions[];
};

layout(std430, binding = 1) buffer VelocityBuffer {
    vec4 velocities[];
};

out vec4 particleVelocity;
out float particleSpeed;
out float particleSize;

uniform mat4 modelViewProjection;
uniform vec3 cameraPos;
uniform float worldSize;
uniform bool pointSizeEnabled;
uniform int totalParticles;
uniform float baseParticleSize;

float calculateParticleSize() {
    if (!pointSizeEnabled) {
        return baseParticleSize;
    }
    
    // Distance-based sizing
    vec3 particlePos = positions[gl_VertexID].xyz;
    float distance = length(cameraPos - particlePos);
    float size = baseParticleSize * worldSize / max(distance, 1.0);
    
    // Scale based on particle count for performance
    if (totalParticles > 1000) {
        size = max(size, 1.0);
    } else {
        size = max(size, 3.0);
    }
    
    return size;
}

void main() {
    vec4 position = positions[gl_VertexID];
    vec4 velocity = velocities[gl_VertexID];
    
    gl_Position = modelViewProjection * position;
    gl_PointSize = calculateParticleSize();
    
    particleVelocity = velocity;
    particleSpeed = length(velocity.xyz);
    particleSize = gl_PointSize;
}
