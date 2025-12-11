#include "starship.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stbImage/stb_image.h"
#include <emscripten/emscripten.h>

// Shader with rotation matrix
static const char* gridVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;
uniform mat4 uProjection;
uniform mat3 uRotation;
void main() {
    vec3 rotated = uRotation * vec3(aPos, 1.0);
    gl_Position = uProjection * vec4(rotated.xy, 0.0, 1.0);
}
)";

static const char* gridFragmentShader = R"(#version 300 es
precision mediump float;
out vec4 fragColor;
void main() {
    fragColor = vec4(0.3, 0.8, 0.3, 1.0);
}
)";

static const char* cellVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;

uniform mat4 uTransforms[256];
uniform vec2 uTexCoords[768];
uniform vec4 uColors[256];

uniform mat4 uProjection;
uniform mat3 uShipRotation;

out vec2 vTexCoord;
out vec2 vLocalUV;
out vec4 vColor;

void main() {
    mat4 model = uTransforms[gl_InstanceID];
    vTexCoord = uTexCoords[gl_InstanceID * 3 + gl_VertexID];
    vColor = uColors[gl_InstanceID];
    
    // Hardcoded local UVs for border calculation
    if(gl_VertexID == 0) vLocalUV = vec2(0.0, 0.0);
    else if(gl_VertexID == 1) vLocalUV = vec2(1.0, 0.0);
    else vLocalUV = vec2(1.0, 1.0);
    
    vec4 localPos = model * vec4(aPos, 0.0, 1.0);
    vec3 rotated = uShipRotation * vec3(localPos.xy, 1.0);
    gl_Position = uProjection * vec4(rotated.xy, 0.0, 1.0);
}
)";

static const char* cellFragmentShader = R"(#version 300 es
precision mediump float;

in vec2 vTexCoord;
in vec2 vLocalUV;
in vec4 vColor;
out vec4 fragColor;

uniform sampler2D uAtlas;
uniform sampler2D uCrackTex;
uniform float uBorderWidth;
uniform float uTime;

void main() {
    float distFromBottom = vLocalUV.y;
    float distFromRight = 1.0 - vLocalUV.x;
    float distFromDiagonal = (vLocalUV.x - vLocalUV.y) * 0.7071;
    
    float minDist = min(min(distFromBottom, distFromRight), distFromDiagonal);
    
    float edge = fwidth(minDist);
    float blend = smoothstep(uBorderWidth - edge, uBorderWidth + edge, minDist);
    
    vec4 texColor = texture(uAtlas, vTexCoord);
    vec4 baseColor = mix(vColor, texColor, blend);
    
    // Crack glow
    float crack = texture(uCrackTex, vTexCoord).r;
    float pulse = 0.01 + 0.08 * sin(uTime * 2.0);
    vec4 glow = vColor * crack * pulse;
    
    fragColor = baseColor + glow;
}
)";

const char* cannonVertexShader = R"(#version 300 es
precision highp float;

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

const int MAX_CANNONS = 256;

uniform vec2 uCannonPositions[MAX_CANNONS];
uniform float uCannonAngle;
uniform mat4 uProjection;
uniform mat3 uShipRotation;

out vec2 vTexCoord;

void main() {
    vec2 pos = uCannonPositions[gl_InstanceID];
    
    float c = cos(uCannonAngle);
    float s = sin(uCannonAngle);
    
    vec2 rotated = vec2(
        aPos.x * c - aPos.y * s,
        aPos.x * s + aPos.y * c
    );
    
    vec2 vertex = rotated + pos;
    
    vec3 shipRotated = uShipRotation * vec3(vertex, 0.0);

    gl_Position = uProjection * vec4(shipRotated, 1.0);

    vTexCoord = aTexCoord;
}
)";

// Fragment Shader
const char* cannonFragmentShader = R"(#version 300 es
precision mediump float;

in vec2 vTexCoord;
uniform sampler2D uTexture;

out vec4 fragColor;

void main() {
    fragColor = texture(uTexture, vTexCoord);
}
)";

extern glm::mat4 projection;  // access the global

//texture(uCrackTex, vLocalUV).r;
static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}

Starship::Starship() 
    : gridVAO(0),
      gridVBO(0),
      gridShader(0),
      rotationUniformLoc(-1),
      gridVertexCount(0),
      currentRotation(0.0f),
      dragStartRotation(0.0f),
      isDragging(false),
      dragStartX(0.0f),
      dragStartY(0.0f)
{
}

Starship::~Starship() {
    cleanupGrid();
}

static GLuint loadTexture(const char* path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);  // force RGBA
    
    if(!data) {
        printf("Failed to load texture: %s\n", path);
        return 0;
    }
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    stbi_image_free(data);
    
    printf("Loaded texture: %s (%dx%d)\n", path, width, height);
    return texture;
}

