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
    float rho = 1.0;
    vec2 u = vec2(0.0, 0.0);
    
    // Add multiple density perturbations
    float dist1 = length(texCoord - vec2(0.3, 0.5));
    if (dist1 < 0.08) {
        rho += 0.05 * exp(-dist1*dist1 * 100.0);
    }
    
    float dist2 = length(texCoord - vec2(0.7, 0.5));
    if (dist2 < 0.08) {
        rho += 0.05 * exp(-dist2*dist2 * 100.0);
    }
    
    float dist3 = length(texCoord - vec2(0.5, 0.7));
    if (dist3 < 0.08) {
        rho += 0.05 * exp(-dist3*dist3 * 100.0);
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
