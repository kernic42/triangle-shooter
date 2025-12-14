#version 300 es
precision mediump float;
in vec2 TexCoord;
uniform sampler2D texture1;
uniform vec2 u_Size;
uniform vec2 u_BottomLeft;
uniform float u_Radius;
uniform float u_PixelSize;
uniform float opacity;
out vec4 FragColor;

void main() {
    vec2 pixelPos = gl_FragCoord.xy;
    vec2 relPos = pixelPos - u_BottomLeft;
    bool inRect = (relPos.x >= 0.0 && relPos.x <= u_Size.x && relPos.y >= 0.0 && relPos.y <= u_Size.y);
    float edgeDist = 0.0;
    if (inRect) {
        bool bl = relPos.x < u_Radius && relPos.y < u_Radius;
        bool br = relPos.x > (u_Size.x - u_Radius) && relPos.y < u_Radius;
        bool tr = relPos.x > (u_Size.x - u_Radius) && relPos.y > (u_Size.y - u_Radius);
        bool tl = relPos.x < u_Radius && relPos.y > (u_Size.y - u_Radius);
        if (bl) edgeDist = u_Radius - distance(relPos, vec2(u_Radius, u_Radius));
        else if (br) edgeDist = u_Radius - distance(relPos, vec2(u_Size.x - u_Radius, u_Radius));
        else if (tr) edgeDist = u_Radius - distance(relPos, vec2(u_Size.x - u_Radius, u_Size.y - u_Radius));
        else if (tl) edgeDist = u_Radius - distance(relPos, vec2(u_Radius, u_Size.y - u_Radius));
        else edgeDist = 1.0;
    } else edgeDist = -1.0;

    float alpha = smoothstep(-u_PixelSize, 0.0, edgeDist);
    vec4 texColor = texture(texture1, TexCoord);
    FragColor = vec4(texColor.rgb, texColor.a * alpha * opacity);
    if (alpha < 0.001) discard;
}