#version 300 es

// Input vertex data
layout (location = 0) in vec4 a_position;
layout (location = 1) in vec2 a_texCoord;

// Pass texture coordinates to the fragment shader
out vec2 v_texCoord;

void main() {
    // Pass through the texture coordinates without modification
    v_texCoord = a_texCoord;

    // Pass through the position without modification
    gl_Position = a_position;
}
