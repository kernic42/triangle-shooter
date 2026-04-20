#version 300 es
precision mediump float;

in vec2 vUV;    // Uv...
in float vSeed; // unique random seed per spark
in float vLife; // makes the color 'breath'
in float vType; // decides what color, should be attributes in mem not picked run time

uniform float uTime;

out vec4 fragColor;

void main() {
    vec2 ctr = vec2(0.5);
    float dist = length(vUV - ctr) * 2.0;
    float circle = 1.0 - smoothstep(0.6, 1.0, dist);
    float shape = circle * vLife;
    if (shape < 0.002) discard;

    float hotAngle = vSeed * 6.2832;
    vec2 hotDir = vec2(cos(hotAngle), sin(hotAngle));
    vec2 uv = vUV - ctr;
    float gradient = dot(uv, hotDir) + 0.5;
    float hotspot = pow(gradient, 1.0 + vLife * 4.0);

    float rimFactor = smoothstep(0.15, 0.75, dist);
    float brightness = mix(0.55, 1.0, rimFactor * 0.6 + 0.4) * vLife;

    vec3 baseCol, hotCol, breatheCol;
    if (vType < 0.5) {
        baseCol = vec3(1.0, 0.4, 0.03); hotCol = vec3(1.0, 0.7, 0.15); breatheCol = vec3(0.6, 0.4, 0.2);
    } else if (vType < 1.5) {
        baseCol = vec3(0.1, 0.5, 1.0); hotCol = vec3(0.6, 0.85, 1.0); breatheCol = vec3(0.4, 0.5, 0.7);
    } else {
        baseCol = vec3(0.08, 0.7, 0.05); hotCol = vec3(0.3, 1.0, 0.15); breatheCol = vec3(0.6, 0.9, 0.3);
    }

    vec3 col = mix(baseCol, hotCol, hotspot * 0.5);

    float b = sin(uTime * 3.5 + vSeed * 41.3) * 0.5
            + sin(uTime * 5.7 + vSeed * 23.7) * 0.3
            + sin(uTime * 9.3 + vSeed * 67.1) * 0.2;
    float breathe = b * 0.5 + 0.5;
    breathe *= breathe;

    float intensity = (1.5 + hotspot * 2.0) * brightness;
    col *= intensity;
    col = mix(col, col * 1.8 + breatheCol, breathe * 0.5);

    fragColor = vec4(col * shape, shape);
}