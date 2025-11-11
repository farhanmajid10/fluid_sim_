#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    
    // Simple visualization
    vec3 color;
    if (density < 0.99) {
        color = vec3(1.0, 0.0, 0.0);  // Red = too low
    } else if (density > 1.01) {
        color = vec3(0.0, 0.0, 1.0);  // Blue = too high  
    } else {
        color = vec3(0.0, 1.0, 0.0);  // Green = good
    }
    
    FragColor = vec4(color, 1.0);
}
