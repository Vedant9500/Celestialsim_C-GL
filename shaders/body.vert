#version 430 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aInstancePos;
layout (location = 2) in float aInstanceRadius;
layout (location = 3) in vec3 aInstanceColor;
layout (location = 4) in float aInstanceSelected;

uniform mat4 uProjection;
uniform mat4 uView;
uniform float uZoom;

out vec3 fragColor;
out float fragSelected;
out vec2 localPos;

void main() {
    // Scale vertex by instance radius
    vec2 scaledPos = aPos * aInstanceRadius;
    
    // Transform to world position
    vec2 worldPos = aInstancePos + scaledPos;
    
    // Transform to clip space
    gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
    
    // Pass data to fragment shader
    fragColor = aInstanceColor;
    fragSelected = aInstanceSelected;
    localPos = aPos; // Local position within circle (-1 to 1)
}
