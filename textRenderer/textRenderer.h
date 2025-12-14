// TextRenderer.h
#pragma once
#include <string>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    bool initialize(const char* fontPath, int screenWidth, int screenHeight);
    void setScreenSize(int width, int height);
    
    void draw(const std::string& text, float x, float y, float scale, glm::vec4 color = glm::vec4(1.0f));
    void draw(const std::string& text, float x, float y, float scale, glm::vec4 color, int zIndex);
    void drawCentered(const std::string& text, float x, float y, float scale, glm::vec4 color = glm::vec4(1.0f));
    void drawCentered(const std::string& text, float x, float y, float scale, glm::vec4 color, int zIndex);
    
    void flush();
    void clear();
    
    void getStringMetrics(const std::string& text, float scale,
                          float& width, float& maxHeight,
                          float& maxAscent, float& maxDescent);
    std::vector<float> getLetterPositions(const std::string& text, float x, float scale);

private:
    struct Character {
        glm::vec2 TexCoords[2];
        glm::ivec2 Size;
        glm::ivec2 Bearing;
        GLuint Advance;
    };

    struct QueuedText {
        std::string text;
        float x, y, scale;
        glm::vec4 color;
        bool centered;
        int zIndex;
    };

    GLuint compileShader(GLenum type, const char* source);
    bool generateAtlas(FT_Face face);
    void setupRenderState();
    void cleanupRenderState();

    bool initialized;
    int screenWidth, screenHeight;
    
    GLuint atlasTexture;
    GLuint shaderProgram;
    GLuint vao, vbo, ibo;
    
    std::map<char, Character> characters;
    std::vector<QueuedText> textQueue;
    std::vector<GLuint> indices;
    std::vector<GLfloat> vertices;
    
    int currentZIndex;
    
    const char* vertexShaderSource;
    const char* fragmentShaderSource;
};