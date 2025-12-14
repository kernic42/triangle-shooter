// LineRenderer.cpp
#include "LineRenderer.h"

using namespace glm;

static const char* vertSrc = R"(#version 300 es
uniform mat4 uProjection;
uniform float uRotation;
layout(location = 0) in vec2 aPos;
void main() {
    float c = cos(uRotation);
    float s = sin(uRotation);
    vec2 rotated = vec2(aPos.x * c - aPos.y * s, aPos.x * s + aPos.y * c);
    gl_Position = uProjection * vec4(rotated, 0.0, 1.0);
}
)";

static const char* fragSrc = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = uColor;
}
)";

void LineRenderer::init() {
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, nullptr);
    glCompileShader(vert);
    
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, nullptr);
    glCompileShader(frag);
    
    shader = glCreateProgram();
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    glLinkProgram(shader);
    glDeleteShader(vert);
    glDeleteShader(frag);
    
    projectionLoc = glGetUniformLocation(shader, "uProjection");
    rotationLoc = glGetUniformLocation(shader, "uRotation");
    colorLoc = glGetUniformLocation(shader, "uColor");
    
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void LineRenderer::cleanup() {
    glDeleteProgram(shader);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

std::vector<vec2> LineRenderer::triangulateLine(const std::vector<vec2>& points, float thickness) {
    std::vector<vec2> triangulatedLine;

    if (points.size() < 2) {
        return triangulatedLine;
    }

    vec2 startLine = normalize(points[1] - points[0]);
    vec2 normal = vec2(-startLine.y, startLine.x);
    triangulatedLine.push_back(points[0] - thickness * normal);
    triangulatedLine.push_back(points[0] + thickness * normal);

    vec2 precedingLine = startLine;

    for (size_t i = 2; i < points.size(); ++i) {
        vec2 line = normalize(points[i] - points[i - 1]);
        vec2 tangent = normalize(line + precedingLine);
        precedingLine = line;

        vec2 normal = vec2(-line.y, line.x);
        vec2 miter = vec2(-tangent.y, tangent.x);
        float dot = glm::dot(normal, miter);
        float length = thickness / dot;

        triangulatedLine.push_back(points[i - 1] - length * miter);
        triangulatedLine.push_back(points[i - 1] + length * miter);
    }

    size_t endIndexLine = points.size() - 1;
    vec2 endLine = normalize(points[endIndexLine] - points[endIndexLine - 1]);
    normal = vec2(-endLine.y, endLine.x);
    triangulatedLine.push_back(points[endIndexLine] - thickness * normal);
    triangulatedLine.push_back(points[endIndexLine] + thickness * normal);

    return triangulatedLine;
}

void LineRenderer::draw(vec2 from, vec2 to, vec4 color, float thickness) {
    draw({from, to}, color, thickness);
}

void LineRenderer::draw(const std::vector<vec2>& points, vec4 color, float thickness) {
    if (points.size() < 2) return;
    LineKey key{color, thickness};
    linesBatch[key].push_back(points);
}

void LineRenderer::flush(const float* projectionMatrix, float rotation) {
    if (linesBatch.empty()) return;
    
    glUseProgram(shader);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projectionMatrix);
    glUniform1f(rotationLoc, rotation);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    for (auto& [key, lines] : linesBatch) {
        if (lines.empty()) continue;
        
        std::vector<float> verts;
        
        for (auto& points : lines) {
            auto strip = triangulateLine(points, key.thickness * 0.5f);
            if (strip.empty()) continue;
            
            // Degenerate triangles to connect strips
            if (!verts.empty()) {
                verts.push_back(verts[verts.size() - 2]);
                verts.push_back(verts[verts.size() - 2]);
                verts.push_back(strip[0].x);
                verts.push_back(strip[0].y);
            }
            
            for (auto& p : strip) {
                verts.push_back(p.x);
                verts.push_back(p.y);
            }
        }
        
        size_t dataSize = verts.size() * sizeof(float);
        if (dataSize > bufferCapacity) {
            glBufferData(GL_ARRAY_BUFFER, dataSize, verts.data(), GL_DYNAMIC_DRAW);
            bufferCapacity = dataSize;
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, verts.data());
        }
        
        glUniform4f(colorLoc, key.color.r, key.color.g, key.color.b, key.color.a);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, verts.size() / 2);
    }
    
    glBindVertexArray(0);
    linesBatch.clear();
}