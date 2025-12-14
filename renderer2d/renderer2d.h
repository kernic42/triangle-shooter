// Renderer2D.h
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>

class Renderer2D {
public:
    void init();
    void cleanup();
    void setScreenSize(int width, int height);
    
    void drawFilledRect(glm::vec2 pos, float width, float height, glm::vec4 color);
    void drawRect(glm::vec2 pos, float width, float height, float borderWidth, glm::vec4 color);
    void drawFilledRoundedRect(glm::vec2 pos, float width, float height, float radius, glm::vec4 color);
    void drawRoundedRect(glm::vec2 pos, float width, float height, float borderWidth, float radius, glm::vec4 color);
    
    void drawImage(GLuint textureId, float x, float y, float width, float height, glm::vec4 tint = glm::vec4(1.0f));
    
    void flush();

private:
    struct RectData {
        glm::vec2 pos;
        float width, height;
        float borderWidth;
        float radius;
        glm::vec4 color;
        bool filled;
    };
    
    struct ImageData {
        GLuint textureId;
        float x, y, width, height;
        glm::vec4 tint;
    };
    
    void initRectShader();
    void initImageShader();
    void flushRects();
    void flushImages();
    
    int screenWidth = 800;
    int screenHeight = 600;
    
    GLuint rectShader = 0;
    GLuint rectVao = 0;
    GLuint rectVbo = 0;
    GLint rectProjLoc = -1;
    GLint rectColorLoc = -1;
    GLint rectPosLoc = -1;
    GLint rectSizeLoc = -1;
    GLint rectRadiusLoc = -1;
    GLint rectBorderLoc = -1;
    std::vector<RectData> rectQueue;
    
    GLuint imageShader = 0;
    GLuint imageVao = 0;
    GLuint imageVbo = 0;
    GLint imageProjLoc = -1;
    GLint imageTintLoc = -1;
    std::vector<ImageData> imageQueue;
};