#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    
    // More sensitive coloring
    vec3 color;
    if (density < 0.999) {
        color = vec3(0.0, 0.0, density);  // Blue shades
    } else if (density > 1.001) {
        color = vec3(density-1.0, 1.0, 0.0);  // Yellow shades
    } else {
        color = vec3(0.0, density, 0.0);  // Green = normal
    }
    
    FragColor = vec4(color, 1.0);
}