GLuint loadTextureBlurry(const char* path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);  // force RGBA
    
    if(!data) {
        printf("Failed to load texture: %s\n", path);
        return 0;
    }
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_2D);
    
    stbi_image_free(data);
    
    printf("Loaded texture: %s (%dx%d)\n", path, width, height);
    return texture;
}


void Starship::updateCannonPositions() {
    glm::vec2 cannonPositions[MAX_CANNONS];
    cannonCount = 0;

    for (int i = 0; i < cells.size() && cannonCount < MAX_CANNONS; ++i) {
        if (cells[i].cellAlive) {
            cannonPositions[cannonCount] = cells[i].middleOfTriangle;
            cannonCount++;
        }
    }

    glUseProgram(cannonShader);
    glUniform2fv(uCannonPositionsLoc, cannonCount, glm::value_ptr(cannonPositions[0]));
}

void Starship::initCannons() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &cannonVertexShader, nullptr);
    glShaderSource(fs, 1, &cannonFragmentShader, nullptr);
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

    printf("uCannonPositionsLoc=%d\n", uCannonPositionsLoc);
    printf("uCannonAngleLoc=%d\n", uCannonAngleLoc);
    printf("uShipRotationLoc=%d\n", uShipRotationLoc);
    printf("uProjectionLoc=%d\n", uProjectionLoc);
    printf("uTextureLoc=%d\n", uTextureLoc);

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

    float height = 0.027f;
    float width = height * aspect;

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

    // Upload initial cannon positions
    updateCannonPositions();
}

void Starship::renderCannons() {
    if (cannonCount == 0) return;

    printf("renderCannons\n");

    // Compute cannon angle toward cursor
    float dirX = cursorX * aspect;
    float dirY = cursorY;
    float cannonAngle = atan2f(dirY, dirX) - currentRotation;

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
    glm::mat3 rotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f), currentRotation, glm::vec3(0.0f, 0.0f, 1.0f)));
    glUniformMatrix3fv(uShipRotationLoc, 1, GL_FALSE, glm::value_ptr(rotationMatrix));

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, cannonCount);
    glBindVertexArray(0);
}

void Starship::setAspect(float aspect) {
    this->aspect = aspect;
}

void Starship::initCellRendering() {
    // Compile shader
    GLuint vert = compileShader(GL_VERTEX_SHADER, cellVertexShader);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, cellFragmentShader);
    cellShader = glCreateProgram();
    glAttachShader(cellShader, vert);
    glAttachShader(cellShader, frag);
    glLinkProgram(cellShader);
    glDeleteShader(vert);
    glDeleteShader(frag);
    
    // Get uniform locations
    transformsLoc = glGetUniformLocation(cellShader, "uTransforms");
    texCoordsLoc = glGetUniformLocation(cellShader, "uTexCoords");
    colorsLoc = glGetUniformLocation(cellShader, "uColors");
    projectionLoc = glGetUniformLocation(cellShader, "uProjection");
    shipRotationLoc = glGetUniformLocation(cellShader, "uShipRotation");
    atlasLoc = glGetUniformLocation(cellShader, "uAtlas");
    atlasCrackLoc = glGetUniformLocation(cellShader, "uCrackTex");
    
    // Create triangle VAO/VBO
    float half = cellSize / 2.0f;
    float triangleVerts[] = {
        -half, -half,
         half, -half,
         half,  half
    };
    
    glGenVertexArrays(1, &cellVAO);
    glGenBuffers(1, &cellVBO);
    
    glBindVertexArray(cellVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cellVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVerts), triangleVerts, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    // Load atlas texture
    cellAtlasTexture = loadTexture("atlas.png");
    crackAtlasTexture = loadTexture("crack_mask.png");
    printf("crack texture ID: %u\n", crackAtlasTexture);
}

void Starship::drawCells() {
    if(cells.empty()) return;
    
    glUseProgram(cellShader);

    float borderWidth = 0.02;
    glUniform1f(glGetUniformLocation(cellShader, "uBorderWidth"), borderWidth);
    
    // Projection
    extern glm::mat4 projection;
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Ship rotation
    float c = cosf(currentRotation);
    float s = sinf(currentRotation);
    float rotationMatrix[9] = {
        c,  s,  0.0f,
       -s,  c,  0.0f,
        0.0f, 0.0f, 1.0f
    };
    glUniformMatrix3fv(shipRotationLoc, 1, GL_FALSE, rotationMatrix);

    glUniform1f(glGetUniformLocation(cellShader, "uTime"), emscripten_get_now() / 1000.0f);
    
    // Bind atlas
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cellAtlasTexture);
    glUniform1i(atlasLoc, 0);

    // Bind crack atlas
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, crackAtlasTexture);
    glUniform1i(atlasCrackLoc, 1);
    
    // Draw
    glBindVertexArray(cellVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, cells.size());
    glBindVertexArray(0);
}

