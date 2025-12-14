// Button.cpp
#include "Button.h"
#include <algorithm>

ButtonManager::ButtonManager() : textRenderer(nullptr), renderer2d(nullptr), activeButton(nullptr) {
}

ButtonManager::~ButtonManager() {
    for (auto* btn : buttons) {
        delete btn;
    }
    buttons.clear();
}

void ButtonManager::init(TextRenderer* textRenderer, Renderer2D* renderer2d) {
    this->textRenderer = textRenderer;
    this->renderer2d = renderer2d;
}

bool ButtonManager::isInsideButton(const Button* button, float x, float y) {
    return (x >= button->x &&
            x <= button->x + button->width &&
            y >= button->y &&
            y <= button->y + button->height);
}

Button* ButtonManager::createButton(const Button& config) {
    Button* newButton = new Button(config);
    buttons.push_back(newButton);
    return newButton;
}

void ButtonManager::setCallback(Button* button, std::function<void(Button*)> callback) {
    if (button) {
        button->callback = callback;
    }
}

void ButtonManager::removeButton(Button* button) {
    for (size_t i = 0; i < buttons.size(); ++i) {
        if (buttons[i] == button) {
            delete buttons[i];
            buttons.erase(buttons.begin() + i);
            if (activeButton == button) {
                activeButton = nullptr;
            }
            break;
        }
    }
}

void ButtonManager::fingerStart(float x, float y) {
    activeButton = nullptr;
    for (auto* btn : buttons) {
        if (isInsideButton(btn, x, y)) {
            activeButton = btn;
            break;
        }
    }
}

bool ButtonManager::fingerRelease(float x, float y) {
    if (activeButton == nullptr) {
        return false;
    }
    
    if (isInsideButton(activeButton, x, y)) {
        if (activeButton->callback) {
            activeButton->callback(activeButton);
        }
        activeButton = nullptr;
        return true;
    }
    
    activeButton = nullptr;
    return false;
}

void ButtonManager::drawButtons() {
    // Draw backgrounds
    for (auto* button : buttons) {
        if (button->borderRadius > 0) {
            renderer2d->drawFilledRoundedRect(
                glm::vec2(button->x, button->y),
                button->width,
                button->height,
                button->borderRadius,
                button->color
            );
            
            if (button->borderWidth > 0) {
                renderer2d->drawRoundedRect(
                    glm::vec2(button->x, button->y),
                    button->width,
                    button->height,
                    button->borderWidth,
                    button->borderRadius,
                    button->borderColor
                );
            }
        } else {
            renderer2d->drawFilledRect(
                glm::vec2(button->x, button->y),
                button->width,
                button->height,
                button->color
            );
            
            if (button->borderWidth > 0) {
                renderer2d->drawRect(
                    glm::vec2(button->x, button->y),
                    button->width,
                    button->height,
                    button->borderWidth,
                    button->borderColor
                );
            }
        }
    }
    
    renderer2d->flush();
    
    // Draw text and images
    for (auto* button : buttons) {
        float textWidth, textHeight, textAscent, textDescent;
        textRenderer->getStringMetrics(button->text, button->textScale, textWidth, textHeight, textAscent, textDescent);
        
        if (button->textureId != 0) {
            float textPosX, textPosY;
            float imagePosX, imagePosY;
            
            if (button->drawImage == "top") {
                float centeringBoxHeight = button->imageHeight + textHeight + button->imageGap;
                float centeringBoxWidth = button->imageWidth > textWidth ? button->imageWidth : textWidth;
                float drawBoxStartX = button->x + button->width / 2 - centeringBoxWidth / 2;
                float drawBoxStartY = button->y + button->height / 2 - centeringBoxHeight / 2;
                
                float imgBoxDiff = centeringBoxWidth - button->imageWidth;
                imagePosX = drawBoxStartX + imgBoxDiff / 2;
                imagePosY = drawBoxStartY + centeringBoxHeight - button->imageHeight;
                
                float textBoxDiff = centeringBoxWidth - textWidth;
                textPosX = drawBoxStartX + textBoxDiff / 2;
                textPosY = drawBoxStartY;
            }
            else if (button->drawImage == "left") {
                float centeringBoxHeight = button->imageHeight > textHeight ? button->imageHeight : textHeight;
                float centeringBoxWidth = button->imageWidth + textWidth + button->imageGap;
                float drawBoxStartX = button->x + button->width / 2 - centeringBoxWidth / 2;
                float drawBoxStartY = button->y + button->height / 2 - centeringBoxHeight / 2;
                
                float imgBoxDiff = centeringBoxHeight - button->imageHeight;
                imagePosX = drawBoxStartX;
                imagePosY = drawBoxStartY + imgBoxDiff / 2;
                
                float textBoxDiff = centeringBoxHeight - textHeight;
                textPosX = drawBoxStartX + centeringBoxWidth - textWidth;
                textPosY = drawBoxStartY + textBoxDiff / 2;
            }
            else { // "center"
                float imgBoxDiffX = button->width - button->imageWidth;
                float imgBoxDiffY = button->height - button->imageHeight;
                imagePosX = button->x + imgBoxDiffX / 2;
                imagePosY = button->y + imgBoxDiffY / 2;
                
                float textBoxDiffX = button->width - textWidth;
                float textBoxDiffY = button->height - textHeight;
                textPosX = button->x + textBoxDiffX / 2;
                textPosY = button->y + textBoxDiffY / 2;
            }
            
            if (!button->text.empty()) {
                textRenderer->draw(button->text, textPosX, textPosY, button->textScale, button->textColor);
            }
            
            renderer2d->drawImage(button->textureId, imagePosX, imagePosY, button->imageWidth, button->imageHeight);
        }
        else {
            float textPosX = (button->x + button->width / 2) - textWidth / 2;
            float textPosY = (button->y + button->height / 2) - textHeight / 2;
            
            textRenderer->draw(button->text, textPosX, textPosY, button->textScale, button->textColor);
        }
    }
    
    renderer2d->flush();
    textRenderer->flush();
}

void ButtonManager::setColor(Button* button, glm::vec4 color) {
    if (button) {
        button->color = color;
    }
}

glm::vec4 ButtonManager::getColor(Button* button) {
    if (button) {
        return button->color;
    }
    return glm::vec4(0, 0, 0, 0);
}