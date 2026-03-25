#include "shipRenderer.h"
#include "stbImage/stb_image.h"

extern glm::mat4 projection;
extern glm::mat4 projView;
extern glm::mat4 shipModel;

void ShipRenderer::setAspect(float width, float height) {
    this->aspect = width / height;
    this->width = width;
    this->height = height;
}

ShipRenderer::ShipRenderer() {

}

ShipRenderer::~ShipRenderer() {

}

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}


///////////////////////////////////////////////////////////////////////////////////////////////
/////////                      OpenGL Cell Grid Hull Part                              ////////
///////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderer::initGrid() {
    // Create shader program
    GLuint vert = compileShader(GL_VERTEX_SHADER, gridVertexShader);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, gridFragmentShader);
    gridShader = glCreateProgram();
    glAttachShader(gridShader, vert);
    glAttachShader(gridShader, frag);
    glLinkProgram(gridShader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    gridRotationLoc = glGetUniformLocation(gridShader, "uRotation");
    gridProjectionLoc = glGetUniformLocation(gridShader, "uProjection");

    // Build line vertices
    std::vector<float> vertices;

    // 1. Vertical lines
    for (int i = 0; i <= gridWidth; i++) {
        float x = originX + i * cellSize;
        float y0 = originY;
        float y1 = originY + gridHeight * cellSize;
        vertices.push_back(x);  vertices.push_back(y0);
        vertices.push_back(x);  vertices.push_back(y1);
    }

    // 2. Horizontal lines
    for (int j = 0; j <= gridHeight; j++) {
        float y = originY + j * cellSize;
        float x0 = originX;
        float x1 = originX + gridWidth * cellSize;
        vertices.push_back(x0); vertices.push_back(y);
        vertices.push_back(x1); vertices.push_back(y);
    }

    // 3. Diagonal lines (bottom-left to top-right of each cell)
    for (int i = 0; i < gridWidth; i++) {
        for (int j = 0; j < gridHeight; j++) {
            float x0 = originX + i * cellSize;
            float y0 = originY + j * cellSize;
            float x1 = originX + (i + 1) * cellSize;
            float y1 = originY + (j + 1) * cellSize;
            vertices.push_back(x0); vertices.push_back(y0);
            vertices.push_back(x1); vertices.push_back(y1);
        }
    }

    gridVertexCount = vertices.size() / 2;

    // Create VAO/VBO
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void ShipRenderer::drawGrid(float rotation) {
    glUseProgram(gridShader);

    float c = cosf(rotation);
    float s = sinf(rotation);
    float rotationMatrix[9] = {
        c,  s,  0.0f,
       -s,  c,  0.0f,
        0.0f, 0.0f, 1.0f
    };

    glUniformMatrix3fv(gridRotationLoc, 1, GL_FALSE, rotationMatrix);

    glm::mat4 transform = projView * shipModel;
    glUniformMatrix4fv(gridProjectionLoc, 1, GL_FALSE, glm::value_ptr(transform));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertexCount);
    glBindVertexArray(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////                        OpenGL Cells Hull Part                                ////////
///////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderer::initCellRendering() {
    // Compile shader
    GLuint vert = compileShader(GL_VERTEX_SHADER, cellVertexShader2);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, cellFragmentShader);
    hullShader = glCreateProgram();
    glAttachShader(hullShader, vert);
    glAttachShader(hullShader, frag);
    glLinkProgram(hullShader);
    glDeleteShader(vert);
    glDeleteShader(frag);
    
    // Get uniform locations
    hullProjectionLoc = glGetUniformLocation(hullShader, "uProjection");
    hullShipRotationLoc = glGetUniformLocation(hullShader, "uShipRotation");
    hullLocalRotationLoc = glGetUniformLocation(hullShader, "uLocalRotation");
    hullAtlasLoc = glGetUniformLocation(hullShader, "uAtlas");
    hullAtlasCrackLoc = glGetUniformLocation(hullShader, "uCrackTex");
    hullBorderWidthLoc = glGetUniformLocation(hullShader, "uBorderWidth");
    hullTimeLoc = glGetUniformLocation(hullShader, "uTime");
    
    // Create triangle VAO/VBO
    float half = cellSize / 2.0f;
    float triangleVerts[] = {
        -half, -half,
         half, -half,
         half,  half
    };
    //////////////////////////////////////////////////////
    //                set hull vao and vbo              //
    //////////////////////////////////////////////////////
    
    // gen hull
    glGenVertexArrays(1, &hullVAO);
    glGenBuffers(1, &hullVBO);
    glBindVertexArray(hullVAO);

    // set aPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // upload aPos once
    glBindBuffer(GL_ARRAY_BUFFER, hullVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVerts), triangleVerts, GL_STATIC_DRAW);

    //////////////////////////////////////////////////////
    //           set hullSettings vao and vbo           //
    //////////////////////////////////////////////////////

    // gen hullSettings
    glGenBuffers(1, &hullSettingsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, hullSettingsVBO);

    // set aColor
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(hullSettings_t), (void*)offsetof(hullSettings_t, aColor));
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(1);

    // set aTexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(hullSettings_t), (void*)offsetof(hullSettings_t, aTexCoord));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(hullSettings_t), (void*)(offsetof(hullSettings_t, aTexCoord) + sizeof(float) * 2));
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(hullSettings_t), (void*)(offsetof(hullSettings_t, aTexCoord) + sizeof(float) * 4));
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
 // i think it takes 3 loc not 2 for mat3x2
    // set aTransform
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(hullSettings_t), (void*)offsetof(hullSettings_t, aTransform));
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(hullSettings_t), (void*)(offsetof(hullSettings_t, aTransform) + sizeof(float) * 4));
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(hullSettings_t), (void*)(offsetof(hullSettings_t, aTransform) + sizeof(float) * 8));
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(hullSettings_t), (void*)(offsetof(hullSettings_t, aTransform) + sizeof(float) * 12));
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);
    glEnableVertexAttribArray(7);
    glEnableVertexAttribArray(8);   
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);

   // layout(location = 0) in vec2 aPos;
   // layout(location = 1) in vec4 aColor;
   // layout(location = 2) in mat3x2 aTexCoord;
   // layout(location = 4) in mat4 aTransform;

    glBindVertexArray(0);
    
    // Load atlas texture
    hullAtlasTexture = loadTexture("atlas.png");
    hullCrackAtlasTexture = loadTexture("crack_mask.png");
    printf("crack texture ID: %u\n", hullCrackAtlasTexture);
}

