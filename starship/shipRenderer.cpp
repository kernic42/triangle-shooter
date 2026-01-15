#include "shipRenderer.h"
#include "stbImage/stb_image.h"

extern glm::mat4 projection;

void ShipRenderer::setAspect(float width, float height) {
    this->aspect = width / height;
    this->width = width;
    this->height = height;
}

ShipRenderer::ShipRenderer(){

}

ShipRenderer::~ShipRenderer() {

}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////                        OpenGL Cells Hull Part                                ////////
///////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderer::renderCellHulls() {

}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////                            OpenGL Cannons Part                               ////////
///////////////////////////////////////////////////////////////////////////////////////////////

// need to send cannon mat4 with glVertexAttribDivisor, 1 per cannon quad
// need to control orientation and setup of this mat4 from ship, ship stores pivot offset and size x,y, then reconstruct mat4 and setup send to gpu

void ShipRenderer::initCannons() {                                 // this init cannons program, set the middle point in memory for each cannon cell that exists.
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);                  // the middle point of each cell can probably just be set everytime shipConfigChanged == true
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);                // the only thing it updated was the middle point, now that's also the only thing it needs to update
    glShaderSource(vs, 1, &cannonVertexShader, nullptr);           // so this init just set the quad for cannons(that we can later change aspect with mat4) from data contained in cannonData_t
    glShaderSource(fs, 1, &cannonFragmentShader, nullptr);         // so update the middle point to whatever ship game struct set when shipConfigChanged == true
    glCompileShader(vs);
    glCompileShader(fs);
    cannonShader = glCreateProgram();
    glAttachShader(cannonShader, vs);
    glAttachShader(cannonShader, fs);
    glLinkProgram(cannonShader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Cache uniform locations
    uCannonPositionsLoc = glGetUniformLocation(cannonShader, "uCannonPositions");
    uCannonAngleLoc = glGetUniformLocation(cannonShader, "uCannonAngle");
    uShipRotationLoc = glGetUniformLocation(cannonShader, "uShipRotation");
    uProjectionLoc = glGetUniformLocation(cannonShader, "uProjection");
    uTextureLoc = glGetUniformLocation(cannonShader, "uTexture");

    // Set projection once
    glUseProgram(cannonShader);
    glUniformMatrix4fv(uProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    struct CannonVertex {
        float x, y;
        float u, v;
    };

    float pivotOffset = 0.020f;

    float texWidth = 1600.0f;
    float texHeight = 500.0f;
    float aspect = texWidth / texHeight;

    float height = 0.030f;
    float width = 0.027 * aspect;

    CannonVertex cannonQuad[] = {
        {-pivotOffset,        -height,  0.0f, 0.0f},
        {width - pivotOffset, -height,  1.0f, 0.0f},
        {width - pivotOffset,  height,  1.0f, 1.0f},
        {-pivotOffset,        -height,  0.0f, 0.0f},
        {width - pivotOffset,  height,  1.0f, 1.0f},
        {-pivotOffset,         height,  0.0f, 1.0f},
    };

    glGenVertexArrays(1, &cannonVAO);
    glGenBuffers(1, &cannonVBO);
    glBindVertexArray(cannonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cannonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cannonQuad), cannonQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(CannonVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CannonVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    cannonTexture = loadTextureBlurry("cannon.png");
}

void ShipRenderer::updateCannonPositions(shipData_t *ships, int shipCount) {
    // iterate through cannons and put data in contiguous array
    std::vector<glm::vec2> cannonPositions;

    //if (cells[i].cellAlive) { but done in the ship game logic instead
    for(int i = 0; i < shipCount; ++i) {
        shipData_t ship = ships[i];
        cannonData_t cannonData = ship.cannonData;

        for(int j = 0; j < cannonData.count; ++j) {
            cannonPositions.push_back(cannonData.pos[j]);
        }
    }

    cannonCount = cannonPositions.size();

    glUseProgram(cannonShader);
    glUniform2fv(uCannonPositionsLoc, cannonCount, glm::value_ptr(cannonPositions[0]));
}

void ShipRenderer::renderCannons(glm::vec2 cursorPos, std::vector<float> shipsRotation) {
    if (cannonCount == 0) return;
    
    // Compute cannon angle toward cursor
    float shipRot = shipsRotation[0];  //for now use rotation for ship 0, but should encode each ship rotation and send it via uniform array

    float dirX = cursorPos.x * aspect; // prob needs to be a property inside ship struct instead
    float dirY = cursorPos.y;
    float cannonAngle = atan2f(dirY, dirX) - shipRot;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(cannonShader);
    glBindVertexArray(cannonVAO);

    // Set projection once
    glUseProgram(cannonShader);
    glUniformMatrix4fv(uProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cannonTexture);
    glUniform1i(uTextureLoc, 1);

    // Upload angle
    glUniform1f(uCannonAngleLoc, cannonAngle);

    // Ship rotation
    glm::mat3 rotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f), shipRot, glm::vec3(0.0f, 0.0f, 1.0f)));
    glUniformMatrix3fv(uShipRotationLoc, 1, GL_FALSE, glm::value_ptr(rotationMatrix));

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, cannonCount);
    glBindVertexArray(0);
}

void ShipRenderer::render(shipData_t *ships, int count, glm::vec2 cursorPos) {
    renderCellHulls();

    // check if any config changed, if any of the ship config changed need to update the entire config in the gpu
    bool anyConfigChanged = false;

    for(int i = 0; i < count; ++i) {
        shipData_t &ship = ships[i];

        if(ship.configChanged) {
            ship.configChanged = false; // reset config flag
            anyConfigChanged = true; // detected any of the config flag was on
        }
    }

    if(anyConfigChanged) {
        updateCannonPositions(ships, count);
        printf("ship config updated\n");
    }
    
    // fill the rotation and dynamic data for each shipID
    std::vector<float> shipsRotation;
    for(int i = 0; i < count; ++i) {
        shipData_t &ship = ships[i];
        shipsRotation.push_back(ship.shipRot);
    }

    // render cannon with gathered data from all the ship structs
    renderCannons(cursorPos, shipsRotation);
}