#version 330 core

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D densityTex;
uniform sampler2D velocityTex;
uniform vec2 screenSize;
uniform vec2 gridSize;

// Convert HSV to RGB color space
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    float density = texture(densityTex, texCoord).r;
    vec2 velocity = texture(velocityTex, texCoord).rg;
    
    float velMag = length(velocity);
    
    // Color based on velocity magnitude and density
    // This mimics the original Processing visualization
    
    // Original used: color(140, dens*200, 200/dens) in HSB mode
    // HSB values are in range 0-255 in Processing, we need 0-1
    
    float hue = 140.0 / 255.0;  // Cyan-ish base color
    float saturation = clamp(density * 200.0 / 255.0, 0.0, 1.0);
    float brightness = clamp(200.0 / density / 255.0, 0.0, 1.0);
    
    // Alternative visualization based on velocity
    // Uncomment to use velocity-based coloring instead
    /*
    float velNorm = clamp(velMag * 4.0, 0.0, 1.0);
    float minHue = 160.0 / 360.0;  // Blue-cyan
    float maxHue = 130.0 / 360.0;  // Green-cyan
    float hue = mix(minHue, maxHue, velNorm);
    float saturation = 0.8;
    float brightness = 0.9;
    */
    
    vec3 color = hsv2rgb(vec3(hue, saturation, brightness));
    
    FragColor = vec4(color, 1.0);
}

