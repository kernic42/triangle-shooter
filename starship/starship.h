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
    struct DefenseData {
        float regenRate;
        float maxStrength;
    };

    struct AttackData {
        float fireRate;
        float damage;
        float projectileSpeed;
    };

    struct UtilityData {
        float range;
        int capacity;
    };

    struct JetData {
        float thrust;
        float energyEfficiency;
    };

    struct CustomData {
        int customEffectID;
    };

    struct TriangleCell {
        CellCategory category;
        CellName name;
        bool cellAlive;
        int cellNumber;
        glm::vec2 middleOfTriangle;
        glm::mat4 transform;
        float x, y;
        CellTexCoords texCoords;
        AtlasSprite spriteName;
        glm::vec4 color;
        union {
            DefenseData defense;
            AttackData attack;
            UtilityData utility;
            JetData jet;
            CustomData custom;
        };
    };

    std::vector<TriangleCell> cells;

    // OpenGL stuff for grid
    GLuint gridVAO = 0;
    GLuint gridVBO = 0;
    GLuint gridShader = 0;
    GLint rotationUniformLoc = -1;
    GLint projectionUniformLoc = -1;
    int gridVertexCount = 0;

    // Rotation state
    float currentRotation = 0.0f;      // stored rotation (radians)
    float dragStartRotation = 0.0f;    // rotation when drag started
    
    // Mouse state
    bool isDragging = false;
    float dragStartX = 0.0f;
    float dragStartY = 0.0f;

    // Cell rendering
    GLuint cellShader = 0;
    GLuint cellVAO = 0;
    GLuint cellVBO = 0;
    GLuint cellAtlasTexture = 0;
    GLuint crackAtlasTexture = 0;
    GLuint localRotationLoc = 0;

    // Uniform locations
    GLint transformsLoc = -1;
    GLint texCoordsLoc = -1;
    GLint colorsLoc = -1;
    GLint projectionLoc = -1;
    GLint shipRotationLoc = -1;
    GLint atlasLoc = -1;
    GLint atlasCrackLoc = -1;

    float cursorX = 0;
    float cursorY = 0;
    float aspect = 0;
    float width = 0;
    float height = 0;

    // button manager
    std::vector<Button*> buttons;
    ButtonManager *buttonManager;
    Renderer2D* renderer2d;

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

    ShipRenderer shipRenderer;
    shipData_t shipData;

    static const int MAX_CANNONS = 256;
    int cannonCount = 0;

    ShipMenu shipMenu;
    int energy = 10000;

    void draw();
    void init();
    Starship();
    ~Starship();

    void screenResize(float width, float height);
    void updateMenuTriangle();

    void drawTriangleAtCursor();
    void updateCursorTriangle();

    void newMenuTriangle(CellName name, int i, float x, float y);
    void initTriangleAtCursor();
    void initMenuTriangle();
    void drawMenuTriangle();
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

    void updateCannonPositions();
    void updateCellUniforms();
    void newAttackCell(CellName name, int cellNumber);
    
    // Input handlers
    void onMouseDown(int button, float x, float y);
    void onMouseUp(int button, float x, float y);
    void onMouseMove(float x, float y);
};