void ShipRenderer::renderCells(std::vector<float> shipsRotation) {
    if (cannonCount == 0) return;

    glUseProgram(hullShader);

    float borderWidth = 0.012;
    glUniform1f(hullBorderWidthLoc, borderWidth);
    
    // init with no rot
    glUniformMatrix4fv(hullLocalRotationLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));

    // Projection
    glm::mat4 transform = projView * shipModel;
    glUniformMatrix4fv(hullProjectionLoc, 1, GL_FALSE, glm::value_ptr(transform));
    
    // Ship rotation
    float currentRotation = shipsRotation[0]; // just use first rotation for now
    float c = cosf(currentRotation);          // prob replace with glm::mat4
    float s = sinf(currentRotation);
    float rotationMatrix[9] = {
        c,  s,  0.0f,
       -s,  c,  0.0f,
        0.0f, 0.0f, 1.0f
    };
    glUniformMatrix3fv(hullShipRotationLoc, 1, GL_FALSE, rotationMatrix);

    glUniform1f(hullTimeLoc, emscripten_get_now() / 1000.0f);
    
    // Bind atlas
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hullAtlasTexture);
    glUniform1i(hullAtlasLoc, 0);

    // Bind crack atlas
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, hullCrackAtlasTexture);
    glUniform1i(hullAtlasCrackLoc, 1);
    
    // Draw
    glBindVertexArray(hullVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, cannonCount);
    glBindVertexArray(0);
}


