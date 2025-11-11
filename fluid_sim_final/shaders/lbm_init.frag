#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;  // f0, f1, f2, f3
layout(location = 1) out vec4 distOut1;  // f4, f5, f6, f7
layout(location = 2) out float distOut2; // f8

// D2Q9 weights
const float w[9] = float[9](
    1.0/36.0, 1.0/9.0, 1.0/36.0,
    1.0/9.0, 4.0/9.0, 1.0/9.0,
    1.0/36.0, 1.0/9.0, 1.0/36.0
);

// D2Q9 lattice velocities
const ivec2 e[9] = ivec2[9](
    ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1),
    ivec2(-1, 0), ivec2(0, 0), ivec2(1, 0),
    ivec2(-1,-1), ivec2(0,-1), ivec2(1,-1)
);

float equilibrium(int i, float rho, vec2 u) {
    float eu = float(e[i].x) * u.x + float(e[i].y) * u.y;
    float u2 = u.x * u.x + u.y * u.y;
    return w[i] * rho * (1.0 + 3.0 * eu + 4.5 * eu * eu - 1.5 * u2);
}

void main() {
    // Initialize with SMALLER density perturbation
    float rho = 1.0;
    vec2 u = vec2(0.0, 0.0);
    
    // Much gentler density blob
    vec2 center = vec2(0.5, 0.5);
    float dist = length(texCoord - center);
    if (dist < 0.15) {
        // Smoother gaussian blob
        rho = 1.0 + 0.1 * exp(-dist*dist * 50.0);
    }
    
    // Initialize to equilibrium
    distOut0.x = equilibrium(0, rho, u);
    distOut0.y = equilibrium(1, rho, u);
    distOut0.z = equilibrium(2, rho, u);
    distOut0.w = equilibrium(3, rho, u);
    
    distOut1.x = equilibrium(4, rho, u);
    distOut1.y = equilibrium(5, rho, u);
    distOut1.z = equilibrium(6, rho, u);
    distOut1.w = equilibrium(7, rho, u);
    
    distOut2 = equilibrium(8, rho, u);
}
