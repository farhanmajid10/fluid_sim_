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
    
    // Scale velocity by texel size for proper units
    // (velocity is in pixels/second, need texture coords/second)
    vec2 velTC = vel * texelSize * dt;
    
    // Semi-Lagrangian advection - trace backward
    vec2 pos = texCoord - velTC;
    
    // Wrap around boundaries for continuous flow
    // This prevents blobs from disappearing
    pos = fract(pos);  // Wraps to [0,1]
    
    // Alternative: Clamp to boundaries (comment out fract above)
    // pos = clamp(pos, vec2(0.001), vec2(0.999));
    
    // Sample density from previous position
    densityOut = texture(densityTex, pos).r;
    
    // Optional: Add a tiny bit of diffusion to prevent artifacts
    // densityOut = densityOut * 0.999 + 1.0 * 0.001;
}
