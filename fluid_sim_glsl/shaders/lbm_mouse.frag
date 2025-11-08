#version 330 core

in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D densityTex;
uniform sampler2D velocityTex;

uniform vec2 mousePos;
uniform vec2 prevMousePos;
uniform float forceStrength;
uniform float forceRadius;
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

float equilibrium(int i, float rho, vec2 u) {
    float eu = float(e[i].x) * u.x + float(e[i].y) * u.y;
    float u2 = u.x * u.x + u.y * u.y;
    return w[i] * rho * (1.0 + 3.0 * eu + 4.5 * eu * eu - 1.5 * u2);
}

void main() {
    vec2 pos = texCoord * gridSize;
    vec4 f0 = texture(distTex0, texCoord);
    vec4 f1 = texture(distTex1, texCoord);
    
    if (mousePos.x >= 0.0 && prevMousePos.x >= 0.0) {
        // Calculate distance to mouse
        vec2 delta = pos - mousePos;
        float dist2 = dot(delta, delta);
        
        if (dist2 < forceRadius * forceRadius && dist2 > 0.25) {
            float rho = texture(densityTex, texCoord).r;
            vec2 vel = texture(velocityTex, texCoord).rg;
            
            // Apply Gaussian-weighted attractive force
            float sigma = forceRadius / 2.0;
            float att = forceStrength * exp(-dist2 / (2.0 * sigma * sigma));
            vec2 force = att * delta / sqrt(dist2);
            
            // Add force to velocity
            vel += force;
            
            // Clamp velocity magnitude
            float maxVel = 0.25;
            float velMag = length(vel);
            if (velMag > maxVel) {
                vel = vel * maxVel / velMag;
            }
            
            // Reset distributions to equilibrium with new velocity
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
    } else {
        distOut0 = f0;
        distOut1 = f1;
    }
}

