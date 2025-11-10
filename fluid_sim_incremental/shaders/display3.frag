#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;
uniform sampler2D velocityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    
    // Debug visualization
    vec3 color;
    
    if (density < 0.0) {
        // Negative = RED (error!)
        color = vec3(1.0, 0.0, 0.0);
    } else if (density < 0.5) {
        // Very low = BLUE
        color = vec3(0.0, 0.0, 1.0);
    } else if (density < 1.0) {
        // Low = CYAN
        color = mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), (density - 0.5) * 2.0);
    } else if (density < 1.1) {
        // Normal = GREEN
        color = mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), (density - 1.0) * 10.0);
    } else if (density < 1.5) {
        // High = YELLOW
        color = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (density - 1.1) * 2.5);
    } else {
        // Very high = WHITE
        color = vec3(1.0, 1.0, 1.0);
    }
    
    FragColor = vec4(color, 1.0);
}
