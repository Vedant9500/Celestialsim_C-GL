#version 430 core

layout (location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform mat4 uView;

void main() {
    gl_Position = uProjection * uView * vec4(aPos, 0.0, 1.0);
}
