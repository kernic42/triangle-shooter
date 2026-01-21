#version 300 es
precision highp float;

layout(location = 0) in vec2 aPos;            // vert
layout(location = 1) in vec2 aTexCoord;       // tex coord
layout(location = 2) in vec2 aCannonPosition; // offset per model
layout(location = 3) in float aGridIndex;     // grid index(color) per model

uniform mat4 uProjection;
uniform vec2 uGridDimensions;
uniform float uCannonAngle;
uniform float uShipRotation;

out vec2 vTexCoord;

void main() {
    // need to calculate position of uv from uGridDimensions
    float row = floor(aGridIndex / uGridDimensions.x);    
    float column = mod(aGridIndex, uGridDimensions.x);

    float widthX = 1.0 / uGridDimensions.x;
    float heightY = 1.0 / uGridDimensions.y;
    
    float coordX = column * widthX + aTexCoord.x * widthX; // when texture coordinate is 1, don't forget to span width
    float coordY = row * heightY + aTexCoord.y * heightY; // starts at bottom y, goes up left to right
    vTexCoord = vec2(coordX, coordY); // UV

    float c = cos(uCannonAngle);
    float s = sin(uCannonAngle);
    vec2 rotated = vec2(
        aPos.x * c - aPos.y * s,
        aPos.x * s + aPos.y * c
    );
    
    vec2 cannonModel = aCannonPosition + rotated;
    
    c = cos(uShipRotation);
    s = sin(uShipRotation);
    vec2 shipRotated = vec2(
        cannonModel.x * c - cannonModel.y * s,
        cannonModel.x * s + cannonModel.y * c
    );

    gl_Position = uProjection * vec4(shipRotated, 0.0, 1.0);
}