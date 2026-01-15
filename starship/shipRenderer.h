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

    // cannon OpenGL stuff
    static const int MAX_CANNONS = 256;
    int cannonCount = 0;
    GLuint cannonVAO, cannonVBO, cannonTexture, cannonShader;
    GLuint projectionLoc, shipRotationLoc, atlasLoc, atlasCrackLoc, uCannonPositionsLoc, uCannonAngleLoc, uShipRotationLoc, uProjectionLoc, uTextureLoc;

    public:

    ShipRenderer();
    ~ShipRenderer();

    // init
    void setAspect(float width, float height);

    // OpenGL Cells Hull Part               
    void renderCellHulls();

    // OpenGL Cannons Part
    void initCannons();
    void renderCannons(glm::vec2 cursorPos, std::vector<float> shipsRotation);
    void updateCannonPositions(shipData_t *ships, int shipCount);
    
    // render part
    void render(shipData_t *ships, int count, glm::vec2 cursorPos); // we use ptr to put all data from ships structs in a same contiguous array, that can be passed with a single glDrawArraysInstanced call
};