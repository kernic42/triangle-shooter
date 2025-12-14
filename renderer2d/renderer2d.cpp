// Renderer2D.cpp
#include "Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

static const char* rectVertSrc = R"(#version 300 es
precision mediump float;
layout(location = 0) in vec2 aPos;
out vec2 vLocalPos;
uniform mat4 uProjection;
uniform vec2 uPos;
uniform vec2 uSize;

void main() {
    vec2 worldPos = uPos + aPos * uSize;
    gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);
    vLocalPos = aPos * uSize;
}
)";

static const char* rectFragSrc = R"(#version 300 es
precision mediump float;
in vec2 vLocalPos;
out vec4 fragColor;
uniform vec4 uColor;
uniform vec2 uSize;
uniform float uRadius;
uniform float uBorder;

float roundedBoxSDF(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    vec2 halfSize = uSize * 0.5;
    vec2 centered = vLocalPos - halfSize;
    
    float dist = roundedBoxSDF(centered, halfSize, uRadius);
    
    if (uBorder > 0.0) {
        float innerDist = roundedBoxSDF(centered, halfSize - uBorder, max(0.0, uRadius - uBorder));
        if (dist > 0.5 || innerDist < -0.5) {
            discard;
        }
        float alpha = smoothstep(0.5, -0.5, dist) * smoothstep(-0.5, 0.5, innerDist);
        fragColor = vec4(uColor.rgb, uColor.a * alpha);
    } else {
        if (dist > 0.5) {
            discard;
        }
        float alpha = smoothstep(0.5, -0.5, dist);
        fragColor = vec4(uColor.rgb, uColor.a * alpha);
    }
}
)";

static const char* imageVertSrc = R"(#version 300 es
layout(location = 0) in vec4 aVertex;
out vec2 vTexCoord;
uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * vec4(aVertex.xy, 0.0, 1.0);
    vTexCoord = aVertex.zw;
}
)";

static const char* imageFragSrc = R"(#version 300 es
precision mediump float;
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D uTexture;
uniform vec4 uTint;

void main() {
    fragColor = texture(uTexture, vTexCoord) * uTint;
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("Shader compile error: %s\n", infoLog);
        return 0;
    }
    return shader;
}

static GLuint createProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    
    if (vert == 0 || frag == 0) {
        printf("Shader compilation failed!\n");
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        printf("Program link error: %s\n", infoLog);
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }
    
    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

void Renderer2D::init() {
    initRectShader();
    initImageShader();
}

