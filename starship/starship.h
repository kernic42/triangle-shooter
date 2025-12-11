#pragma once

#include <vector>
#include <cstdint>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

class Starship {
public:
    enum CellCategory {
        CELL_ATTACK,
        CELL_DEFENSE,
        CELL_UTILITY,
        CELL_JET,
        CELL_CUSTOM
    };

    enum CellName {
        // Attack
        CELL_FIRE,
        CELL_ICE,
        CELL_RADIOACTIVE,
        CELL_PROJECTILE_GUN,
        CELL_LASER_GUN,
        CELL_MISSILE_GUN,
        CELL_PLASMA_GUN,
        CELL_RAPID_FIRE_PROJECTILE,

        // Defense
        CELL_KINETIC_BARRIER,
        CELL_ENERGY_SHIELD,
        CELL_HYBRID_SHIELD,
        CELL_REFLECTIVE_SHIELD,
        CELL_REGEN_SHIELD,
        CELL_SPIKE_ARMOR,
        CELL_CLOAKING_FIELD,
        CELL_FORCE_BUBBLE,

        // Utility
        CELL_SENSOR,
        CELL_REPAIR_DRONE,
        CELL_SCANNER,
        CELL_JAMMER,
        CELL_CARGO_HOLD,
        CELL_BATTERY,
        CELL_ANALYZER,
        CELL_ENERGY_CORE,

        // Jet
        CELL_FORWARD_THRUST_JET,
        CELL_OMNI_BOOST_JET,
        CELL_TURN_JET,
        CELL_BURST_JET,
        CELL_EFFICIENCY_JET,
        CELL_OVERDRIVE_JET,
        CELL_STABILIZER_JET,

        // Custom
        CELL_HOMING_MISSILE,
        CELL_AREA_DENIAL_MINE,
        CELL_STEAM_LASER,
        CELL_SWITCH_BLASTER
    };

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

    struct CellTexCoords {
        float u0, v0;  // bottom-left
        float u1, v1;  // bottom-right
        float u2, v2;  // top-right
        float pad0, pad1;  // padding to align to 32 bytes (std140)
    };

    enum AtlasSprite {
        ATLAS_FIRE = 0,
        ATLAS_ICE = 1,
        ATLAS_RADIOACTIVE = 2,
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

    // Grid settings
    int gridWidth = 9;
    int gridHeight = 9;
    float cellSize = 0.120;
    float originX = -(gridWidth*cellSize)/2.0;
    float originY = -(gridHeight*cellSize)/2.0;

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

    // Uniform locations
    GLint transformsLoc = -1;
    GLint texCoordsLoc = -1;
    GLint colorsLoc = -1;
    GLint projectionLoc = -1;
    GLint shipRotationLoc = -1;
    GLint atlasLoc = -1;
    GLint atlasCrackLoc = -1;

    GLuint cannonVAO;
    GLuint cannonVBO;
    GLuint cannonTexture;
    GLuint cannonShader;

    // In Starship class header
    static const int MAX_CANNONS = 256;
    GLint uCannonPositionsLoc;
    GLint uCannonAngleLoc;
    GLint uShipRotationLoc;
    GLint uProjectionLoc;
    GLint uTextureLoc;
    int cannonCount = 0;

    float cursorX = 0;
    float cursorY = 0;
    float aspect = 0;


    Starship();
    ~Starship();

    CellTexCoords getRandomAtlasCoords(AtlasSprite sprite, int cellNumber);
    
    void setAspect(float aspect);
    void updateCannonPositions();
    void initCannons();
    void renderCannons();
    void initCellMiddlePoints();
    void initStarshipCells();

    void initCellRendering();
    void updateCellUniforms();
    void newAttackCell(CellName name, int cellNumber);
    void drawCells();

    void initGrid();
    void drawGrid();
    void cleanupGrid();

    // Input handlers
    void onMouseDown(int button, float x, float y);
    void onMouseUp(int button, float x, float y);
    void onMouseMove(float x, float y);
};