#version 330 core

in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform vec2 gridSize;

// D2Q9 lattice velocities
const ivec2 e[9] = ivec2[9](
    ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1),
    ivec2(-1, 0), ivec2(0, 0), ivec2(1, 0),
    ivec2(-1,-1), ivec2(0,-1), ivec2(1,-1)
);

vec2 getSourceCoord(vec2 coord, ivec2 velocity) {
    vec2 sourcePos = coord * gridSize - vec2(velocity);
    // Periodic boundary conditions
    sourcePos = mod(sourcePos + gridSize, gridSize);
    return sourcePos / gridSize;
}

void main() {
    vec4 newF0, newF1;
    
    // Stream from neighboring cells
    // Each distribution function streams from the cell it's coming from
    newF0.x = texture(distTex0, getSourceCoord(texCoord, e[0])).x;
    newF0.y = texture(distTex0, getSourceCoord(texCoord, e[1])).y;
    newF0.z = texture(distTex0, getSourceCoord(texCoord, e[2])).z;
    newF0.w = texture(distTex0, getSourceCoord(texCoord, e[3])).w;
    
    newF1.x = texture(distTex1, getSourceCoord(texCoord, e[4])).x;
    newF1.y = texture(distTex1, getSourceCoord(texCoord, e[5])).y;
    newF1.z = texture(distTex1, getSourceCoord(texCoord, e[6])).z;
    newF1.w = texture(distTex1, getSourceCoord(texCoord, e[7])).w;
    
    distOut0 = newF0;
    distOut1 = newF1;
}

