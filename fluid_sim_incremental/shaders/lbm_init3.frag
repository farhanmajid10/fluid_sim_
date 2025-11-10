#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;  
layout(location = 1) out vec4 distOut1;  
layout(location = 2) out float distOut2;

void main() {
    // SUPER SIMPLE TEST: Set each distribution to a known value
    // For rho=1.0 and u=0, each distribution should be w[i] * rho
    
    // These are the D2Q9 weights
    float w0 = 1.0/36.0;  // corners
    float w1 = 1.0/9.0;   // edges
    float w4 = 4.0/9.0;   // center
    
    float rho = 1.0;  // Start with uniform density
    
    // Check if we're in a blob
    float dist = length(texCoord - vec2(0.5, 0.5));
    if (dist < 0.1) {
        rho = 1.1;  // Just 10% higher
    }
    
    // Set distributions to equilibrium for zero velocity
    distOut0 = vec4(
        w0 * rho,  // f0 (corner)
        w1 * rho,  // f1 (edge)
        w0 * rho,  // f2 (corner)
        w1 * rho   // f3 (edge)
    );
    
    distOut1 = vec4(
        w4 * rho,  // f4 (center) - this is the biggest
        w1 * rho,  // f5 (edge)
        w0 * rho,  // f6 (corner)
        w1 * rho   // f7 (edge)
    );
    
    distOut2 = w0 * rho;  // f8 (corner)
}
