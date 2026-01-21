#version 300 es
precision highp float;

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

const int MAX_CANNONS = 256;

uniform vec2 uCannonPositions[MAX_CANNONS];
uniform float uCannonAngle;
uniform mat4 uProjection;
uniform mat3 uShipRotation;

uniform vec2 uGridDimensions;
uniform float uGridIndex;

out vec2 vTexCoord;

void main() {
    vec2 pos = uCannonPositions[gl_InstanceID];

    // need to calculate position of uv from uGridDimensions
    float row = floor(uGridIndex / uGridDimensions.x);    
    float column = mod(uGridIndex, uGridDimensions.x);

    float widthX = 1.0 / uGridDimensions.x;
    float heightY = 1.0 / uGridDimensions.y;
    
    // UV
    float coordX = column * widthX + aTexCoord.x * widthX; // when texture coordinate is 1, don't forget to span width
    float coordY = row * heightY + aTexCoord.y * heightY; // starts at bottom y, goes up left to right
    
    float c = cos(uCannonAngle);
    float s = sin(uCannonAngle);
    
    vec2 rotated = vec2(
        aPos.x * c - aPos.y * s,
        aPos.x * s + aPos.y * c
    );
    
    vec2 vertex = rotated + pos;
    
    vec3 shipRotated = uShipRotation * vec3(vertex, 0.0);

    gl_Position = uProjection * vec4(shipRotated, 1.0);

    vTexCoord = vec2(coordX, coordY);
}