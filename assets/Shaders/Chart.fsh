#version 300 es
precision mediump float;

out vec4 FragColor;
in vec2 v_texCoord;
uniform sampler2D boundaryTexture;

void main() {
    // Simple test: just output the sampled color to see if texture works
    vec3 boundaries = texture(boundaryTexture, v_texCoord).rgb;
    FragColor = vec4(boundaries, 1.0); // Display raw texture data
}