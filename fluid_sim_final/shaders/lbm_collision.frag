#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;
layout(location = 2) out float distOut2;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D distTex2;
uniform float tau;

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
    // Read distributions
    vec4 f0123 = texture(distTex0, texCoord);
    vec4 f4567 = texture(distTex1, texCoord);
    float f8 = texture(distTex2, texCoord).r;
    
    // Compute density
    float rho = f0123.x + f0123.y + f0123.z + f0123.w +
                f4567.x + f4567.y + f4567.z + f4567.w + f8;
    
    // Compute velocity
    //u = (Σ f_i * e_i) / ρ
    vec2 u = vec2(0.0);
    u += f0123.x * vec2(e[0]);
    u += f0123.y * vec2(e[1]);
    u += f0123.z * vec2(e[2]);
    u += f0123.w * vec2(e[3]);
    u += f4567.x * vec2(e[4]);
    u += f4567.y * vec2(e[5]);
    u += f4567.z * vec2(e[6]);
    u += f4567.w * vec2(e[7]);
    u += f8 * vec2(e[8]);
    u /= rho;
    
    // BGK collision
    //f_i^new = f_i^old + (f_i^eq - f_i^old) / τ
    distOut0.x = f0123.x + (equilibrium(0, rho, u) - f0123.x) / tau;
    distOut0.y = f0123.y + (equilibrium(1, rho, u) - f0123.y) / tau;
    distOut0.z = f0123.z + (equilibrium(2, rho, u) - f0123.z) / tau;
    distOut0.w = f0123.w + (equilibrium(3, rho, u) - f0123.w) / tau;
    
    distOut1.x = f4567.x + (equilibrium(4, rho, u) - f4567.x) / tau;
    distOut1.y = f4567.y + (equilibrium(5, rho, u) - f4567.y) / tau;
    distOut1.z = f4567.z + (equilibrium(6, rho, u) - f4567.z) / tau;
    distOut1.w = f4567.w + (equilibrium(7, rho, u) - f4567.w) / tau;
    
    distOut2 = f8 + (equilibrium(8, rho, u) - f8) / tau;
}
