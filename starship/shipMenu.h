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
#include "shaders.hpp"
#include <emscripten/emscripten.h>

class ShipMenu {
    public:

    GLuint cellAtlasTexture = 0;
    GLuint crackAtlasTexture = 0;
    GLuint localRotationLoc = 0;

    GLuint cursorTriangleAtlasCrackLoc = 0;
    GLuint cursorTriangleAtlasLoc = 0;

    // cell menu
    GLuint cellMenuVAO = 0, cellMenuVBO = 0, cellMenuShader = 0;
    GLuint menuTransformsLoc, menuTexCoordsLoc, menuColorsLoc, menuProjectionLoc, menuShipRotationLoc, menuAtlasLoc, menuAtlasCrackLoc, menuTimeLoc, menuBorderWidthLoc;

    GLuint triangleAtCursorProgram = 0, cursorTriangleVAO = 0, cursorTriangleVBO = 0;
    GLuint cursorTriangleTransformsLoc, cursorTriangleTexCoordsLoc, cursorTriangleColorsLoc, cursorTriangleProjectionLoc, cursorTriangleShipRotationLoc, cursorTriangleTimeLoc, cursorTriangleBorderWidthLoc;
     
    typedef struct {
        std::vector<glm::mat4> transforms;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec4> colors;
    } cellMenu_t;

    typedef struct {
        bool canDrawTriangleAtCursor = false;
        CellName type  = CELL_NONE;
        int cannonCount = 0;
        int buttonId = -1;
    } menuCursorSelection;
    
    cellMenu_t cellMenu;
    cellMenu_t cursorTriangleCell;
    menuCursorSelection menuCursorSelect;

    glm::vec2 anchor;
    float backWidth;
    float backHeight;

    float width;
    float height;
    float aspect;
    float cursorX;
    float cursorY;

    // button manager
    std::vector<Button*> buttons;
    ButtonManager *buttonManager;
    Renderer2D* renderer2d;
    
    bool isCursorInsideMenu(glm::vec2 cursorPos);
    void unselectButtons();

    CellTexCoords getRandomAtlasCoords(AtlasSprite sprite, int cellNumber);
    void setAspect(float width, float height);
    void setCursor(glm::vec2 cursorPos);
    void screenResize(float width, float height);
    void init();

    // triangle drawn under cursor for menu
    void initTriangleAtCursor();
    void drawTriangleAtCursor();

    // menu triangle inside buttons
    void initMenuTriangle();
    void drawMenuTriangle();
    void updateMenuTriangle();
    void newMenuTriangle(CellName name, int i, float x, float y);

    // menu buttons
    void setButtonManager(ButtonManager *buttonManager, Renderer2D *renderer2d);
    void createMenuButtons(CellCategory category); // decides what menu spawn, by category

    // get access to starship class, call newAttackCell()
    void click();
    void render();
};