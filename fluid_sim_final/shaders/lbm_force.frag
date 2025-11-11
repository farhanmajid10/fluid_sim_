#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;
layout(location = 2) out float distOut2;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D distTex2;
uniform vec2 mousePos;
uniform vec2 mouseVel;
uniform float forceRadius;
uniform float forceStrength;

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
    vec4 f0123 = texture(distTex0, texCoord);
    vec4 f4567 = texture(distTex1, texCoord);
    float f8 = texture(distTex2, texCoord).r;
    
    // Default: pass through
    distOut0 = f0123;
    distOut1 = f4567;
    distOut2 = f8;
    
    // Apply force near mouse
    float dist = length(texCoord - mousePos);
    if (dist < forceRadius) {
        // Gentler Gaussian force
        float force = forceStrength * exp(-dist*dist / (forceRadius*forceRadius * 0.1));
        
        // Compute current density
        float rho = f0123.x + f0123.y + f0123.z + f0123.w +
                    f4567.x + f4567.y + f4567.z + f4567.w + f8;
        
        // SUBTLE density perturbation
        rho += force * 0.1;  // Much less
        
        // GENTLE velocity from mouse movement
        vec2 u = mouseVel * force * 0.005;  // Much less
        
        // Reinitialize to new equilibrium
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
}
