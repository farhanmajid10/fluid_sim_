#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;  
layout(location = 1) out vec4 distOut1;  
layout(location = 2) out float distOut2;

uniform float tau = 1.0;

void main() {
    // D2Q9 weights for equilibrium at rest (u=0)
    float w0 = 1.0/36.0;  // corners
    float w1 = 1.0/9.0;   // edges
    float w4 = 4.0/9.0;   // center
    
    // Initial density
    float rho = 1.0;
    
    // Add a blob in center for testing
    vec2 center = vec2(0.5, 0.5);
    float dist = length(texCoord - center);
    if (dist < 0.1) {
        rho = 1.2;  // 20% higher density
    }
    
    // Equilibrium distributions for u=0
    distOut0 = vec4(
        w0 * rho,  // f0 (-1,1)
        w1 * rho,  // f1 (0,1)
        w0 * rho,  // f2 (1,1)
        w1 * rho   // f3 (-1,0)
    );
    
    distOut1 = vec4(
        w4 * rho,  // f4 (0,0) - center, largest weight
        w1 * rho,  // f5 (1,0)
        w0 * rho,  // f6 (-1,-1)
        w1 * rho   // f7 (0,-1)
    );
    
    distOut2 = w0 * rho;  // f8 (1,-1)
}
