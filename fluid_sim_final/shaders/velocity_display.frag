#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D velocityTex;

void main() {
    vec2 velocity = texture(velocityTex, texCoord).rg;
    
    // Visualize velocity as color
    // Red = positive X velocity (right)
    // Green = positive Y velocity (up)
    // Blue = velocity magnitude
    
    float speed = length(velocity);
    
    // Normalize for visualization (assuming max speed ~20)
    vec3 color;
    color.r = velocity.x / 20.0 + 0.5;  // Map [-20,20] to [0,1]
    color.g = velocity.y / 20.0 + 0.5;
    color.b = speed / 20.0;
    
    FragColor = vec4(color, 1.0);
}