void ShipRenderer::updateCellUniforms(shipData_t *ships, int shipCount) {
    glUseProgram(hullShader);
    glBindVertexArray(hullVAO);
    
    // need to iterate through all ships and copy their data into contiguous vector to send to gpu
    std::vector<hullSettings_t> hullSettings;
    
    int aliveCount = 0;

    for(size_t i = 0; i < shipCount; ++i) {
        shipData_t &ship = ships[i];

        for(int j = 0; j < ship.cellHullData.count; ++j) { // maybe put the count as a global to share between hull and cannons instead in the struct
            hullSettings_t hull;
            hull.aTexCoord = ship.cellHullData.texCoords[j];
            hull.aTransform = glm::mat4(1.0f); // identity
            hull.aColor = ship.cellHullData.colors[j];
            hullSettings.push_back(hull);

            aliveCount++;
        }
    }
    // I think aliveCount and cannonCount is the same duplicate variable.....
    glBindBuffer(GL_ARRAY_BUFFER, hullSettingsVBO); // I think vao already bound but in case
    glBufferData(GL_ARRAY_BUFFER, aliveCount * sizeof(hullSettings_t), &hullSettings[0], GL_STATIC_DRAW);
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////                            OpenGL Cannons Part                               ////////
///////////////////////////////////////////////////////////////////////////////////////////////

// need to send cannon mat4 with glVertexAttribDivisor, 1 per cannon quad
// need to control orientation and setup of this mat4 from ship, ship stores pivot offset and size x,y, then reconstruct mat4 and setup send to gpu

void ShipRenderer::initCannons() {                                 
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);                 
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);               
    std::string cannonVertShader = loadTextFile("shaders/cannon.vert");
    std::string cannonFragShader = loadTextFile("shaders/cannon.frag");
    const char* cannonFragShaderChar = cannonFragShader.c_str();
    const char* cannonVertShaderChar = cannonVertShader.c_str();
    glShaderSource(vs, 1, &cannonVertShaderChar, nullptr);         
    glShaderSource(fs, 1, &cannonFragShaderChar, nullptr);        
    glCompileShader(vs);
    glCompileShader(fs);
    cannonShader = glCreateProgram();
    glAttachShader(cannonShader, vs);
    glAttachShader(cannonShader, fs);
    glLinkProgram(cannonShader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Cache uniform locations
    cannonAngleLoc = glGetUniformLocation(cannonShader, "uCannonAngle");
    cannonShipRotationLoc = glGetUniformLocation(cannonShader, "uShipRotation");
    cannonProjectionLoc = glGetUniformLocation(cannonShader, "uProjection");
    cannonTextureLoc = glGetUniformLocation(cannonShader, "uTexture");
    cannonGridDimensionsLoc = glGetUniformLocation(cannonShader, "uGridDimensions");

    glUseProgram(cannonShader);

    // set cannon grid dimensions once
    glUniform2fv(cannonGridDimensionsLoc, 1, glm::value_ptr(cannonGridDimensions));

    // Set projection once
    glUniformMatrix4fv(cannonProjectionLoc, 1, GL_FALSE, glm::value_ptr(projView));

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
    glGenBuffers(1, &cannonPositionsVBO);

    glBindVertexArray(cannonVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cannonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cannonQuad), cannonQuad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(CannonVertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CannonVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    int cannonStrideSize = sizeof(glm::vec2) + sizeof(float); // cannon position x,y + cannon textureID

    glBindBuffer(GL_ARRAY_BUFFER, cannonPositionsVBO);
    glVertexAttribDivisor(2, 1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, cannonStrideSize, (void*)0);
    glEnableVertexAttribArray(2);

    glVertexAttribDivisor(3, 1);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, cannonStrideSize, (void*)(sizeof(glm::vec2)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    cannonTexture = loadTextureBlurry("cannon.png");
}

void ShipRenderer::updateCannonPositions(shipData_t *ships, int shipCount) {
    // iterate through cannons and put data in contiguous array
    typedef struct {
        glm::vec2 position;
        float color;
    } cannonStride;

    std::vector<cannonStride> strides;

    //if (cells[i].cellAlive) { but done in the ship game logic instead
    for(int i = 0; i < shipCount; ++i) {
        shipData_t ship = ships[i];
        cannonData_t cannonData = ship.cannonData;

        for(int j = 0; j < cannonData.count; ++j) {
            cannonStride stride;
            stride.position = cannonData.pos[j];
            stride.color = (float)cannonData.colors[j];

            strides.push_back(stride); // prob really slow with push back
        }
    }

    cannonCount = strides.size();

    glBindBuffer(GL_ARRAY_BUFFER, cannonPositionsVBO);
    glBufferData(GL_ARRAY_BUFFER, cannonCount * sizeof(cannonStride), &strides[0], GL_STATIC_DRAW);
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

    // Set projection once
    glUseProgram(cannonShader);
    glBindVertexArray(cannonVAO);

    glm::mat4 transform = projView * shipModel;
    glUniformMatrix4fv(cannonProjectionLoc, 1, GL_FALSE, glm::value_ptr(transform));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cannonTexture);
    glUniform1i(cannonTextureLoc, 1);

    // Upload angle
    glUniform1f(cannonAngleLoc, cannonAngle);
    glUniform1f(cannonShipRotationLoc, shipRot);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, cannonCount);
    glBindVertexArray(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////                            OpenGL Bullets Part                               ////////
///////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderer::initBullets() {                                 
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);                 
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);               
    std::string cannonVertShader = loadTextFile("shaders/bullet.vert");
    std::string cannonFragShader = loadTextFile("shaders/bullet.frag");
    const char* cannonFragShaderChar = cannonFragShader.c_str();
    const char* cannonVertShaderChar = cannonVertShader.c_str();
    glShaderSource(vs, 1, &cannonVertShaderChar, nullptr);         
    glShaderSource(fs, 1, &cannonFragShaderChar, nullptr);        
    glCompileShader(vs);
    glCompileShader(fs);
    bulletShader = glCreateProgram();
    glAttachShader(bulletShader, vs);
    glAttachShader(bulletShader, fs);
    glLinkProgram(bulletShader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    bulletTexture = loadTexture("bullet.png");

    glGenVertexArrays(1, &bulletVAO);
    glGenBuffers(1, &bulletVBO);
    glGenBuffers(1, &bulletAttributesVBO);

    glUseProgram(bulletShader);
    glBindVertexArray(bulletVAO);

    // get uniform locations
    bulletTimeLoc = glGetUniformLocation(bulletShader, "uTime");
    bulletGridDimensionsLoc = glGetUniformLocation(bulletShader, "uGridDimensions");
    bulletProjectionLoc = glGetUniformLocation(bulletShader, "uProjection");
    bulletTextureLoc = glGetUniformLocation(bulletShader, "uTexture");

    // upload uGridDimensions once
    glUniform2fv(bulletGridDimensionsLoc, 1, &bulletGridDimensions[0]);

    // upload projection once
    glUniformMatrix4fv(bulletProjectionLoc, 1, GL_FALSE,  glm::value_ptr(projView));

    // static data
    int staticStrideSize = 4 * sizeof(float);
    glBindBuffer(GL_ARRAY_BUFFER, bulletVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, staticStrideSize, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, staticStrideSize, (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    float s = 0.004f;  // adjust to taste
    float quad[] = {
        // triangle 1
        -s, -s,  0.0f, 0.0f,
        s, -s,  1.0f, 0.0f,
        s,  s,  1.0f, 1.0f,
        // triangle 2
        -s, -s,  0.0f, 0.0f,
        s,  s,  1.0f, 1.0f,
        -s,  s,  0.0f, 1.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    // dynamic data
    glBindBuffer(GL_ARRAY_BUFFER, bulletAttributesVBO);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(bulletData_t), (void*)offsetof(bulletData_t, origin));
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(bulletData_t), (void*)offsetof(bulletData_t, direction));
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(bulletData_t), (void*)offsetof(bulletData_t, shipTranslate));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(bulletData_t), (void*)offsetof(bulletData_t, shipRotation));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(5);

    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(bulletData_t), (void*)offsetof(bulletData_t, velocity));
    glVertexAttribDivisor(6, 1);
    glEnableVertexAttribArray(6);

    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(bulletData_t), (void*)offsetof(bulletData_t, gridIndex));
    glVertexAttribDivisor(7, 1);
    glEnableVertexAttribArray(7);

    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(bulletData_t), (void*)offsetof(bulletData_t, startTime));
    glVertexAttribDivisor(8, 1);
    glEnableVertexAttribArray(8);

    glBindVertexArray(0);
}

