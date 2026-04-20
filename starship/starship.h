#pragma once

#include <vector>
#include <cstdint>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include "button/button.h"
#include "Renderer2D.h"
#include "shipRenderer.h"
#include "shipMenu.h"

class Starship {
public:
    int f = 0;
    std::vector<TriangleCell> cells;

    // Rotation state
    float currentRotation = 0.0f;      // stored rotation (radians)
    float dragStartRotation = 0.0f;    // rotation when drag started
    
    // Mouse state
    bool isDragging = false;
    float dragStartX = 0.0f;
    float dragStartY = 0.0f;

    float cursorX = 0;
    float cursorY = 0;
    float aspect = 0;
    float width = 0;
    float height = 0;

    // renderer2d, buttonManager
    ButtonManager *buttonManager;
    Renderer2D* renderer2d;

    // shipRenderer
    ShipRenderer shipRenderer;
    shipData_t shipData;

    // menu
    ShipMenu shipMenu;
    int energy = 10000;

    glm::vec2 shipTranslate = glm::vec2(0.0, 0.0);
    glm::vec2 translateSpeed = glm::vec2(0.0, 0.0);

    void draw();
    void init();
    Starship();
    ~Starship();

    void screenResize(float width, float height);
    void setButtonManager(ButtonManager *buttonManager, Renderer2D *renderer2d);

    CellTexCoords getRandomAtlasCoords(AtlasSprite sprite, int cellNumber);
    bool isCursorInsideCell(glm::vec2 cursor, glm::vec4 v1, glm::vec4 v2, glm::vec4 v3);
    int getSelectedCellId(glm::vec2 cursorPos);
    bool placeCell(glm::vec2 cursorPos);
    bool neighborsAlive(int cellId);
    int getPrice(CellName type, int cannonCount);
    
    void setAspect(float aspect, float width, float height);
    void initCellMiddlePoints();
    void initStarshipCells();

    void updateCannons();
    void updateHulls();
    void newAttackCell(CellName name, int cellNumber);

    void shotBullet();
    
    // Input handlers
    void onMouseDown(int button, float x, float y);
    void onMouseUp(int button, float x, float y);
    void onMouseMove(float x, float y);
};