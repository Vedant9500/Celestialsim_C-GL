#version 430 core

out vec4 FragColor;

void main() {
    // Cyan/light blue with good transparency for velocity vectors
    FragColor = vec4(0.2, 0.8, 1.0, 0.7);
}
