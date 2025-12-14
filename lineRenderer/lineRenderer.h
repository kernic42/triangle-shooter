// LineRenderer.h
#pragma once
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>

class LineRenderer {
public:
    void init();
    void cleanup();
    
    void draw(glm::vec2 from, glm::vec2 to, glm::vec4 color = glm::vec4(1.0f), float thickness = 1.0f);
    void draw(const std::vector<glm::vec2>& points, glm::vec4 color = glm::vec4(1.0f), float thickness = 1.0f);
    void flush(const float* projectionMatrix, float rotation = 0.0f);
    
private:
    std::vector<glm::vec2> triangulateLine(const std::vector<glm::vec2>& points, float thickness);
    
    struct LineKey {
        glm::vec4 color;
        float thickness;
        
        bool operator<(const LineKey& other) const {
            if (color.r != other.color.r) return color.r < other.color.r;
            if (color.g != other.color.g) return color.g < other.color.g;
            if (color.b != other.color.b) return color.b < other.color.b;
            if (color.a != other.color.a) return color.a < other.color.a;
            return thickness < other.thickness;
        }
    };
    
    GLuint shader = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint projectionLoc = -1;
    GLint rotationLoc = -1;
    GLint colorLoc = -1;
    
    std::map<LineKey, std::vector<std::vector<glm::vec2>>> linesBatch;
    size_t bufferCapacity = 0;
};