#version 300 es
layout (location = 0) in vec4 position;
layout (location = 1) in vec2 texCoord; // Texture coordinates input

out vec2 TexCoord; // Pass texture coordinates to fragment shader

void main() {
    TexCoord = texCoord; // Pass the texture coordinates
    gl_Position = position;
}