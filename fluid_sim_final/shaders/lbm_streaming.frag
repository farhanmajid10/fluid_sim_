#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;
layout(location = 2) out float distOut2;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D distTex2;
uniform vec2 gridSize;

// D2Q9 lattice velocities
const ivec2 e[9] = ivec2[9](
    ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1),
    ivec2(-1, 0), ivec2(0, 0), ivec2(1, 0),
    ivec2(-1,-1), ivec2(0,-1), ivec2(1,-1)
);

// Opposite directions for bounce-back
const int opp[9] = int[9](8, 7, 6, 5, 4, 3, 2, 1, 0);

//fetch a single distribution from texture.
float fetchDist(int i, vec2 coord) {
    // Clamp to texture boundaries
    coord = clamp(coord, vec2(0.0), vec2(1.0));  //clamp(value, min, max)  prevents reading outside bounds.
    
    //If `i` is 0, 1, 2, or 3:** Read from `distTex0`
    if (i < 4) {
        vec4 f = texture(distTex0, coord);
        if (i == 0) return f.x;
        if (i == 1) return f.y;
        if (i == 2) return f.z;
        return f.w;
    } else if (i < 8) {  // If `i` is 4,5,6, or 7:** Read from `distTex1`
        vec4 f = texture(distTex1, coord);
        if (i == 4) return f.x;
        if (i == 5) return f.y;
        if (i == 6) return f.z;
        return f.w;
    } else {  //else read from distTex2
        return texture(distTex2, coord).r;
    }
}

void main() {
    vec2 texelSize = 1.0 / gridSize;  //size of one grid cell in texture coordinates
    vec2 pixel = texCoord * gridSize;
    
    // Stream each distribution
    for (int i = 0; i < 9; i++) {
        vec2 sourceCoord = texCoord - vec2(e[i]) * texelSize;  //src is one to the left, previous time step.
        vec2 sourcePixel = sourceCoord * gridSize;  //convert to pixel coord for boundary check.
        
        float value;
        
        // Check if source is outside boundaries
        if (sourcePixel.x < 0.5 || sourcePixel.x > gridSize.x - 0.5 ||
            sourcePixel.y < 0.5 || sourcePixel.y > gridSize.y - 0.5) {
            // Bounce-back: take opposite direction from current cell
            value = fetchDist(opp[i], texCoord);
        } else {
            // Normal streaming
            value = fetchDist(i, sourceCoord);
        }
        
        // Store in appropriate output
        if (i == 0) distOut0.x = value;
        else if (i == 1) distOut0.y = value;
        else if (i == 2) distOut0.z = value;
        else if (i == 3) distOut0.w = value;
        else if (i == 4) distOut1.x = value;
        else if (i == 5) distOut1.y = value;
        else if (i == 6) distOut1.z = value;
        else if (i == 7) distOut1.w = value;
        else distOut2 = value;
    }
}
