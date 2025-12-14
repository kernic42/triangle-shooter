#version 300 es
precision highp float;
uniform vec4 u_Color;

out vec4 FragColor;

void main() {
    // Simply output the uniform color
    FragColor = u_Color;
}
