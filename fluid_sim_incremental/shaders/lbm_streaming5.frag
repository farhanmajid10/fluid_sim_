#version 330 core
in vec2 texCoord;

layout(location = 0) out vec4 distOut0;
layout(location = 1) out vec4 distOut1;
layout(location = 2) out float distOut2;

uniform sampler2D distTex0;
uniform sampler2D distTex1;
uniform sampler2D distTex2;
uniform vec2 gridSize;

const ivec2 e[9] = ivec2[9](
    ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1),
    ivec2(-1, 0), ivec2(0, 0), ivec2(1, 0),
    ivec2(-1,-1), ivec2(0,-1), ivec2(1,-1)
);

float fetchDist(int i, vec2 coord) {
    if (i < 4) {
        vec4 f = texture(distTex0, coord);
        if (i == 0) return f.x;
        if (i == 1) return f.y;
        if (i == 2) return f.z;
        return f.w;
    } else if (i < 8) {
        vec4 f = texture(distTex1, coord);
        if (i == 4) return f.x;
        if (i == 5) return f.y;
        if (i == 6) return f.z;
        return f.w;
    } else {
        return texture(distTex2, coord).r;
    }
}

void main() {
    vec2 texelSize = 1.0 / gridSize;
    
    // Stream: pull distributions from neighbors
    distOut0.x = fetchDist(0, texCoord - vec2(e[0]) * texelSize);
    distOut0.y = fetchDist(1, texCoord - vec2(e[1]) * texelSize);
    distOut0.z = fetchDist(2, texCoord - vec2(e[2]) * texelSize);
    distOut0.w = fetchDist(3, texCoord - vec2(e[3]) * texelSize);
    
    distOut1.x = fetchDist(4, texCoord - vec2(e[4]) * texelSize);
    distOut1.y = fetchDist(5, texCoord - vec2(e[5]) * texelSize);
    distOut1.z = fetchDist(6, texCoord - vec2(e[6]) * texelSize);
    distOut1.w = fetchDist(7, texCoord - vec2(e[7]) * texelSize);
    
    distOut2 = fetchDist(8, texCoord - vec2(e[8]) * texelSize);
}
