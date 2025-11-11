#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;
uniform sampler2D velocityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    vec2 velocity = texture(velocityTex, texCoord).rg;
    
    // Velocity magnitude for surface effects
    float velMag = length(velocity) * 30.0;
    
    // Water-like color mapping
    vec3 deepWater = vec3(0.0, 0.15, 0.3);   // Dark blue
    vec3 midWater = vec3(0.0, 0.3, 0.5);     // Medium blue
    vec3 shallowWater = vec3(0.1, 0.5, 0.6); // Cyan-blue
    vec3 foam = vec3(0.8, 0.9, 1.0);         // White-blue foam
    
    vec3 color;
    
    // Map density to water depth appearance
    if (density < 0.98) {
        color = deepWater;
    } else if (density < 1.0) {
        float t = (density - 0.98) / 0.02;
        color = mix(deepWater, midWater, t);
    } else if (density < 1.02) {
        float t = (density - 1.0) / 0.02;
        color = mix(midWater, shallowWater, t);
    } else {
        float t = min(1.0, (density - 1.02) / 0.02);
        color = mix(shallowWater, foam, t);
    }
    
    // Add velocity highlights (ripples/waves)
    color += foam * min(0.3, velMag * 0.5);
    
    // Subtle ambient variation
    color *= (0.95 + 0.05 * sin(texCoord.x * 50.0) * sin(texCoord.y * 50.0));
    
    FragColor = vec4(color, 1.0);
}
