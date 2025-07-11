#version 450 core

in vec4 particleVelocity;
in float particleSpeed;
in float particleSize;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

uniform bool bloomEnabled;
uniform float bloomThreshold;
uniform float velocityColorScale;

vec3 velocityToColor(vec3 velocity) {
    float speed = length(velocity);
    float normalizedSpeed = speed * velocityColorScale;
    
    // Color based on speed: blue (slow) -> green -> yellow -> red (fast)
    vec3 color;
    if (normalizedSpeed < 0.33) {
        // Blue to cyan
        float t = normalizedSpeed / 0.33;
        color = mix(vec3(0.0, 0.2, 1.0), vec3(0.0, 0.8, 1.0), t);
    } else if (normalizedSpeed < 0.66) {
        // Cyan to yellow
        float t = (normalizedSpeed - 0.33) / 0.33;
        color = mix(vec3(0.0, 0.8, 1.0), vec3(1.0, 1.0, 0.2), t);
    } else {
        // Yellow to red
        float t = min((normalizedSpeed - 0.66) / 0.34, 1.0);
        color = mix(vec3(1.0, 1.0, 0.2), vec3(1.0, 0.2, 0.0), t);
    }
    
    return color;
}

void main() {
    // Create circular particle
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float distance = length(circCoord);
    
    if (distance > 1.0) {
        discard;
    }
    
    // Smooth edges
    float alpha = 1.0 - smoothstep(0.8, 1.0, distance);
    
    // Color based on velocity
    vec3 color = velocityToColor(particleVelocity.xyz);
    
    // Add brightness based on size and speed
    float brightness = 0.5 + 0.5 * (particleSize / 10.0) + 0.3 * min(particleSpeed * 0.1, 1.0);
    color *= brightness;
    
    FragColor = vec4(color, alpha);
    
    // Extract bright parts for bloom
    if (bloomEnabled) {
        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
        if (luminance > bloomThreshold) {
            BrightColor = vec4(color * (luminance - bloomThreshold), alpha);
        } else {
            BrightColor = vec4(0.0, 0.0, 0.0, 0.0);
        }
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