void Renderer2D::initRectShader() {
    rectShader = createProgram(rectVertSrc, rectFragSrc);
    if (rectShader == 0) {
        printf("Failed to create rect shader program!\n");
        return;
    }
    
    rectProjLoc = glGetUniformLocation(rectShader, "uProjection");
    rectColorLoc = glGetUniformLocation(rectShader, "uColor");
    rectPosLoc = glGetUniformLocation(rectShader, "uPos");
    rectSizeLoc = glGetUniformLocation(rectShader, "uSize");
    rectRadiusLoc = glGetUniformLocation(rectShader, "uRadius");
    rectBorderLoc = glGetUniformLocation(rectShader, "uBorder");
    
    printf("Rect shader uniforms: proj=%d color=%d pos=%d size=%d radius=%d border=%d\n",
           rectProjLoc, rectColorLoc, rectPosLoc, rectSizeLoc, rectRadiusLoc, rectBorderLoc);
    
    float quadVerts[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    
    glGenVertexArrays(1, &rectVao);
    glGenBuffers(1, &rectVbo);
    glBindVertexArray(rectVao);
    glBindBuffer(GL_ARRAY_BUFFER, rectVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    
    printf("Rect shader initialized: program=%u vao=%u vbo=%u\n", rectShader, rectVao, rectVbo);
}

void Renderer2D::initImageShader() {
    imageShader = createProgram(imageVertSrc, imageFragSrc);
    if (imageShader == 0) {
        printf("Failed to create image shader program!\n");
        return;
    }
    
    imageProjLoc = glGetUniformLocation(imageShader, "uProjection");
    imageTintLoc = glGetUniformLocation(imageShader, "uTint");
    
    printf("Image shader uniforms: proj=%d tint=%d\n", imageProjLoc, imageTintLoc);
    
    glGenVertexArrays(1, &imageVao);
    glGenBuffers(1, &imageVbo);
    glBindVertexArray(imageVao);
    glBindBuffer(GL_ARRAY_BUFFER, imageVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    
    printf("Image shader initialized: program=%u vao=%u vbo=%u\n", imageShader, imageVao, imageVbo);
}

void Renderer2D::cleanup() {
    glDeleteProgram(rectShader);
    glDeleteVertexArrays(1, &rectVao);
    glDeleteBuffers(1, &rectVbo);
    glDeleteProgram(imageShader);
    glDeleteVertexArrays(1, &imageVao);
    glDeleteBuffers(1, &imageVbo);
}

void Renderer2D::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void Renderer2D::drawFilledRect(glm::vec2 pos, float width, float height, glm::vec4 color) {
    rectQueue.push_back({pos, width, height, 0.0f, 0.0f, color, true});
}

void Renderer2D::drawRect(glm::vec2 pos, float width, float height, float borderWidth, glm::vec4 color) {
    rectQueue.push_back({pos, width, height, borderWidth, 0.0f, color, false});
}

void Renderer2D::drawFilledRoundedRect(glm::vec2 pos, float width, float height, float radius, glm::vec4 color) {
    rectQueue.push_back({pos, width, height, 0.0f, radius, color, true});
}

void Renderer2D::drawRoundedRect(glm::vec2 pos, float width, float height, float borderWidth, float radius, glm::vec4 color) {
    rectQueue.push_back({pos, width, height, borderWidth, radius, color, false});
}

void Renderer2D::drawImage(GLuint textureId, float x, float y, float width, float height, glm::vec4 tint) {
    imageQueue.push_back({textureId, x, y, width, height, tint});
}

void Renderer2D::flush() {
    flushRects();
    flushImages();
}

void Renderer2D::flushRects() {
    if (rectQueue.empty()) return;
    if (rectShader == 0) {
        printf("Rect shader not valid!\n");
        rectQueue.clear();
        return;
    }
    
    glm::mat4 proj = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);
    
    glUseProgram(rectShader);
    glUniformMatrix4fv(rectProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
    glBindVertexArray(rectVao);
    
    for (auto& r : rectQueue) {
        glUniform4f(rectColorLoc, r.color.r, r.color.g, r.color.b, r.color.a);
        glUniform2f(rectPosLoc, r.pos.x, r.pos.y);
        glUniform2f(rectSizeLoc, r.width, r.height);
        glUniform1f(rectRadiusLoc, r.radius);
        glUniform1f(rectBorderLoc, r.filled ? 0.0f : r.borderWidth);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindVertexArray(0);
    rectQueue.clear();
}

void Renderer2D::flushImages() {
    if (imageQueue.empty()) return;
    if (imageShader == 0) {
        printf("Image shader not valid!\n");
        imageQueue.clear();
        return;
    }
    
    glm::mat4 proj = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);
    
    glUseProgram(imageShader);
    glUniformMatrix4fv(imageProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
    glBindVertexArray(imageVao);
    glBindBuffer(GL_ARRAY_BUFFER, imageVbo);
    
    for (auto& img : imageQueue) {
        float x = img.x, y = img.y, w = img.width, h = img.height;
        
        float verts[] = {
            x,     y,     0.0f, 1.0f,
            x + w, y,     1.0f, 1.0f,
            x,     y + h, 0.0f, 0.0f,
            x + w, y,     1.0f, 1.0f,
            x + w, y + h, 1.0f, 0.0f,
            x,     y + h, 0.0f, 0.0f
        };
        
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glUniform4f(imageTintLoc, img.tint.r, img.tint.g, img.tint.b, img.tint.a);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, img.textureId);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindVertexArray(0);
    imageQueue.clear();
}