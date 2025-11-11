#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;
uniform sampler2D velocityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    vec2 velocity = texture(velocityTex, texCoord).rg;
    
    // Velocity magnitude
    float velMag = length(velocity) * 100.0;
    
    // BETTER color mapping with wider range
    vec3 color;
    
    // Map density range [0.9, 1.1] to colors
    float d = (density - 0.9) * 5.0; // Map to [0, 1] range
    d = clamp(d, 0.0, 1.0);
    
    if (d < 0.25) {
        // Dark blue to blue
        color = mix(vec3(0.0, 0.0, 0.2), vec3(0.0, 0.0, 1.0), d * 4.0);
    } else if (d < 0.5) {
        // Blue to cyan (at density = 1.0)
        color = mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), (d - 0.25) * 4.0);
    } else if (d < 0.75) {
        // Cyan to green
        color = mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), (d - 0.5) * 4.0);
    } else {
        // Green to yellow
        color = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (d - 0.75) * 4.0);
    }
    
    // Add subtle velocity brightness
    color = mix(color, vec3(1.0), min(0.1, velMag));
    
    FragColor = vec4(color, 1.0);
}
