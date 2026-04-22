#include "shipRenderer.h"
#include "stbImage/stb_image.h"

extern glm::mat4 projection;
extern glm::mat4 projView;
extern glm::mat4 shipPos;

void ShipRenderer::setAspect(float width, float height) {
    this->aspect = width / height;
    this->width = width;
    this->height = height;
}

ShipRenderer::ShipRenderer() {

}

ShipRenderer::~ShipRenderer() {

}

void ShipRenderer::init() {
    this->initBullets();
    this->initGrid();
    this->initCannons();
    this->initHulls();
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
    gridProjectionLoc = glGetUniformLocation(gridShader, "uProjection");

    // Create VAO/VBO
    glGenVertexArrays(1, &gridVAO);
    glBindVertexArray(gridVAO);

    glGenBuffers(1, &gridVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void ShipRenderer::drawGrid(float rotation) {
    glm::mat4 shipRotation = glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 transform = projView * shipPos * shipRotation;

    glUseProgram(gridShader);

    glUniformMatrix4fv(gridProjectionLoc, 1, GL_FALSE, glm::value_ptr(transform));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertexCount);
    glBindVertexArray(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////                        OpenGL Cells Hull Part                                ////////
///////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderer::initHulls() {
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
    
    glGenVertexArrays(1, &hullVAO);
    glBindVertexArray(hullVAO);

    glGenBuffers(1, &hullVBO);
    glBindBuffer(GL_ARRAY_BUFFER, hullVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVerts), triangleVerts, GL_STATIC_DRAW);
    
    // set vertices
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //////////////////////////////////////
    //          set hullSettings        //
    //////////////////////////////////////
    glGenBuffers(1, &hullSettingsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, hullSettingsVBO);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(hullStride_t), (void*)offsetof(hullStride_t, color));
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(hullStride_t), (void*)offsetof(hullStride_t, texCoord));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(hullStride_t), (void*)(offsetof(hullStride_t, texCoord) + sizeof(float) * 2));
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(hullStride_t), (void*)(offsetof(hullStride_t, texCoord) + sizeof(float) * 4));
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(hullStride_t), (void*)offsetof(hullStride_t, model));
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(hullStride_t), (void*)(offsetof(hullStride_t, model) + sizeof(float) * 4));
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(hullStride_t), (void*)(offsetof(hullStride_t, model) + sizeof(float) * 8));
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(hullStride_t), (void*)(offsetof(hullStride_t, model) + sizeof(float) * 12));
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);
    glEnableVertexAttribArray(7);
    glEnableVertexAttribArray(8);   
    
    glBindVertexArray(0);
    
    // Load atlas texture
    hullAtlasTexture = loadTexture("atlas.png");
    hullCrackAtlasTexture = loadTexture("crack_mask.png");
    printf("crack texture ID: %u\n", hullCrackAtlasTexture);
}

void ShipRenderer::renderHulls(std::vector<float> shipsRotation) {
    if (cannonCount == 0) return;

    float borderWidth = 0.012;
    float currentRotation = shipsRotation[0]; // just use first rotation for now
    glm::mat4 shipRotation = glm::rotate(glm::mat4(1.0f), currentRotation, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 transform = projView * shipPos * shipRotation;

    glUseProgram(hullShader);

    glUniformMatrix4fv(hullProjectionLoc, 1, GL_FALSE, glm::value_ptr(transform));
    glUniform1f(hullBorderWidthLoc, borderWidth);
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

void ShipRenderer::updateHulls(shipData_t &shipData, std::vector<TriangleCell> &cells) {
    std::vector<hullStride_t> strides;

    for(int i = 0; i < cells.size(); ++i) {
        if(cells[i].cellAlive) {
            hullStride_t stride;
            stride.model = cells[i].transform;
            stride.color = glm::vec4(cells[i].color.r, cells[i].color.g, cells[i].color.b, cells[i].color.a);
            stride.texCoord = glm::mat3x2(glm::vec2(cells[i].texCoords.u0, cells[i].texCoords.v0),
                                                glm::vec2(cells[i].texCoords.u1, cells[i].texCoords.v1),
                                                glm::vec2(cells[i].texCoords.u2, cells[i].texCoords.v2));
            strides.push_back(stride);
        }
    }

    shipData.configChanged = true;
    shipData.hullStride = strides;
}

void ShipRenderer::updateHullsGpu(shipData_t *ships, int shipCount) {
    // need to iterate through all ships and copy their data into contiguous vector to send to gpu
    std::vector<hullStride_t> strides;

    for(int i = 0; i < shipCount; ++i) {
        shipData_t &ship = ships[i];   

        strides.insert(strides.end(), ship.hullStride.begin(), ship.hullStride.end());
    }

    glBindBuffer(GL_ARRAY_BUFFER, hullSettingsVBO);
    glBufferData(GL_ARRAY_BUFFER, strides.size() * sizeof(hullStride_t), &strides[0], GL_STATIC_DRAW);
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
    cannonProjectionLoc = glGetUniformLocation(cannonShader, "uProjection");
    cannonTextureLoc = glGetUniformLocation(cannonShader, "uTexture");
    cannonGridDimensionsLoc = glGetUniformLocation(cannonShader, "uGridDimensions");

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

    
    glUseProgram(cannonShader);

    // set cannon grid dimensions once
    glUniform2fv(cannonGridDimensionsLoc, 1, glm::value_ptr(cannonGridDimensions));

    glGenVertexArrays(1, &cannonVAO);
    glBindVertexArray(cannonVAO);

    glGenBuffers(1, &cannonVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cannonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cannonQuad), cannonQuad, GL_STATIC_DRAW);

    // set cannon vertices
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(CannonVertex), (void*)0);
    glEnableVertexAttribArray(0);

    // set cannons uvs
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CannonVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    ///////////////////////////////
    //      cannons positions    //
    ///////////////////////////////
    glGenBuffers(1, &cannonPositionsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cannonPositionsVBO);

    glVertexAttribDivisor(2, 1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(cannonStride_t), (void*)offsetof(cannonStride_t, position));
    glEnableVertexAttribArray(2);

    glVertexAttribDivisor(3, 1);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(cannonStride_t), (void*)offsetof(cannonStride_t, color));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    cannonTexture = loadTextureBlurry("cannon.png");
}

