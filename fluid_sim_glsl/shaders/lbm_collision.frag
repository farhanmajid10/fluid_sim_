#version 330 core

in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform float tau;
uniform vec2 gridSize;

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

float compute_density(vec4 f0, vec4 f1) {
    return f0.x + f0.y + f0.z + f0.w + f1.x + f1.y + f1.z + f1.w;
}

vec2 compute_velocity(vec4 f0, vec4 f1, float rho) {
    vec2 vel = vec2(0.0);
    
    // First 4 distributions
    vel += f0.x * vec2(e[0]);
    vel += f0.y * vec2(e[1]);
    vel += f0.z * vec2(e[2]);
    vel += f0.w * vec2(e[3]);
    
    // Next 4 distributions
    vel += f1.x * vec2(e[4]);
    vel += f1.y * vec2(e[5]);
    vel += f1.z * vec2(e[6]);
    vel += f1.w * vec2(e[7]);
    
    return vel / rho;
}

float equilibrium(int i, float rho, vec2 u) {
    float eu = float(e[i].x) * u.x + float(e[i].y) * u.y;
    float u2 = u.x * u.x + u.y * u.y;
    return w[i] * rho * (1.0 + 3.0 * eu + 4.5 * eu * eu - 1.5 * u2);
}

void main() {
    // Read current distributions
    vec4 f0 = texture(distTex0, texCoord);
    vec4 f1 = texture(distTex1, texCoord);
    
    // Compute macroscopic quantities
    float rho = compute_density(f0, f1);
    vec2 u = compute_velocity(f0, f1, rho);
    
    // BGK collision operator
    vec4 newF0, newF1;
    
    newF0.x = f0.x + (equilibrium(0, rho, u) - f0.x) / tau;
    newF0.y = f0.y + (equilibrium(1, rho, u) - f0.y) / tau;
    newF0.z = f0.z + (equilibrium(2, rho, u) - f0.z) / tau;
    newF0.w = f0.w + (equilibrium(3, rho, u) - f0.w) / tau;
    
    newF1.x = f1.x + (equilibrium(4, rho, u) - f1.x) / tau;
    newF1.y = f1.y + (equilibrium(5, rho, u) - f1.y) / tau;
    newF1.z = f1.z + (equilibrium(6, rho, u) - f1.z) / tau;
    newF1.w = f1.w + (equilibrium(7, rho, u) - f1.w) / tau;
    
    distOut0 = newF0;
    distOut1 = newF1;
}

