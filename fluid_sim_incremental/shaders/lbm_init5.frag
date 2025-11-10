#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;  
layout(location = 1) out vec4 distOut1;  
layout(location = 2) out float distOut2;

const float w[9] = float[9](
    1.0/36.0, 1.0/9.0, 1.0/36.0,
    1.0/9.0, 4.0/9.0, 1.0/9.0,
    1.0/36.0, 1.0/9.0, 1.0/36.0
);

void main() {
    // CRITICAL: Initialize to exact equilibrium for rho=1, u=0
    float rho = 1.0;
    
    // For u=0, equilibrium is just w[i] * rho
    distOut0.x = w[0] * rho;  // 1/36
    distOut0.y = w[1] * rho;  // 1/9
    distOut0.z = w[2] * rho;  // 1/36
    distOut0.w = w[3] * rho;  // 1/9
    
    distOut1.x = w[4] * rho;  // 4/9
    distOut1.y = w[5] * rho;  // 1/9
    distOut1.z = w[6] * rho;  // 1/36
    distOut1.w = w[7] * rho;  // 1/9
    
    distOut2 = w[8] * rho;     // 1/36
}
