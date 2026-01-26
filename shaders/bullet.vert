#version 300 es
precision mediump float;

layout(location = 0) in vec2 aPos; // per vertex
layout(location = 1) in vec2 aTexCoord; // per vertex
layout(location = 2) in vec2 aOrigin; // per bullet
layout(location = 3) in vec2 aDirection; // per bullet
layout(location = 4) in float aVelocity; // per bullet
layout(location = 5) in float aGridIndex; // per bullet
layout(location = 6) in float aStartTime; // per bullet

uniform float uTime;
uniform vec2 uGridDimensions;

out vec2 vTexCoord;

void main() {
    // calculate uv
    float row = floor(aGridIndex / uGridDimensions.x);
    float column = mod(aGridIndex, uGridDimensions.x);

    float rowHeight = 1.0 / row;
    float columnWidth = 1.0 / column;

    float u = columnWidth * column + aTexCoord.x * columnWidth;
    float v = rowHeight * row + aTexCoord.y * rowHeight;
    vTexCoord = vec2(aTexCoord.x, aTexCoord.y);

    // bullet advance in aDirection, following timePassed scaled by aVelocity
    float timePassed = uTime - aStartTime;
    vec2 traveledDist = aDirection * (timePassed * aVelocity);
    vec2 translate = aOrigin + traveledDist;
    gl_Position = vec4(aPos + translate, 0.0, 1.0);
}