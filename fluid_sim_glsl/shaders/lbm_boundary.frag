#version 330 core

in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform vec2 gridSize;
uniform float wallDamping;

// Bounce-back indices
const int bounce[9] = int[9](8, 7, 6, 5, 4, 3, 2, 1, 0);

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
    vec2 pos = texCoord * gridSize;
    vec4 f0 = texture(distTex0, texCoord);
    vec4 f1 = texture(distTex1, texCoord);
    
    // Check if at boundary
    bool atLeftWall = pos.x < 1.0;
    bool atRightWall = pos.x > gridSize.x - 1.0;
    bool atBottomWall = pos.y < 1.0;
    bool atTopWall = pos.y > gridSize.y - 1.0;
    
    if (atLeftWall || atRightWall || atBottomWall || atTopWall) {
        // Compute density
        float rho = f0.x + f0.y + f0.z + f0.w + f1.x + f1.y + f1.z + f1.w;
        
        // Compute velocity
        vec2 vel = vec2(0.0);
        vel += f0.x * vec2(e[0]);
        vel += f0.y * vec2(e[1]);
        vel += f0.z * vec2(e[2]);
        vel += f0.w * vec2(e[3]);
        vel += f1.x * vec2(e[4]);
        vel += f1.y * vec2(e[5]);
        vel += f1.z * vec2(e[6]);
        vel += f1.w * vec2(e[7]);
        vel = vel / rho;
        
        // Apply wall damping
        vel *= wallDamping;
        
        // Reset to equilibrium with damped velocity
        distOut0.x = equilibrium(0, rho, vel);
        distOut0.y = equilibrium(1, rho, vel);
        distOut0.z = equilibrium(2, rho, vel);
        distOut0.w = equilibrium(3, rho, vel);
        distOut1.x = equilibrium(4, rho, vel);
        distOut1.y = equilibrium(5, rho, vel);
        distOut1.z = equilibrium(6, rho, vel);
        distOut1.w = equilibrium(7, rho, vel);
    } else {
        distOut0 = f0;
        distOut1 = f1;
    }
}

