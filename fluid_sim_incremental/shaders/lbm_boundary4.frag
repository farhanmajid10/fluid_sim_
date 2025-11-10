#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;
layout(location = 2) out float distOut2;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D distTex2;
uniform vec2 gridSize;

void main() {
    vec4 f0123 = texture(distTex0, texCoord);
    vec4 f4567 = texture(distTex1, texCoord);
    float f8 = texture(distTex2, texCoord).r;
    
    // Copy distributions
    distOut0 = f0123;
    distOut1 = f4567;
    distOut2 = f8;
    
    vec2 pixel = texCoord * gridSize;
    
    // Bounce-back at walls
    float margin = 2.0;
    
    // Left wall
    if (pixel.x < margin) {
        distOut0.z = f0123.x;  // f2 = f0 (swap east-west)
        distOut1.y = f0123.w;  // f5 = f3
        distOut1.w = f4567.z;  // f7 = f6
    }
    
    // Right wall
    if (pixel.x > gridSize.x - margin) {
        distOut0.x = f0123.z;  // f0 = f2
        distOut0.w = f4567.y;  // f3 = f5
        distOut1.z = f4567.w;  // f6 = f7
    }
    
    // Bottom wall
    if (pixel.y < margin) {
        distOut0.x = f4567.z;  // f0 = f6
        distOut0.y = f4567.w;  // f1 = f7
        distOut0.z = f8;       // f2 = f8
    }
    
    // Top wall
    if (pixel.y > gridSize.y - margin) {
        distOut1.z = f0123.x;  // f6 = f0
        distOut1.w = f0123.y;  // f7 = f1
        distOut2 = f0123.z;    // f8 = f2
    }
}
