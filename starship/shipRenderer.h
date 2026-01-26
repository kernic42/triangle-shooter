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
#include <emscripten/emscripten.h>
#include "shaders.hpp"

class ShipRenderer {
    private:

    // screen settings
    float aspect = 0;
    float width = 0;
    float height = 0;

    // grid OpenGL stuff
    GLuint gridShader, gridVAO, gridVBO;
    GLuint gridRotationLoc, gridProjectionLoc;
    GLuint gridVertexCount;

    // cell hull OpenGL stuff
    GLuint hullShader, hullVAO, hullVBO, hullAtlasTexture, hullCrackAtlasTexture;
    GLuint hullTransformLoc, hullTexCoordsLoc, hullColorsLoc, hullProjectionLoc, hullShipRotationLoc, hullLocalRotationLoc, hullAtlasLoc, hullAtlasCrackLoc, hullBorderWidthLoc, hullTimeLoc;

    // cannon OpenGL stuff
    GLuint cannonShader, cannonVAO, cannonVBO, cannonPositionsVBO, cannonTexture;
    GLuint cannonAngleLoc, cannonShipRotationLoc, cannonProjectionLoc, cannonTextureLoc, cannonGridDimensionsLoc;
    glm::vec2 cannonGridDimensions = glm::vec2(2.0, 2.0);
    static const int MAX_CANNONS = 256;
    int cannonCount = 0;

    // bullet OpenGL stuff
    GLuint bulletShader, bulletVAO, bulletVBO, bulletAttributesVBO, bulletTexture;
    GLuint bulletTimeLoc, bulletGridDimensionsLoc, bulletTextureLoc;
    glm::vec2 bulletGridDimensions = glm::vec2(1.0, 1.0);
    int bulletCount = 0;

    public:

    ShipRenderer();
    ~ShipRenderer();

    // init
    void setAspect(float width, float height);

    // OpenGL Cell Grid Hull Part
    void initGrid();
    void drawGrid(float rotation);

    // OpenGL Cells Hull Part               
    void initCellRendering();
    void renderCells(std::vector<float> shipsRotation);
    void updateCellUniforms(shipData_t *ships, int shipCount);

    // OpenGL Cannons Part
    void initCannons();
    void renderCannons(glm::vec2 cursorPos, std::vector<float> shipsRotation);
    void updateCannonPositions(shipData_t *ships, int shipCount);

    // OpenGL Bullets Part
    void initBullets();
    void renderBullets();
    void updateBullets(shipData_t *ships, int shipCount);
    
    // render part
    void render(shipData_t *ships, int count, glm::vec2 cursorPos); // we use ptr to put all data from ships structs in a same contiguous array, that can be passed with a single glDrawArraysInstanced call
};