void ShipRenderer::updateCannons(shipData_t &shipData, std::vector<TriangleCell> &cells) {
    std::vector<cannonStride_t> strides;

    for (int i = 0; i < cells.size(); ++i) {
        if (cells[i].cellAlive) {
            cannonStride_t stride;
            stride.color = cells[i].name;
            stride.position = cells[i].middleOfTriangle;
            strides.push_back(stride);
        }
    }

    shipData.configChanged = true;
    shipData.cannonStride = strides;
}

void ShipRenderer::updateCannonsGpu(shipData_t *ships, int shipCount) {
    std::vector<cannonStride_t> strides;

    for(int i = 0; i < shipCount; ++i) {
        shipData_t &ship = ships[i];

        strides.insert(strides.end(), ship.cannonStride.begin(), ship.cannonStride.end());
    }

    glBindBuffer(GL_ARRAY_BUFFER, cannonPositionsVBO);
    glBufferData(GL_ARRAY_BUFFER, strides.size() * sizeof(cannonStride_t), &strides[0], GL_STATIC_DRAW);

    cannonCount = strides.size(); // global needs to be updated
}

void ShipRenderer::renderCannons(glm::vec2 cursorPos, std::vector<float> shipsRotation) {
    if (cannonCount == 0) return;
    
    // cannon angle toward cursor
    float shipRot = shipsRotation[0];  //for now use rotation for ship 0, but should encode each ship rotation and send it via uniform array
    float dirX = cursorPos.x * aspect; // prob needs to be a property inside ship struct instead
    float dirY = cursorPos.y;
    float cannonAngle = atan2f(dirY, dirX) - shipRot;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set transform
    glm::mat4 shipRotation = glm::rotate(glm::mat4(1.0f), shipRot, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 transform = projView * shipPos * shipRotation;

    glUseProgram(cannonShader);

    glUniformMatrix4fv(cannonProjectionLoc, 1, GL_FALSE, glm::value_ptr(transform));
    glUniform1f(cannonAngleLoc, cannonAngle);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cannonTexture);
    glUniform1i(cannonTextureLoc, 1);

    glBindVertexArray(cannonVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, cannonCount);
    glBindVertexArray(0);
}

