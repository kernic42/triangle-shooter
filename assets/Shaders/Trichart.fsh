#version 300 es
precision mediump float;

// Final output color
out vec4 FragColor;

// Input from the vertex shader
in vec2 v_texCoord;

// Single texture containing all three boundaries in RGB channels
uniform sampler2D boundaryTexture;

void main() {
    // Sample all three Y-boundary values from the single texture
    vec3 boundaries = texture(boundaryTexture, v_texCoord).rgb;
    float y_boundary1 = boundaries.r;  // Top layer
    float y_boundary2 = boundaries.g;  // Middle layer
    float y_boundary3 = boundaries.b;  // Bottom layer

    // Define the hardcoded colors for each region
    vec4 colorTop =  vec4(0.0, 0.0, 0.0, 0.0);     // Transparent
    vec4 colorMiddle = vec4(1.0, 0.0, 0.0, 0.5);   // Red
    vec4 colorBottom = vec4(0.0, 0.0, 1.0, 0.5);   // Blue
    vec4 colorLowest = vec4(0.0, 1.0, 0.0, 0.5);   // Green

    // Determine the final color using conditional logic
    float currentY = v_texCoord.y;

    if (currentY > y_boundary1) {
        FragColor = colorTop;
    } else if (currentY > y_boundary2) {
        FragColor = colorMiddle;
    } else if (currentY > y_boundary3) {
        FragColor = colorBottom;
    } else {
        FragColor = colorLowest;
    }
}