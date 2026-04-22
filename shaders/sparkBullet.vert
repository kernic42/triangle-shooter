#version 300 es
precision mediump float;

layout(location = 0) in vec2 aQuad;
layout(location = 1) in vec2 iCenter;
layout(location = 2) in vec2 iVelocity;
layout(location = 3) in float iSeed;
layout(location = 4) in float iType;
layout(location = 5) in float iSpawnTime;

uniform float uTime;
//uniform float uAspect;
uniform mat4 uProjview;
uniform float uSize;
uniform float uLifespan;
uniform float uExplodeDist;
uniform float uBoomZone;

out vec2 vUV;
out float vSeed;
out float vLife;
out float vType;

void main() {
    vUV = aQuad * 0.5 + 0.5;
    vSeed = iSeed;
    vType = iType;

    float age = uTime - iSpawnTime;
    if (age < 0.0 || age > uLifespan) {
        gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
        vLife = 0.0;
        return;
    }

    float speed = length(iVelocity);
    float dist = speed * age;
    float myExplodeDist = max(uExplodeDist - fract(iSeed * 31.71) * uBoomZone, 0.01);

    // Past explode point? Hide - explosion shader handles it
    if (uExplodeDist > 0.0 && dist > myExplodeDist) {
        gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
        vLife = 0.0;
        return;
    }

    float lifeFrac = age / uLifespan;
    float peak = 0.3 + fract(iSeed * 7.13) * 0.3;
    float rampUp = smoothstep(0.0, peak, lifeFrac);
    float rampDown = 1.0 - smoothstep(peak, 1.0, lifeFrac);
    vLife = 1.0;//max(mix(0.85, 1.0, rampUp) * rampDown, 0.85);

    vec2 center = iCenter + iVelocity * age;
    vec2 dir = speed > 0.001 ? iVelocity / speed : vec2(1.0, 0.0);
    vec2 perp = vec2(-dir.y, dir.x);

    float s = 0.012 * uSize;
    vec2 world = center + dir * aQuad.x * s + perp * aQuad.y * s;
    gl_Position = uProjview * vec4(world, 0.0, 1.0);
}