///////////////////////////////////////////////////////////////////////////////////////////
/////////                            OpenGL Bullets                                ////////
///////////////////////////////////////////////////////////////////////////////////////////
/*
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

    // get uniform locations
    bulletTimeLoc = glGetUniformLocation(bulletShader, "uTime");
    bulletGridDimensionsLoc = glGetUniformLocation(bulletShader, "uGridDimensions");
    bulletProjectionLoc = glGetUniformLocation(bulletShader, "uProjection");
    bulletTextureLoc = glGetUniformLocation(bulletShader, "uTexture");

    glUseProgram(bulletShader);

    // upload once
    glUniform2fv(bulletGridDimensionsLoc, 1, &bulletGridDimensions[0]);

    glGenVertexArrays(1, &bulletVAO);
    glBindVertexArray(bulletVAO);

    glGenBuffers(1, &bulletVBO);
    glBindBuffer(GL_ARRAY_BUFFER, bulletVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    //////////////////////////////////
    //     static data(quad, uv)    //
    //////////////////////////////////
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    /////////////////////////////
    //      dynamic data       //
    /////////////////////////////
    glGenBuffers(1, &bulletAttributesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, bulletAttributesVBO);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(bulletStride_t), (void*)offsetof(bulletStride_t, origin));
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(bulletStride_t), (void*)offsetof(bulletStride_t, direction));
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(bulletStride_t), (void*)offsetof(bulletStride_t, shipTranslate));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(bulletStride_t), (void*)offsetof(bulletStride_t, shipRotation));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(5);

    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(bulletStride_t), (void*)offsetof(bulletStride_t, velocity));
    glVertexAttribDivisor(6, 1);
    glEnableVertexAttribArray(6);

    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(bulletStride_t), (void*)offsetof(bulletStride_t, gridIndex));
    glVertexAttribDivisor(7, 1);
    glEnableVertexAttribArray(7);

    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(bulletStride_t), (void*)offsetof(bulletStride_t, startTime));
    glVertexAttribDivisor(8, 1);
    glEnableVertexAttribArray(8);

    glBindVertexArray(0);

    bulletTexture = loadTexture("bullet.png");
}

void ShipRenderer::renderBullets() {
    glUseProgram(bulletShader);

    glUniformMatrix4fv(bulletProjectionLoc, 1, GL_FALSE,  glm::value_ptr(projView));
    glUniform1f(bulletTimeLoc, emscripten_get_now() / 1000.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bulletTexture);
    glUniform1i(bulletTextureLoc, 0);

    glBindVertexArray(bulletVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, bulletCount);
    glBindVertexArray(0);
}

void ShipRenderer::updateBullets(shipData_t *ships, int shipCount) {
    std::vector<bulletStride_t> strides;

    for(int i = 0; i < shipCount; ++i) {
        shipData_t &ship = ships[i];

        strides.insert(strides.end(), ship.bulletStride.begin(), ship.bulletStride.end());
    }

    glBindBuffer(GL_ARRAY_BUFFER, bulletAttributesVBO); 
    glBufferData(GL_ARRAY_BUFFER, strides.size() * sizeof(bulletStride_t), &strides[0], GL_STATIC_DRAW);

    bulletCount = strides.size();
}

void ShipRenderer::emitBullet(float cannonAngle, shipData_t &shipData, std::vector<TriangleCell> &cells) {
    bulletStride_t bullet;
    bullet.direction = glm::vec2(cos(cannonAngle), sin(cannonAngle));
    bullet.gridIndex = 0;
    bullet.startTime = emscripten_get_now() / 1000.0f;
    bullet.velocity = 0.2;
    
    for(int i = 0; i < cells.size(); ++i) {
        // each cell that is alive can shot a new bullet when the user shots
        if(cells[i].cellAlive) {
            bullet.shipTranslate = glm::vec2(shipPos[3][0], shipPos[3][1]);
            bullet.origin = cells[i].middleOfTriangle;
            bullet.shipRotation = shipData.shipRot;
            shipData.bulletStride.push_back(bullet);
        }
    }

    shipData.configChanged = true;
}*/

///////////////////////////////////////////////////////////////////////////////////////////
/////////                            OpenGL Bullets 2                              ////////
///////////////////////////////////////////////////////////////////////////////////////////

float bulletSize = 0.555f;
float explodeDistance = 7.0;
float bulletSpeed = 0.2;
float bulletBoomZone = 0.7;

