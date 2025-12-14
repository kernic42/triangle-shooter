// Button.h
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include "TextRenderer.h"
#include "Renderer2D.h"

struct Button {
    float x = 0, y = 0;
    float width = 100, height = 40;
    std::string text;
    float textScale = 1.0f;
    glm::vec4 color = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    glm::vec4 textColor = glm::vec4(1.0f);
    glm::vec4 borderColor = glm::vec4(1.0f);
    float borderWidth = 0;
    float borderRadius = 0;
    GLuint textureId = 0;
    float imageWidth = 0, imageHeight = 0;
    float imageGap = 10;
    std::string drawImage;  // "top", "left", "center"
    std::function<void(Button*)> callback;
};

class ButtonManager {
public:
    ButtonManager();
    ~ButtonManager();
    
    void init(TextRenderer* textRenderer, Renderer2D* renderer2d);
    
    Button* createButton(const Button& config);
    void removeButton(Button* button);
    void setCallback(Button* button, std::function<void(Button*)> callback);
    void setColor(Button* button, glm::vec4 color);
    glm::vec4 getColor(Button* button);
    
    void fingerStart(float x, float y);
    bool fingerRelease(float x, float y);
    
    void drawButtons();
    
private:
    bool isInsideButton(const Button* button, float x, float y);
    
    TextRenderer* textRenderer;
    Renderer2D* renderer2d;
    std::vector<Button*> buttons;
    Button* activeButton = nullptr;
};