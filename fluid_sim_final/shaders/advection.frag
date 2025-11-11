#version 330 core
in vec2 texCoord;
out float densityOut;

uniform sampler2D densityTex;
uniform sampler2D velocityTex;
uniform float dt;
uniform vec2 texelSize;

void main() {
    // Get velocity at this point
    vec2 vel = texture(velocityTex, texCoord).xy;
    
    // Convert velocity from pixels/frame to texture coordinates
    vec2 velTC = vel * texelSize * dt;
    
    // Semi-Lagrangian advection: trace backwards
    vec2 srcPos = texCoord - velTC;
    
    // Clamp to boundaries (no wrap)
    srcPos = clamp(srcPos, vec2(0.001), vec2(0.999));
    
    // Sample density from source position
    densityOut = texture(densityTex, srcPos).r;
    
    // Optional: tiny diffusion to prevent artifacts
    densityOut = densityOut * 0.9995 + 1.0 * 0.0005;
}
