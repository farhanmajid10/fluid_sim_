#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;  
layout(location = 1) out vec4 distOut1;  
layout(location = 2) out float distOut2;

void main() {
    // HARDCODE exact values for debugging
    distOut0 = vec4(0.0277778, 0.111111, 0.0277778, 0.111111);
    distOut1 = vec4(0.444444, 0.111111, 0.0277778, 0.111111);
    distOut2 = 0.0277778;
}
