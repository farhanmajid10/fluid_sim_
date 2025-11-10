#version 330 core
in vec2 texCoord;

layout(location = 0) out float densityOut;
layout(location = 1) out vec2 velocityOut;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D distTex2;

void main() {
    vec4 f0123 = texture(distTex0, texCoord);
    vec4 f4567 = texture(distTex1, texCoord);
    float f8 = texture(distTex2, texCoord).r;
    
    // Sum all distributions for density
    float rho = f0123.x + f0123.y + f0123.z + f0123.w +
                f4567.x + f4567.y + f4567.z + f4567.w + f8;
    
    // For now, velocity is zero (we'll compute it properly later)
    vec2 vel = vec2(0.0, 0.0);
    
    densityOut = rho;
    velocityOut = vel;
}
