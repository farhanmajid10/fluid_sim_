#version 330 core

in vec2 texCoord;

layout(location = 0) out vec4 distOut0;  // Distributions 0-3
layout(location = 1) out vec4 distOut1;  // Distributions 4-7, with 8 in alpha

uniform float tau;

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
    // Initial conditions
    float rho = 1.0;  // Initial density
    vec2 u = vec2(0.0, 0.0);  // Initial velocity
    
    // Calculate equilibrium distributions
    float f[9];
    for (int i = 0; i < 9; i++) {
        f[i] = equilibrium(i, rho, u);
    }
    
    // Pack into output textures
    distOut0 = vec4(f[0], f[1], f[2], f[3]);
    distOut1 = vec4(f[4], f[5], f[6], f[7]);
    // Note: f[8] will need special handling in full implementation
}