Starship::CellTexCoords Starship::getRandomAtlasCoords(AtlasSprite sprite, int cellNumber) {
    CellTexCoords coords;
    
    float spriteWidth = 1.0f / 3.0f;

    float offsetTop = 0.020;
    float offsetLeft = 0.04;

    float uL, uR;
    if(sprite == 3) {
        uL = sprite * spriteWidth - offsetLeft;
        uR = uL + spriteWidth + offsetLeft;
    } else {
        uL = sprite * spriteWidth + offsetLeft;
        uR = uL + spriteWidth - offsetLeft;
    }
    
    bool useTop = rand() % 2 == 1;
    bool flipU = rand() % 2 == 1;

    float vB, vT;
    if(useTop) {
        vB = 0.0f + offsetTop;
        vT = 0.667f - offsetTop;
    } else {
        vB = 1.0f - offsetTop;
        vT = 0.333f + offsetTop;
    }
    
    // Only flip horizontally, never touch V
    if(flipU) {
        coords.u0 = uR;  coords.v0 = vB;
        coords.u1 = uL;  coords.v1 = vB;
        coords.u2 = uL;  coords.v2 = vT;
    } else {
        coords.u0 = uL;  coords.v0 = vB;
        coords.u1 = uR;  coords.v1 = vB;
        coords.u2 = uR;  coords.v2 = vT;
    }
    
    coords.pad0 = 0.0f;
    coords.pad1 = 0.0f;
    
    return coords;
}


void Starship::updateCellUniforms() {
    if(cells.empty()) return;
    
    glUseProgram(cellShader);
    
    std::vector<glm::mat4> transforms(cells.size());
    std::vector<glm::vec2> texCoords(cells.size() * 3);
    std::vector<glm::vec4> colors(cells.size());
    
    int aliveCount = 0;

    for(size_t i = 0; i < cells.size(); ++i) {
        if(cells[i].cellAlive) {
            transforms[aliveCount] = cells[i].transform;
            
            texCoords[aliveCount * 3 + 0] = glm::vec2(cells[i].texCoords.u0, cells[i].texCoords.v0);
            texCoords[aliveCount * 3 + 1] = glm::vec2(cells[i].texCoords.u1, cells[i].texCoords.v1);
            texCoords[aliveCount * 3 + 2] = glm::vec2(cells[i].texCoords.u2, cells[i].texCoords.v2);
            
            colors[aliveCount] = glm::vec4(cells[i].color.r, cells[i].color.g, cells[i].color.b, cells[i].color.a);

            aliveCount += 1;
        }
    }
    
    glUniformMatrix4fv(transformsLoc, aliveCount, GL_FALSE, glm::value_ptr(transforms[0]));
    glUniform2fv(texCoordsLoc, aliveCount * 3, glm::value_ptr(texCoords[0]));
    glUniform4fv(colorsLoc, aliveCount, glm::value_ptr(colors[0]));
}

void Starship::newAttackCell(CellName name, int cellNumber) {
    TriangleCell newCell;
    newCell.middleOfTriangle = cells[cellNumber-1].middleOfTriangle; // get back old state, then overwrite
    newCell.category = CellCategory::CELL_ATTACK;
    newCell.name = name;
    newCell.cellAlive = true;
    newCell.cellNumber = cellNumber;

    // calculate x,y translate (row column) via cellNumber over grid
    int pairIndex = (cellNumber - 1) / 2; // which square cell
    int row = pairIndex / gridWidth;
    int column = pairIndex % gridWidth;

    // Both triangles in a pair share the same center position
    newCell.x = originX + column * cellSize + cellSize / 2.0f;
    newCell.y = -originY - cellSize - row * cellSize + cellSize / 2.0f;

    glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(newCell.x, newCell.y, 0.0f));
    glm::mat4 rotate = glm::mat4(1.0f);
    if(cellNumber % 2 == 1) {
        rotate = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    newCell.transform = translate * rotate;

    if(name == CellName::CELL_FIRE) {
        newCell.spriteName = ATLAS_FIRE;
        newCell.texCoords = getRandomAtlasCoords(ATLAS_FIRE, cellNumber);
        newCell.color = {1.0f, 0.5f, 0.2f, 1.0f};  // orange
    }
    else if (name == CellName::CELL_ICE) {
        newCell.spriteName = ATLAS_ICE;
        newCell.texCoords = getRandomAtlasCoords(ATLAS_ICE, cellNumber);
        newCell.color = {0.2f, 0.6f, 1.0f, 1.0f};  // blue
    }
    else if(name == CellName::CELL_RADIOACTIVE) {
        newCell.spriteName = ATLAS_RADIOACTIVE;
        newCell.texCoords = getRandomAtlasCoords(ATLAS_RADIOACTIVE, cellNumber);
        newCell.color = {0.2f, 1.0f, 0.2f, 1.0f};  // green
    }
    
    // replace cell in cells vector
    for(int i = 0; i < cells.size(); ++i) {
        if(cells[i].cellNumber == cellNumber) {
            cells[i] = newCell;
            break;
        }
    }

    // Update uniforms after adding/replacing cell
    updateCellUniforms();
}

