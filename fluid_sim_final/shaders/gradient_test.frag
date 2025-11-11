#version 330 core
in vec2 texCoord;
out vec4 FragColor;

void main() {
    // Simple gradient: red=x, green=y, blue=0.5
    FragColor = vec4(texCoord.x, texCoord.y, 0.5, 1.0);
}
