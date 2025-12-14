// Fragment shader
#version 300 es
precision mediump float;

in vec2 TexCoord;
uniform sampler2D texture1;
out vec4 FragColor;

void main() {
    FragColor = textureLod(texture1, TexCoord, -1.0);
}