#version 330 core
in vec2 texCoord;

layout(location = 0) out float densityOut;
layout(location = 1) out vec2 velocityOut;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D distTex2;

const ivec2 e[9] = ivec2[9](
    ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1),
    ivec2(-1, 0), ivec2(0, 0), ivec2(1, 0),
    ivec2(-1,-1), ivec2(0,-1), ivec2(1,-1)
);

void main() {
    vec4 f0123 = texture(distTex0, texCoord);
    vec4 f4567 = texture(distTex1, texCoord);
    float f8 = texture(distTex2, texCoord).r;
    
    // Compute density
    float rho = f0123.x + f0123.y + f0123.z + f0123.w +
                f4567.x + f4567.y + f4567.z + f4567.w + f8;
    
    // Compute velocity
    vec2 vel = vec2(0.0);
    vel += f0123.x * vec2(e[0]);
    vel += f0123.y * vec2(e[1]);
    vel += f0123.z * vec2(e[2]);
    vel += f0123.w * vec2(e[3]);
    vel += f4567.x * vec2(e[4]);
    vel += f4567.y * vec2(e[5]);
    vel += f4567.z * vec2(e[6]);
    vel += f4567.w * vec2(e[7]);
    vel += f8 * vec2(e[8]);
    
    vel /= rho;
    
    densityOut = rho;
    velocityOut = vel;
}