void ShipRenderer::renderBullets() {
    glUseProgram(bulletShader);
    glBindVertexArray(bulletVAO);

    glUniform1f(bulletTimeLoc, emscripten_get_now() / 1000.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bulletTexture);
    glUniform1i(bulletTextureLoc, 0);

    // upload projView
    glUniformMatrix4fv(bulletProjectionLoc, 1, GL_FALSE,  glm::value_ptr(projView));


    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, bulletCount);

    glBindVertexArray(0);
}

void ShipRenderer::updateBullets(shipData_t *ships, int shipCount) {
    shipData_t &ship = ships[0];
    // copy bullets contained within ship inside vertex attributes

    //layout(location = 2) in vec2 aOrigin; // per bullet
    //layout(location = 3) in vec2 aDirection; // per bullet
    //layout(location = 4) in float aVelocity; // per bullet
    //layout(location = 5) in float aGridIndex; // per bullet
    //layout(location = 6) in float aStartTime; // per bullet
    
    bulletCount = ship.bulletDataCount;// ship.bulletDataCount;

    glBindBuffer(GL_ARRAY_BUFFER, bulletAttributesVBO); 
    glBufferData(GL_ARRAY_BUFFER, bulletCount * sizeof(bulletData_t), &ship.bulletData[0], GL_STATIC_DRAW);

    glBindVertexArray(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////                             OpenGL Render Part                               ////////
///////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderer::render(shipData_t *ships, int count, glm::vec2 cursorPos) {
    // check if any config changed, if any of the ship config changed need to update the entire config in the gpu
    bool anyConfigChanged = false;

    for(int i = 0; i < count; ++i) {
        shipData_t &ship = ships[i];

        if(ship.configChanged) {
            ship.configChanged = false; // reset config flag
            anyConfigChanged = true; // detected any of the config flag was on
        }
    }

    if(anyConfigChanged) { // maybe just update the config for the thing that changed..
        updateCannonPositions(ships, count);
        updateCellUniforms(ships, count);
        updateBullets(ships, count);
        printf("ship config updated\n");
    }
    
    // fill the rotation and dynamic data for each shipID
    std::vector<float> shipsRotation;
    for(int i = 0; i < count; ++i) {
        shipData_t &ship = ships[i];
        shipsRotation.push_back(ship.shipRot);
    }

    // stuff like renderCannons should actually iterate through all the ships itself, and then you can extract the cannon angle, and the ship rot angle, inside the function instead of here

    // render cannon with gathered data from all the ship structs
    drawGrid(shipsRotation[0]); // ship is probably always stored as ship 0 and never gets destroyed, maybe add a property field for main ship
    renderCells(shipsRotation);
    renderCannons(cursorPos, shipsRotation);
    renderBullets();
}