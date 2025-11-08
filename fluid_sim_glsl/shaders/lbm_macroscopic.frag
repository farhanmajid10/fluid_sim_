#version 330 core

in vec2 texCoord;

layout(location = 0) out float densityOut;
layout(location = 1) out vec2 velocityOut;

uniform sampler2D distTex0;
uniform sampler2D distTex1;

// D2Q9 lattice velocities
const ivec2 e[9] = ivec2[9](
    ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1),
    ivec2(-1, 0), ivec2(0, 0), ivec2(1, 0),
    ivec2(-1,-1), ivec2(0,-1), ivec2(1,-1)
);

void main() {
    vec4 f0 = texture(distTex0, texCoord);
    vec4 f1 = texture(distTex1, texCoord);
    
    // Compute density (zeroth moment)
    float rho = f0.x + f0.y + f0.z + f0.w + f1.x + f1.y + f1.z + f1.w;
    
    // Compute velocity (first moment)
    vec2 vel = vec2(0.0);
    vel += f0.x * vec2(e[0]);
    vel += f0.y * vec2(e[1]);
    vel += f0.z * vec2(e[2]);
    vel += f0.w * vec2(e[3]);
    vel += f1.x * vec2(e[4]);
    vel += f1.y * vec2(e[5]);
    vel += f1.z * vec2(e[6]);
    vel += f1.w * vec2(e[7]);
    
    vel /= rho;
    
    densityOut = rho;
    velocityOut = vel;
}

