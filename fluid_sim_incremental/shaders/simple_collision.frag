#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;
layout(location = 2) out float distOut2;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D distTex2;

// D2Q9 weights - EXACT values
const float w0 = 0.0277777777;  // 1/36 for corners
const float w1 = 0.1111111111;  // 1/9 for edges  
const float w4 = 0.4444444444;  // 4/9 for center

void main() {
    // Read current distributions
    vec4 f0123 = texture(distTex0, texCoord);
    vec4 f4567 = texture(distTex1, texCoord);
    float f8 = texture(distTex2, texCoord).r;
    
    // Calculate density
    float rho = f0123.x + f0123.y + f0123.z + f0123.w +
                f4567.x + f4567.y + f4567.z + f4567.w + f8;
    
    // For now, velocity is ZERO (no flow)
    vec2 u = vec2(0.0, 0.0);
    
    // Equilibrium for u=0 is just w[i] * rho
    float eq0 = w0 * rho;  // corners
    float eq1 = w1 * rho;  // edges
    float eq4 = w4 * rho;  // center
    
    // BGK collision with tau=1.0 (simplest case)
    float tau = 1.0;
    
    // Output: f_new = f_old + (f_eq - f_old) / tau
    // With tau=1.0 and u=0, this should converge to equilibrium
    distOut0.x = f0123.x + (eq0 - f0123.x) / tau;  // f0
    distOut0.y = f0123.y + (eq1 - f0123.y) / tau;  // f1
    distOut0.z = f0123.z + (eq0 - f0123.z) / tau;  // f2
    distOut0.w = f0123.w + (eq1 - f0123.w) / tau;  // f3
    
    distOut1.x = f4567.x + (eq4 - f4567.x) / tau;  // f4
    distOut1.y = f4567.y + (eq1 - f4567.y) / tau;  // f5
    distOut1.z = f4567.z + (eq0 - f4567.z) / tau;  // f6
    distOut1.w = f4567.w + (eq1 - f4567.w) / tau;  // f7
    
    distOut2 = f8 + (eq0 - f8) / tau;  // f8
}
