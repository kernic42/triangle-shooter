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

    // Uniform locations
    GLint transformsLoc = -1;
    GLint texCoordsLoc = -1;
    GLint colorsLoc = -1;
    GLint projectionLoc = -1;
    GLint shipRotationLoc = -1;
    GLint atlasLoc = -1;
    GLint atlasCrackLoc = -1;

    // Cell rendering
    GLuint cellShader = 0;
    GLuint cellVAO = 0;
    GLuint cellVBO = 0;
    GLuint cellAtlasTexture = 0;
    GLuint crackAtlasTexture = 0;
    GLuint localRotationLoc = 0;

    // cell menu
    GLuint cellMenuVAO = 0;
    GLuint cellMenuVBO = 0;
    GLuint cellMenuShader = 0;
    GLuint menuTransformsLoc = 0, menuTexCoordsLoc = 0, menuColorsLoc = 0;
    GLuint menuProjectionLoc = 0, menuShipRotationLoc = 0;
    GLuint menuAtlasLoc = 0, menuAtlasCrackLoc = 0;
    GLuint menuTimeLoc = 0, menuBorderWidthLoc = 0;

    typedef struct {
        std::vector<glm::mat4> transforms;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec4> colors;
    } cellMenu_t;

    cellMenu_t cellMenu;
    cellMenu_t cursorTriangleCell;

    GLuint triangleAtCursorProgram = 0;

    GLuint cursorTriangleTransformsLoc = 0;
    GLuint cursorTriangleTexCoordsLoc = 0;
    GLuint cursorTriangleColorsLoc = 0;
    GLuint cursorTriangleProjectionLoc = 0;
    GLuint cursorTriangleShipRotationLoc = 0;
    GLuint cursorTriangleAtlasLoc = 0;
    GLuint cursorTriangleAtlasCrackLoc = 0;
    GLuint cursorTriangleTimeLoc = 0;
    GLuint cursorTriangleBorderWidthLoc = 0;

    GLuint cursorTriangleVAO = 0;
    GLuint cursorTriangleVBO = 0;

    typedef struct {
        bool canDrawTriangleAtCursor = false;
        CellName type  = CELL_NONE;
        int cannonCount = 0;
        int buttonId = -1;
    } menuCursorSelection;
    
    menuCursorSelection menuCursorSelect;

    glm::vec2 anchor;
    float backWidth;
    float backHeight;

    // button manager
    std::vector<Button*> buttons;
    ButtonManager *buttonManager;
    Renderer2D* renderer2d;

    float width;
    float height;
    float aspect;

    float cursorX;
    float cursorY;

    float cellSize = 0.120;
    
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