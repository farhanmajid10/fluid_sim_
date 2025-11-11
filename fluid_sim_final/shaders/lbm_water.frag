#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;
uniform sampler2D velocityTex;

void main() {
    float density = texture(densityTex, texCoord).r;
    vec2 velocity = texture(velocityTex, texCoord).rg;
    
    // Velocity magnitude for surface effects
    float velMag = length(velocity) * 20.0;  // Reduced sensitivity
    
    // Water-like color mapping
    vec3 deepWater = vec3(0.0, 0.15, 0.3);
    vec3 midWater = vec3(0.0, 0.3, 0.5);
    vec3 shallowWater = vec3(0.1, 0.5, 0.6);
    vec3 foam = vec3(0.7, 0.8, 0.9);  // Less bright foam
    
    vec3 color;
    
    // Less sensitive density mapping
    float d = (density - 0.99) * 10.0;  // Less sensitive
    d = clamp(d, -0.5, 1.5);
    
    if (d < 0.0) {
        color = mix(deepWater * 0.9, deepWater, d + 0.5);
    } else if (d < 0.5) {
        color = mix(deepWater, midWater, d * 2.0);
    } else if (d < 1.0) {
        color = mix(midWater, shallowWater, (d - 0.5) * 2.0);
    } else {
        color = mix(shallowWater, foam, (d - 1.0) * 2.0);
    }
    
    // Subtle velocity highlights
    if (velMag > 0.2) {
        color = mix(color, foam, min(0.2, velMag * 0.5));  // Much less white
    }
    
    FragColor = vec4(color, 1.0);
}