void Starship::initCellMiddlePoints() {
    for (int j = 0; j < gridHeight; j++) {
        for (int i = 0; i < gridWidth; i++) {
            float x0 = originX + i * cellSize; // left
            float x1 = originX + (i + 1) * cellSize; // right
            float y0 = originY + (gridHeight - 1 - j) * cellSize; // bottom
            float y1 = originY + (gridHeight - j) * cellSize; // top

            int baseCellNumber = (j * gridWidth + i) * 2;

            // Bottom-right triangle: vertices (x0,y0), (x1,y0), (x1,y1)
            int cellNum0 = baseCellNumber + 1;
            cells[cellNum0].middleOfTriangle = glm::vec2(
                (x0 + x1 + x1) / 3.0f,  // x is left + right + right / 3
                (y0 + y0 + y1) / 3.0f  // y is bottom + bottom + top / 3
            );

            // Top-left triangle: vertices (x0,y0), (x1,y1), (x0,y1)
            int cellNum1 = baseCellNumber;
            cells[cellNum1].middleOfTriangle = glm::vec2(
                (x0 + x0 + x1) / 3.0f,
                (y0 + y1 + y1) / 3.0f
            );
        }
    }
}

void Starship::initStarshipCells() {
    int totalTriangles = gridWidth * gridHeight * 2;

    cells.resize(0);

    for(int i = 1; i < totalTriangles + 1; ++i) {
        TriangleCell newCell;
        newCell.cellAlive = false;
        newCell.cellNumber = i;
        cells.push_back(newCell);
    }
}

void Starship::initGrid() {
    // Create shader program
    GLuint vert = compileShader(GL_VERTEX_SHADER, gridVertexShader);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, gridFragmentShader);
    gridShader = glCreateProgram();
    glAttachShader(gridShader, vert);
    glAttachShader(gridShader, frag);
    glLinkProgram(gridShader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    rotationUniformLoc = glGetUniformLocation(gridShader, "uRotation");
    projectionUniformLoc = glGetUniformLocation(gridShader, "uProjection");

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

void Starship::drawGrid() {
    glUseProgram(gridShader);

    float c = cosf(currentRotation);
    float s = sinf(currentRotation);
    float rotationMatrix[9] = {
        c,  s,  0.0f,
       -s,  c,  0.0f,
        0.0f, 0.0f, 1.0f
    };

    glUniformMatrix3fv(rotationUniformLoc, 1, GL_FALSE, rotationMatrix);
    glUniformMatrix4fv(projectionUniformLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertexCount);
    glBindVertexArray(0);
}

void Starship::cleanupGrid() {
    if (gridVAO) glDeleteVertexArrays(1, &gridVAO);
    if (gridVBO) glDeleteBuffers(1, &gridVBO);
    if (gridShader) glDeleteProgram(gridShader);
    gridVAO = gridVBO = gridShader = 0;
}

void Starship::onMouseDown(int button, float x, float y) {
    if (button == 2) {
        isDragging = true;
        dragStartRotation = currentRotation;
        
        // Calculate center of grid
        float centerX = originX + (gridWidth * cellSize) / 2.0f;
        float centerY = originY + (gridHeight * cellSize) / 2.0f;
        
        // Store starting angle from center to mouse
        dragStartX = atan2f(y - centerY, x - centerX);
    }
}

void Starship::onMouseUp(int button, float x, float y) {
    if (button == 2) {
        isDragging = false;
    }
}

void Starship::onMouseMove(float x, float y) {
    cursorX = x;
    cursorY = y;

    if (!isDragging) return;

    // Calculate center of grid
    float centerX = originX + (gridWidth * cellSize) / 2.0f;
    float centerY = originY + (gridHeight * cellSize) / 2.0f;
    
    // Current angle from center to mouse
    float currentAngle = atan2f(y - centerY, x - centerX);
    
    // Rotation = stored rotation + angle delta
    currentRotation = dragStartRotation + (currentAngle - dragStartX);
}