#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;
layout(location = 2) out float distOut2;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D distTex2;
uniform vec2 dropPos;
uniform float dropStrength;
uniform float dropRadius;

const float w[9] = float[9](
    1.0/36.0, 1.0/9.0, 1.0/36.0,
    1.0/9.0, 4.0/9.0, 1.0/9.0,
    1.0/36.0, 1.0/9.0, 1.0/36.0
);

void main() {
    // Read existing distributions
    vec4 f0123 = texture(distTex0, texCoord);
    vec4 f4567 = texture(distTex1, texCoord);
    float f8 = texture(distTex2, texCoord).r;
    
    // Pass through
    distOut0 = f0123;
    distOut1 = f4567;
    distOut2 = f8;
    
    // Add perturbation at drop location
    float dist = length(texCoord - dropPos);
    if (dist < dropRadius) {
        float amount = dropStrength * exp(-10.0 * dist*dist / (dropRadius*dropRadius));
        
        // Add to all distributions weighted by w[i]
        distOut0.x += w[0] * amount;
        distOut0.y += w[1] * amount;
        distOut0.z += w[2] * amount;
        distOut0.w += w[3] * amount;
        
        distOut1.x += w[4] * amount;
        distOut1.y += w[5] * amount;
        distOut1.z += w[6] * amount;
        distOut1.w += w[7] * amount;
        
        distOut2 += w[8] * amount;
    }
}
