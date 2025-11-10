#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;
uniform sampler2D velocityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    
    // Direct grayscale visualization
    // If density = 1.0, we should see gray (0.5)
    // If density = 1.1, slightly brighter
    float gray = density * 0.5;  // Scale so 1.0 -> 0.5 gray
    
    // Add red tint if density > 1.0 to make it obvious
    vec3 color = vec3(gray, gray * (density <= 1.0 ? 1.0 : 0.5), gray * (density <= 1.0 ? 1.0 : 0.5));
    
    FragColor = vec4(color, 1.0);
}
