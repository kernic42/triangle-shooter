// TextRenderer.cpp
#include "TextRenderer.h"
#include <stdexcept>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

TextRenderer::TextRenderer() : initialized(false), atlasTexture(0), currentZIndex(0),
                               screenWidth(800), screenHeight(600) {
    vertexShaderSource = R"(#version 300 es
        layout (location = 0) in vec4 vertex;
        out vec2 TexCoords;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        }
    )";

    fragmentShaderSource = R"(#version 300 es
        precision mediump float;
        in vec2 TexCoords;
        out vec4 FragColor;
        uniform sampler2D text;
        uniform vec4 textColor;

        void main() {
            float alpha = texture(text, TexCoords).r;
            FragColor = vec4(textColor.rgb, textColor.a * alpha);
        }
    )";
}

TextRenderer::~TextRenderer() {
    if (initialized) {
        glDeleteTextures(1, &atlasTexture);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ibo);
        glDeleteProgram(shaderProgram);
    }
}

GLuint TextRenderer::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        throw std::runtime_error(std::string("Shader compilation failed: ") + infoLog);
    }
    return shader;
}

bool TextRenderer::generateAtlas(FT_Face face) {
    FT_Set_Pixel_Sizes(face, 0, 48);

    int maxWidth = 0, maxHeight = 0;

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }
        maxWidth = std::max(maxWidth, static_cast<int>(face->glyph->bitmap.width));
        maxHeight = std::max(maxHeight, static_cast<int>(face->glyph->bitmap.rows));
    }

    const int padding = 1;
    maxWidth += padding * 2;
    maxHeight += padding * 2;

    const int charsPerRow = 16;
    int atlasWidth = maxWidth * charsPerRow;
    int atlasHeight = maxHeight * ((128 + charsPerRow - 1) / charsPerRow);

    std::vector<unsigned char> atlasBuffer(atlasWidth * atlasHeight, 0);

    int xOffset = 0, yOffset = 0;

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }

        FT_Bitmap& bitmap = face->glyph->bitmap;

        if (xOffset + maxWidth > atlasWidth) {
            xOffset = 0;
            yOffset += maxHeight;
        }

        int xPos = xOffset + padding;
        int yPos = yOffset + padding;

        for (unsigned int row = 0; row < bitmap.rows; row++) {
            for (unsigned int col = 0; col < bitmap.width; col++) {
                size_t atlasIndex = (yPos + row) * atlasWidth + (xPos + col);
                if (atlasIndex < atlasBuffer.size()) {
                    atlasBuffer[atlasIndex] = bitmap.buffer[row * bitmap.width + col];
                }
            }
        }

        float x1 = static_cast<float>(xPos) / static_cast<float>(atlasWidth);
        float y1 = static_cast<float>(yPos) / static_cast<float>(atlasHeight);
        float x2 = static_cast<float>(xPos + bitmap.width) / static_cast<float>(atlasWidth);
        float y2 = static_cast<float>(yPos + bitmap.rows) / static_cast<float>(atlasHeight);

        Character character = {
            {glm::vec2(x1, y1), glm::vec2(x2, y2)},
            glm::ivec2(bitmap.width, bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };

        characters.insert(std::pair<char, Character>(c, character));

        xOffset += maxWidth;
    }

    glGenTextures(1, &atlasTexture);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasBuffer.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return true;
}

bool TextRenderer::initialize(const char* fontPath, int width, int height) {
    screenWidth = width;
    screenHeight = height;
    currentZIndex = 0;

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        FT_Done_FreeType(ft);
        return false;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (!generateAtlas(face)) {
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Setup VAO/VBO/IBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 4 * 4096, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    indices.clear();
    constexpr int MAX_CHARS = 4096;
    indices.reserve(MAX_CHARS * 6);

    for (int i = 0; i < MAX_CHARS; i++) {
        int baseVertex = i * 4;
        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 3);
        indices.push_back(baseVertex + 2);
    }

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Compile shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw std::runtime_error(std::string("Shader program linking failed: ") + infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    initialized = true;
    return true;
}

void TextRenderer::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void TextRenderer::setupRenderState() {
    if (!initialized) return;

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 
                                       0.0f, static_cast<float>(screenHeight), 
                                       -1.0f, 1.0f);

    glBindVertexArray(vao);
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "text"), 0);
}

