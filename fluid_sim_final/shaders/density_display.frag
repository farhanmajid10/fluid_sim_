#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    
    // Color mapping (similar to your Processing visualization)
    vec3 color;
    if (density < 1.0) {
        // Below normal: blue to cyan
        color = mix(vec3(0.0, 0.0, 0.4), vec3(0.0, 0.5, 0.5), density);
    } else if (density < 1.5) {
        // Normal to high: cyan to green
        color = mix(vec3(0.0, 0.5, 0.5), vec3(0.0, 1.0, 0.0), (density - 1.0) * 2.0);
    } else {
        // High: green to yellow
        color = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (density - 1.5) * 2.0);
    }
    
    FragColor = vec4(color, 1.0);
}
