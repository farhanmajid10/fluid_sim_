#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;
uniform sampler2D velocityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    vec2 velocity = texture(velocityTex, texCoord).rg;
    
    // Velocity magnitude for visualization
    float velMag = length(velocity) * 50.0;
    
    // Color based on density
    vec3 color;
    if (density < 0.98) {
        color = vec3(0.0, 0.0, 0.3);
    } else if (density < 1.0) {
        color = mix(vec3(0.0, 0.0, 0.5), vec3(0.0, 0.5, 1.0), (density - 0.98) * 50.0);
    } else if (density < 1.02) {
        color = mix(vec3(0.0, 0.5, 1.0), vec3(0.0, 1.0, 0.5), (density - 1.0) * 50.0);
    } else {
        color = mix(vec3(0.0, 1.0, 0.5), vec3(1.0, 1.0, 0.0), min(1.0, (density - 1.02) * 50.0));
    }
    
    // Add velocity brightness
    color = mix(color, vec3(1.0), min(0.3, velMag));
    
    FragColor = vec4(color, 1.0);
}