void TextRenderer::cleanupRenderState() {
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void TextRenderer::draw(const std::string& text, float x, float y, float scale, glm::vec4 color) {
    textQueue.push_back({text, x, y, scale, color, false, currentZIndex++});
}

void TextRenderer::draw(const std::string& text, float x, float y, float scale, glm::vec4 color, int zIndex) {
    textQueue.push_back({text, x, y, scale, color, false, zIndex});
}

void TextRenderer::drawCentered(const std::string& text, float x, float y, float scale, glm::vec4 color) {
    textQueue.push_back({text, x, y, scale, color, true, currentZIndex++});
}

void TextRenderer::drawCentered(const std::string& text, float x, float y, float scale, glm::vec4 color, int zIndex) {
    textQueue.push_back({text, x, y, scale, color, true, zIndex});
}

void TextRenderer::flush() {
    if (!initialized || textQueue.empty()) return;

    std::sort(textQueue.begin(), textQueue.end(),
              [](const QueuedText& a, const QueuedText& b) {
                  return a.zIndex < b.zIndex;
              });

    setupRenderState();

    struct Vec4Comparator {
        bool operator()(const glm::vec4& a, const glm::vec4& b) const {
            if (a.x != b.x) return a.x < b.x;
            if (a.y != b.y) return a.y < b.y;
            if (a.z != b.z) return a.z < b.z;
            return a.w < b.w;
        }
    };

    std::map<glm::vec4, std::vector<GLfloat>, Vec4Comparator> colorBatches;
    std::map<glm::vec4, int, Vec4Comparator> charCounts;

    for (const auto& text : textQueue) {
        auto& colorVertices = colorBatches[text.color];
        auto& colorCharCount = charCounts[text.color];

        float startX = text.x;
        float startY = text.y;

        if (text.centered) {
            float totalWidth = 0.0f;
            float maxHeight = 0.0f;
            for (const char& c : text.text) {
                Character ch = characters[c];
                totalWidth += (ch.Advance >> 6) * text.scale;
                maxHeight = std::max(maxHeight, ch.Size.y * text.scale);
            }
            startX = text.x - (totalWidth / 2.0f);
            startY = text.y - maxHeight;
        }

        float x = startX;
        for (const char& c : text.text) {
            Character ch = characters[c];

            float xpos = x + ch.Bearing.x * text.scale;
            float ypos = startY - (ch.Size.y - ch.Bearing.y) * text.scale;
            float w = ch.Size.x * text.scale;
            float h = ch.Size.y * text.scale;

            GLfloat quadVertices[] = {
                xpos,     ypos,     ch.TexCoords[0].x, ch.TexCoords[1].y,
                xpos,     ypos + h, ch.TexCoords[0].x, ch.TexCoords[0].y,
                xpos + w, ypos,     ch.TexCoords[1].x, ch.TexCoords[1].y,
                xpos + w, ypos + h, ch.TexCoords[1].x, ch.TexCoords[0].y
            };

            colorVertices.insert(colorVertices.end(), std::begin(quadVertices), std::end(quadVertices));
            colorCharCount++;

            x += (ch.Advance >> 6) * text.scale;
        }
    }

    for (const auto& [color, vertices] : colorBatches) {
        int charCount = charCounts[color];

        glUniform4f(glGetUniformLocation(shaderProgram, "textColor"),
                    color.x, color.y, color.z, color.w);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(GLfloat), vertices.data());

        glDrawElements(GL_TRIANGLES, charCount * 6, GL_UNSIGNED_INT, 0);
    }

    cleanupRenderState();
    clear();
}

void TextRenderer::clear() {
    textQueue.clear();
    vertices.clear();
    currentZIndex = 0;
}

void TextRenderer::getStringMetrics(const std::string& text, float scale,
                                    float& width, float& maxHeight,
                                    float& maxAscent, float& maxDescent) {
    if (!initialized) {
        throw std::runtime_error("TextRenderer not initialized!");
    }

    width = 0.0f;
    maxHeight = 0.0f;
    maxAscent = 0.0f;
    maxDescent = 0.0f;

    for (const char& c : text) {
        Character ch = characters[c];
        width += (ch.Advance >> 6) * scale;

        float height = ch.Size.y * scale;
        float ascent = ch.Bearing.y * scale;
        float descent = (height - ascent);

        maxHeight = std::max(maxHeight, height);
        maxAscent = std::max(maxAscent, ascent);
        maxDescent = std::max(maxDescent, descent);
    }
}

std::vector<float> TextRenderer::getLetterPositions(const std::string& text, float x, float scale) {
    if (!initialized) {
        throw std::runtime_error("TextRenderer not initialized!");
    }

    std::vector<float> positions;
    positions.reserve(text.length());

    float currentX = x;
    positions.push_back(currentX);

    for (size_t i = 0; i < text.length(); i++) {
        Character ch = characters[text[i]];
        currentX += (ch.Advance >> 6) * scale;

        if (i < text.length() - 1) {
            positions.push_back(currentX);
        }
    }

    return positions;
}