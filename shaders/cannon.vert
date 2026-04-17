#version 300 es
precision mediump float;

layout(location = 0) in vec2 aPos;            // vert
layout(location = 1) in vec2 aTexCoord;       // tex coord
layout(location = 2) in vec2 aCannonPos;      // offset per model
layout(location = 3) in float aGridIndex;     // grid index(color) per model

uniform mat4 uProjection;
uniform vec2 uGridDimensions;
uniform float uCannonAngle;
//uniform float uShipRotation;

out vec2 vTexCoord;

void main() {
    // need to calculate position of uv from uGridDimensions
    float row = floor(aGridIndex / uGridDimensions.x);    
    float column = mod(aGridIndex, uGridDimensions.x);

    float columnWidth = 1.0 / uGridDimensions.x;
    float rowHeight = 1.0 / uGridDimensions.y;
    
    float u = column * columnWidth + aTexCoord.x * columnWidth; // when texture coordinate is 1, don't forget to span width
    float v = row * rowHeight + aTexCoord.y * rowHeight; // starts at bottom y, goes up left to right
    vTexCoord = vec2(u, v); // UV

    float c = cos(uCannonAngle);
    float s = sin(uCannonAngle);
    vec2 rotatedLocal =  mat2(c, s, -s, c) * aPos; // x gives power to x, then gives power to y
    vec2 cannonModel = rotatedLocal + aCannonPos;

    gl_Position = uProjection * vec4(cannonModel, 0.0, 1.0);
}