void ShipRenderer::initBullets() {                                 
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);                 
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);               
    std::string cannonVertShader = loadTextFile("shaders/sparkBullet.vert");
    std::string cannonFragShader = loadTextFile("shaders/sparkBullet.frag");
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

    float s = 1.0f;  // adjust to taste // literally spanning entire screen...
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

    // get uniform locations
    //bulletTimeLoc = glGetUniformLocation(bulletShader, "uTime");
    //bulletGridDimensionsLoc = glGetUniformLocation(bulletShader, "uGridDimensions");
    //bulletProjectionLoc = glGetUniformLocation(bulletShader, "uProjection");
    //bulletTextureLoc = glGetUniformLocation(bulletShader, "uTexture");

    bulletTimeLoc = glGetUniformLocation(bulletShader, "uTime");
    //bulletAspectLoc = glGetUniformLocation(bulletShader, "uAspect");
    bulletProjviewLoc = glGetUniformLocation(bulletShader, "uProjview");
    bulletSizeLoc = glGetUniformLocation(bulletShader, "uSize");
    bulletLifespanLoc = glGetUniformLocation(bulletShader, "uLifespan");
    bulletExplodeDistLoc = glGetUniformLocation(bulletShader, "uExplodeDist");
    bulletBoomZoneLoc = glGetUniformLocation(bulletShader, "uBoomZone");

    glUseProgram(bulletShader);

    float lifespan = std::max(explodeDistance / std::max(bulletSpeed, 0.05f) + 0.5, 3.0);

    //glUniform1f(bulletAspectLoc, aspect);
    glUniform1f(bulletSizeLoc, bulletSize);
    glUniform1f(bulletExplodeDistLoc, explodeDistance);
    glUniform1f(bulletBoomZoneLoc, bulletBoomZone);
    glUniform1f(bulletLifespanLoc, lifespan);

    // upload once
    //glUniform2fv(bulletGridDimensionsLoc, 1, &bulletGridDimensions[0]);

    glGenVertexArrays(1, &bulletVAO);
    glBindVertexArray(bulletVAO);

    glGenBuffers(1, &bulletVBO);
    glBindBuffer(GL_ARRAY_BUFFER, bulletVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    //////////////////////////////////
    //     static data(quad, uv)    //
    //////////////////////////////////
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
    //glEnableVertexAttribArray(1);
    
    /////////////////////////////
    //      dynamic data       //
    /////////////////////////////
    glGenBuffers(1, &bulletAttributesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, bulletAttributesVBO);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(sparkBulletStride), (void*)offsetof(sparkBulletStride, iCenter));
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(sparkBulletStride), (void*)offsetof(sparkBulletStride, iVelocity));
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(sparkBulletStride), (void*)offsetof(sparkBulletStride, iSeed));
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(sparkBulletStride), (void*)offsetof(sparkBulletStride, iType));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(sparkBulletStride), (void*)offsetof(sparkBulletStride, iSpawnTime));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(5);

    glBindVertexArray(0);

    bulletTexture = loadTexture("bullet.png");
}

void ShipRenderer::renderBullets() {
    glUseProgram(bulletShader);

    //glUniformMatrix4fv(bulletProjectionLoc, 1, GL_FALSE,  glm::value_ptr(projView));
    glUniform1f(bulletTimeLoc, float(emscripten_get_now()-startTime) / 1000.0f);
    glUniformMatrix4fv(bulletProjviewLoc, 1, GL_FALSE, &projView[0][0]);
    printf("(emscripten_get_now()-startTime) / 1000.0f: %f\n", (emscripten_get_now()-startTime) / 1000.0);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, bulletTexture);
    //glUniform1i(bulletTextureLoc, 0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBindVertexArray(bulletVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, bulletCount);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(0);
}

void ShipRenderer::updateBullets(shipData_t *ships, int shipCount) {
    std::vector<sparkBulletStride> strides;

    for(int i = 0; i < shipCount; ++i) {
        shipData_t &ship = ships[i];

        strides.insert(strides.end(), ship.bulletStride.begin(), ship.bulletStride.end());
    }

    glBindBuffer(GL_ARRAY_BUFFER, bulletAttributesVBO); 
    glBufferData(GL_ARRAY_BUFFER, strides.size() * sizeof(sparkBulletStride), &strides[0], GL_STATIC_DRAW);

    bulletCount = strides.size();

    printf("bulletCount: %d", bulletCount);
}

void ShipRenderer::emitBullet(float cannonAngle, shipData_t &shipData, std::vector<TriangleCell> &cells) {
    sparkBulletStride bullet;
    bullet.iSpawnTime = float(emscripten_get_now()-startTime) / 1000.0f;
    bullet.iVelocity = bulletSpeed * glm::vec2(cos(cannonAngle), sin(cannonAngle));
    
    for(int i = 0; i < cells.size(); ++i) {
        // each cell that is alive can shot a new bullet when the user shots
        if(cells[i].cellAlive) {
            bullet.iSeed = rand() / RAND_MAX;
            //bullet.shipTranslate = glm::vec2(shipPos[3][0], shipPos[3][1]);
            glm::mat2 rot = glm::mat2(cos(shipData.shipRot), sin(shipData.shipRot),
                                       -sin(shipData.shipRot), cos(shipData.shipRot));

            bullet.iCenter = rot * cells[i].middleOfTriangle + glm::vec2(shipPos[3][0], shipPos[3][1]);
            //bullet.shipRotation = shipData.shipRot;
            if(cells[i].name == CELL_FIRE) bullet.iType = 0.0f;
            else if(cells[i].name == CELL_ICE) bullet.iType = 1.0f;
            else if(cells[i].name == CELL_RADIOACTIVE) bullet.iType = 1.5f;

            shipData.bulletStride.push_back(bullet);
        }
    }

    shipData.configChanged = true;
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
        updateCannonsGpu(ships, count);
        updateHullsGpu(ships, count);
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
    renderHulls(shipsRotation);
    renderCannons(cursorPos, shipsRotation);
    renderBullets();
}