#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;
uniform sampler2D velocityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    vec2 velocity = texture(velocityTex, texCoord).rg;
    
    // Velocity magnitude for visualization
    float velMag = length(velocity) * 50.0;  // Scale for visibility
    
    // Color based on density and velocity
    vec3 color;
    
    if (density < 0.9) {
        // Low density = dark blue
        color = vec3(0.0, 0.0, 0.3);
    } else if (density < 1.0) {
        // Normal low = blue to cyan
        color = mix(vec3(0.0, 0.0, 0.5), vec3(0.0, 0.5, 1.0), (density - 0.9) * 10.0);
    } else if (density < 1.1) {
        // Normal = cyan to green
        color = mix(vec3(0.0, 0.5, 1.0), vec3(0.0, 1.0, 0.5), (density - 1.0) * 10.0);
    } else if (density < 1.2) {
        // High = green to yellow
        color = mix(vec3(0.0, 1.0, 0.5), vec3(1.0, 1.0, 0.0), (density - 1.1) * 10.0);
    } else {
        // Very high = yellow to white
        color = mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 1.0), min(1.0, (density - 1.2) * 5.0));
    }
    
    // Add velocity brightness
    color = mix(color, vec3(1.0), min(0.3, velMag));
    
    FragColor = vec4(color, 1.0);
}
