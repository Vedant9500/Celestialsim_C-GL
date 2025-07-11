#version 430 core

in vec3 fragColor;
in float fragSelected;
in vec2 localPos;

out vec4 FragColor;

void main() {
    // Calculate distance from center
    float dist = length(localPos);
    
    // Discard fragments outside circle
    if (dist > 1.0) {
        discard;
    }
    
    // Create smooth circle with anti-aliasing
    float alpha = 1.0 - smoothstep(0.9, 1.0, dist);
    
    // Base color
    vec3 color = fragColor;
    
    // Add glow effect for selected bodies
    if (fragSelected > 0.5) {
        color = mix(color, vec3(1.0, 1.0, 0.0), 0.3);
        alpha = max(alpha, 0.3 * (1.0 - smoothstep(0.8, 1.2, dist)));
    }
    
    // Add subtle gradient
    float gradient = 1.0 - dist * 0.3;
    color *= gradient;
    
    FragColor = vec4(color, alpha);
}
