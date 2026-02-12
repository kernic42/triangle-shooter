#version 300 es
precision mediump float;

layout(location = 0) in vec2 aPos; // per vertex
layout(location = 1) in vec2 aTexCoord; // per vertex
layout(location = 2) in vec2 aOrigin; // per bullet
layout(location = 3) in vec2 aDirection; // per bullet
layout(location = 4) in vec2 aShipTranslate; // per bullet
layout(location = 5) in float aShipRotation; // per bullet
layout(location = 6) in float aVelocity; // per bullet
layout(location = 7) in float aGridIndex; // per bullet
layout(location = 8) in float aStartTime; // per bullet

uniform float uTime;
uniform vec2 uGridDimensions;
uniform mat4 uProjection;

out vec2 vTexCoord;

void main() {
    // calculate uv
    float row = floor(aGridIndex / uGridDimensions.x);
    float column = mod(aGridIndex, uGridDimensions.x);

    float columnWidth = 1.0 / uGridDimensions.x;
    float rowHeight = 1.0 / uGridDimensions.x;

    float u = columnWidth * column + aTexCoord.x * columnWidth;
    float v = rowHeight * row + aTexCoord.y * rowHeight;
    vTexCoord = vec2(u, v);

    // bullet advance in aDirection, following timePassed scaled by aVelocity (maybe should use v = d/t to calculate ray path on cpu)
    float timePassed = uTime - aStartTime;
    vec2 bulletPath = aDirection * (timePassed * aVelocity);

    float c = cos(aShipRotation);
    float s = sin(aShipRotation);
    vec2 rotatedOrigin = mat2(c, s, -s, c) * aOrigin;

    vec2 bulletTranslate = rotatedOrigin + bulletPath + aShipTranslate;

    gl_Position = uProjection * vec4(aPos + bulletTranslate, 0.0, 1.0);
}