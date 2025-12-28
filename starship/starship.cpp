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
/*
static const char* cellVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;

uniform mat4 uTransforms[256];
uniform vec2 uTexCoords[768];
uniform vec4 uColors[256];

uniform mat4 uProjection;
uniform mat4 uLocalRotation;
uniform mat3 uShipRotation;

out vec2 vTexCoord;
out vec3 vBary;
out vec4 vColor;

void main() {
    mat4 model = uTransforms[gl_InstanceID];
    vTexCoord = uTexCoords[gl_InstanceID * 3 + gl_VertexID];
    vColor = uColors[gl_InstanceID];
    
    if(gl_VertexID == 0) vBary = vec3(1.0, 0.0, 0.0);
    else if(gl_VertexID == 1) vBary = vec3(0.0, 1.0, 0.0);
    else vBary = vec3(0.0, 0.0, 1.0);
    
    vec4 localPos = model * uLocalRotation * vec4(aPos, 0.0, 1.0);
    vec3 rotated = uShipRotation * vec3(localPos.xy, 1.0);
    gl_Position = uProjection * vec4(rotated.xy, 0.0, 1.0);
}
)";*/
/*
static const char* cellFragmentShader = R"(#version 300 es
precision mediump float;

in vec2 vTexCoord;
in vec3 vBary;
in vec4 vColor;
out vec4 fragColor;

uniform sampler2D uAtlas;
uniform sampler2D uCrackTex;
uniform float uBorderWidth;
uniform float uTime;

void main() {
    float minDist = min(min(vBary.x, vBary.y), vBary.z);
    
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
)";*/

static const char* cursorCellVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;

uniform mat4 uTransforms[128];
uniform vec2 uTexCoords[384];
uniform vec4 uColors[128];

uniform mat4 uProjection;
uniform mat4 uLocalRotation;
uniform mat3 uShipRotation;

uniform int uCellID;

out vec2 vTexCoord;
out vec2 vLocalUV;
out vec4 vColor;

void main() {
    mat4 model = uTransforms[uCellID];
    vTexCoord = uTexCoords[uCellID * 3 + gl_VertexID];
    vColor = uColors[uCellID];
    
    // Hardcoded local UVs for border calculation
    if(gl_VertexID == 0) vLocalUV = vec2(0.0, 0.0);
    else if(gl_VertexID == 1) vLocalUV = vec2(1.0, 0.0);
    else vLocalUV = vec2(1.0, 1.0);
    
    vec4 localPos = model * uLocalRotation * vec4(aPos, 0.0, 1.0);
    vec3 rotated = uShipRotation * vec3(localPos.xy, 1.0);
    gl_Position = uProjection * vec4(rotated.xy, 0.0, 1.0);
}
)";

static const char* cellVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;

uniform mat4 uTransforms[128];
uniform vec2 uTexCoords[384];
uniform vec4 uColors[128];

uniform mat4 uProjection;
uniform mat4 uLocalRotation;
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
    
    vec4 localPos = model * uLocalRotation * vec4(aPos, 0.0, 1.0);
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
    //fragColor = vec4(vTexCoord, 0.0, 1.0);
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

Starship::Starship() :
      gridVAO(0),
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

    // Upload initial cannon positions
    updateCannonPositions();
}

void Starship::renderCannons() {
    if (cannonCount == 0) return;
    
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

void Starship::setAspect(float aspect, float width, float height) {
    this->aspect = aspect;
    this->width = width;
    this->height = height;
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
    localRotationLoc = glGetUniformLocation(cellShader, "uLocalRotation");
    
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

    float borderWidth = 0.012;
    glUniform1f(glGetUniformLocation(cellShader, "uBorderWidth"), borderWidth);
    
    // init with no rot
    glUniformMatrix4fv(localRotationLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));

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
    glBindVertexArray(cellVAO);
    
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

bool Starship::isCursorInsideCell(glm::vec2 cursor, glm::vec4 v1, glm::vec4 v2, glm::vec4 v3) {
    glm::vec2 a = glm::vec2(v1);
    glm::vec2 b = glm::vec2(v2);
    glm::vec2 c = glm::vec2(v3);

    auto sign = [](glm::vec2 p, glm::vec2 a, glm::vec2 b) {
        return (p.x - b.x) * (a.y - b.y) - (a.x - b.x) * (p.y - b.y);
    };

    float d1 = sign(cursor, a, b);
    float d2 = sign(cursor, b, c);
    float d3 = sign(cursor, c, a);

    bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(hasNeg && hasPos);
}

int Starship::getSelectedCellId(glm::vec2 cursorPos) {
    float half = cellSize / 2.0f;
    glm::vec4 triangleVerts[] = {
        {-half, -half, 0, 1.0},
        { half, -half, 0, 1.0},
        { half,  half, 0, 1.0}
    };

    glm::mat4 shipRotation = glm::rotate(glm::mat4(1.0f), currentRotation, glm::vec3(0.0f, 0.0f, 1.0f));

    // iterate through cells in order
    for(int i = 0; i < cells.size(); ++i) {
        // apply this cell transform to triangleVerts
        glm::vec4 vert1 = projection * shipRotation * cells[i].transform * triangleVerts[0];
        glm::vec4 vert2 = projection * shipRotation * cells[i].transform * triangleVerts[1];
        glm::vec4 vert3 = projection * shipRotation * cells[i].transform * triangleVerts[2];

        // if cursor is inside these triangleVerts, return id of cell
        if(isCursorInsideCell(cursorPos, vert1, vert2, vert3)){
            printf("cells[i].cellNumber: %d\n", cells[i].cellNumber);
            return cells[i].cellNumber;
        }
    }

    return -1; // cursor wasn't inside any cell
}

void Starship::initStarshipCells() {
    int totalTriangles = gridWidth * gridHeight * 2;

    cells.resize(0);

    for(int i = 0; i < totalTriangles; ++i) {
        int cellNumber = i;

        TriangleCell newCell;
        newCell.cellAlive = false;
        newCell.cellNumber = cellNumber;

        // calculate x,y translate (row column) via cellNumber over grid
        int pairIndex = (cellNumber) / 2; // which square cell
        int row = pairIndex / gridWidth;
        int column = pairIndex % gridWidth;

        // Both triangles in a pair share the same center position
        newCell.x = originX + column * cellSize + cellSize / 2.0f;
        newCell.y = -originY - cellSize - row * cellSize + cellSize / 2.0f;

        glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(newCell.x, newCell.y, 0.0f));
        glm::mat4 rotate = glm::mat4(1.0f);
        if(cellNumber % 2 == 0) {
            rotate = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        }

        newCell.transform = translate * rotate;

        cells.push_back(newCell);
    }
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

void Starship::setButtonManager(ButtonManager *buttonManager, Renderer2D *renderer2d) {
    this->buttonManager = buttonManager;
    this->renderer2d = renderer2d;
}

void Starship::createMenuButtons(CellCategory category) {
    const int buttonCount = 8;
    typedef struct {
        std::string cellText[buttonCount];
        CellName cellName[buttonCount];
    } menuItems_t;

    menuItems_t menuItems;

    // menuConfig for attack type cell
    if(category == CellCategory::CELL_ATTACK) {
        menuItems = {
            {"Fire", "Ice", "Radioactive", "Radioactive", "Radioactive", "Radioactive", "Radioactive", "Radioactive"},
            {CellName::CELL_FIRE, CellName::CELL_ICE, CellName::CELL_RADIOACTIVE, CellName::CELL_RADIOACTIVE, CellName::CELL_RADIOACTIVE, CellName::CELL_RADIOACTIVE, CellName::CELL_RADIOACTIVE, CellName::CELL_RADIOACTIVE}
        };
    }

    float width = this->width;
    float aspect = width / height;
    float firstWidth = width;

    float menuWidth = 0.23;
    float menuHeight = 0.43;
    float menuAnchorX = 0.007;
    float menuAnchorY = 0.01;//15;

    float buttonWidth = 0.09; // want button to grow width slower when width aspect is bigger
    float buttonHeight = 0.09;

    int btnCountPerRow = 2;
    int btnCountPerColumn = buttonCount / btnCountPerRow;

    float marginLeftRight = 0.015;
    float marginTopBottom = 0.015;

    // create button
    width /= aspect;

    for(int i = 0; i < buttonCount; ++i) {
        // column (x)
        int column = i % btnCountPerRow; // remainder from i after removing all row

        float totalWidthRowBtns = btnCountPerRow * buttonWidth;
        float availableXRange = menuWidth - totalWidthRowBtns - marginLeftRight*2.0;
        float gapX = availableXRange / (btnCountPerRow-1);

        float buttonX = column * (buttonWidth + gapX);
        
        // row (y)
        int row = i / btnCountPerRow; // remove all row width from i, no remainder

        float totalHeightColumnBtns = btnCountPerColumn * buttonHeight;
        float availableYRange = menuHeight - totalHeightColumnBtns - marginTopBottom*2.0;
        float gapY = availableYRange / (btnCountPerColumn-1);

        float buttonY = menuHeight - buttonHeight - row * (gapY + buttonHeight);

        Button config;
        config.x = firstWidth + buttonX * width - menuWidth * width + marginLeftRight*width - menuAnchorX*width;
        config.y = buttonY * height - marginTopBottom*height + menuAnchorY*height;
        config.width = buttonWidth * width;
        config.height = buttonHeight * height;
        config.text = menuItems.cellText[i];
        config.textScale = 0.55f * (height / 2000.0);
        config.color = glm::vec4(22.0/255.0, 22.0/255.0, 22.0/255.0, 1.0f);
        config.borderRadius = 10.0f * (height / 2000.0);
        config.borderColor = glm::vec4(0.0, 0.0, 0.0, 1.0); // grey
        config.borderWidth = 1.0;
        config.drawImage = "top";
        config.textureId = 1; // still need to draw with img centering logic when no texture, just don't call renderer
        config.imageHeight = 0.05 * height;
        config.imageGap = 22.0 * (height / 2000.0);
                                                                
        Button* myButton = buttonManager->createButton(config); // do not discard reference, when screen resize need to recreate all the buttons
        buttonManager->setCallback(myButton, [i, menuItems, this](Button* btn) {
            // reset all buttons state to normal color
            for(int i = 0; i < this->buttons.size(); ++i) {
                this->buttons[i]->color = glm::vec4(22.0/255.0, 22.0/255.0, 22.0/255.0, 1.0f); // default color
            }

            if(menuCursorSelect.buttonId != i) { // the id of the previous clicked button is different than the id of this button
                // change selected triangle cell state for this i
                btn->color = glm::vec4(14.0/255.0, 11.0/255.0, 11.0/255.0, 1.0f);  // redish
                menuCursorSelect.canDrawTriangleAtCursor = true;
                menuCursorSelect.type = menuItems.cellName[i];
                menuCursorSelect.cannonCount = 1;
                menuCursorSelect.buttonId = i;
            }
            else { // menuCursorSelect.type == menuItems.cellName[i]
                btn->color = glm::vec4(22.0/255.0, 22.0/255.0, 22.0/255.0, 1.0f); // default color
                menuCursorSelect.type = CELL_NONE;
                menuCursorSelect.canDrawTriangleAtCursor = false;
                menuCursorSelect.buttonId = -1;
            }
        });

        this->buttons.push_back(myButton); // add this button to the button list

        // create triangle for button
        buttonManager->drawButtons();
        float middleImgX = myButton->calculatedMiddleImgX; // set button middle image pos for getter
        float middleImgY = myButton->calculatedMiddleImgY;
        
        newMenuTriangle(menuItems.cellName[i], i, ((middleImgX / firstWidth * 2.0) - 1.0) * aspect, middleImgY / height * 2.0 - 1.0);
    }

    backWidth = menuWidth * width;     // also update this when screen resize, so need to overwrite this
    backHeight = menuHeight * height;
    anchor = glm::vec2(firstWidth - menuWidth * width - menuAnchorX * width, menuAnchorY * height);
}

bool Starship::neighborsAlive(int cellId) {
    bool canPlaceCell = false;

    bool livingCellLeft = false;
    bool livingCellRight = false;
    bool livingCellTop = false;
    bool livingCellBottom = false;

    int topId = cellId - gridWidth*2 + 1; // get back 1 row from same id
    int bottomId = cellId + gridWidth*2 - 1; // get front 1 row from same id
    if(cellId-1 <= cells.size()) livingCellLeft = cells[cellId-1].cellAlive; 
    if(cellId+1 > 0) livingCellRight = cells[cellId+1].cellAlive;
    if(topId > 0) livingCellTop = cells[topId].cellAlive;
    if(bottomId <= cells.size()) livingCellBottom = cells[bottomId].cellAlive;

    if(cellId % 2 == 0) canPlaceCell = livingCellLeft || livingCellRight || livingCellTop; // can place cell if a living cell is left, bottom, or right to cell
    if(cellId % 2 == 1) canPlaceCell = livingCellLeft || livingCellRight || livingCellBottom; // can place cell if a living cell is left, top, or right to cell

    return canPlaceCell;
}

int Starship::getPrice(Starship::CellName type, int cannonCount) {
    const int maxCannon = 5;
    if(cannonCount > maxCannon) return -1;
    if(cannonCount < 1) return -1;

    float multiplyFactor[maxCannon] = { 1.0, 2.5, 4.0, 6.0, 8.0 };

    int price = 0;
    if(type == CellName::CELL_FIRE) price = 500 * multiplyFactor[cannonCount-1];
    else if(type == CellName::CELL_ICE) price = 600 * multiplyFactor[cannonCount-1];
    else if(type == CellName::CELL_RADIOACTIVE) price = 700 * multiplyFactor[cannonCount-1];

    return price;
}

bool Starship::placeCell(glm::vec2 cursorPos) { // when user clicks
    int cellId = getSelectedCellId(cursorPos);

    // if user interacts with menu, do not do anything
    float cursorX = (cursorPos.x+1.0)/2.0 * width;
    float cursorY = (cursorPos.y+1.0)/2.0 * height;
    bool insideMenu = anchor.x < cursorX && cursorX < anchor.x + backWidth &&
                      anchor.y < cursorY && cursorY < anchor.y + backHeight;
    if(insideMenu) return false;

    // get cell id, if outside ship && outside menu, just click out of menu
    if(cellId == -1 && !insideMenu) {
        // unselect clicked state
        menuCursorSelect.canDrawTriangleAtCursor = false;
        menuCursorSelect.type = CELL_NONE;
        menuCursorSelect.buttonId = -1;

        // reset buttons color(unselect button)
        for(int i = 0; i < buttons.size(); ++i)
            buttons[i]->color = glm::vec4(22.0/255.0, 22.0/255.0, 22.0/255.0, 1.0f);

        // couldn't place cell..
        return false;
    }

    // if inside cell, check if player has enough energy to buy it & at least 1 neighbors exist
    Starship::CellName type = menuCursorSelect.type;
    int cannonCount = menuCursorSelect.cannonCount;

    if(energy < getPrice(type, cannonCount)) { // not enough energy
        // eventPopup(enum::warningSign, "not enough energy");
        printf("not enough energy\n");
        return false;
    }

    if(!neighborsAlive(cellId)) { // not even 1 neighbors is alive
        // eventPopup(enum::warningSign, "place cell next to neighboring cell");
        printf("place cell next to neighboring cell\n");
        return false;
    }

    energy -= getPrice(type, cannonCount);
    printf("energy -= getPrice %d:\n", energy);
    newAttackCell(type, cellId);
    return true;
}

void Starship::initTriangleAtCursor() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &cursorCellVertexShader, nullptr);
    glShaderSource(fragmentShader, 1, &cellFragmentShader, nullptr);

    glCompileShader(vertexShader);
    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (success) {
        printf("Vertex shader compiled OK\n");
    } else {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        printf("Vertex shader error: %s\n", infoLog);
    }

    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (success) {
        printf("Fragment shader compiled OK\n");
    } else {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        printf("Fragment shader error: %s\n", infoLog);
    }

    triangleAtCursorProgram = glCreateProgram();
    glAttachShader(triangleAtCursorProgram, vertexShader);
    glAttachShader(triangleAtCursorProgram, fragmentShader);
    glLinkProgram(triangleAtCursorProgram);

    glGetProgramiv(triangleAtCursorProgram, GL_LINK_STATUS, &success);
    if (success) {
        printf("Program linked OK\n");
    } else {
        glGetProgramInfoLog(triangleAtCursorProgram, 512, nullptr, infoLog);
        printf("Program link error: %s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get uniform locations for menu shader
    cursorTriangleTransformsLoc = glGetUniformLocation(triangleAtCursorProgram, "uTransforms");
    cursorTriangleTexCoordsLoc = glGetUniformLocation(triangleAtCursorProgram, "uTexCoords");
    cursorTriangleColorsLoc = glGetUniformLocation(triangleAtCursorProgram, "uColors");
    cursorTriangleProjectionLoc = glGetUniformLocation(triangleAtCursorProgram, "uProjection");
    cursorTriangleShipRotationLoc = glGetUniformLocation(triangleAtCursorProgram, "uShipRotation");
    cursorTriangleAtlasLoc = glGetUniformLocation(triangleAtCursorProgram, "uAtlas");
    cursorTriangleAtlasCrackLoc = glGetUniformLocation(triangleAtCursorProgram, "uCrackTex");
    cursorTriangleTimeLoc = glGetUniformLocation(triangleAtCursorProgram, "uTime");
    cursorTriangleBorderWidthLoc = glGetUniformLocation(triangleAtCursorProgram, "uBorderWidth");

    // Create VAO/VBO
    glGenVertexArrays(1, &cursorTriangleVAO);
    glGenBuffers(1, &cursorTriangleVBO);

    float half = cellSize / 3.1f;
    float triangleVerts[] = {
        -half - half/3.0f,  -half + half/3.0f,   // was (-half, -half)
        half - half/3.0f,  -half + half/3.0f,   // was (half, -half)
        half - half/3.0f,   half + half/3.0f    // was (half, half)
    };

    glBindVertexArray(cursorTriangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cursorTriangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVerts), triangleVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    int menuItemCount = 8;
    cursorTriangleCell.transforms.resize(menuItemCount);
    cursorTriangleCell.colors.resize(menuItemCount);
    cursorTriangleCell.texCoords.resize(menuItemCount * 3);

    for(int i = 0; i < menuItemCount; ++i) {
        CellTexCoords texCoords;

        if(i % 3 == CellName::CELL_FIRE) { // % 3 because 3 options
            texCoords = getRandomAtlasCoords(ATLAS_FIRE, 1);
            cursorTriangleCell.colors[i] = {1.0f, 0.5f, 0.2f, 1.0f};  // orange
        }
        else if (i % 3 == CellName::CELL_ICE) {
            texCoords = getRandomAtlasCoords(ATLAS_ICE, 2);
            cursorTriangleCell.colors[i] = {0.2f, 0.6f, 1.0f, 1.0f};  // blue
        }
        else if(i % 3 == CellName::CELL_RADIOACTIVE) {
            texCoords = getRandomAtlasCoords(ATLAS_RADIOACTIVE, 3);
            cursorTriangleCell.colors[i] = {0.2f, 1.0f, 0.2f, 1.0f};  // green
        }

        cursorTriangleCell.texCoords[i * 3 + 0] = glm::vec2(texCoords.u0, texCoords.v0);
        cursorTriangleCell.texCoords[i * 3 + 1] = glm::vec2(texCoords.u1, texCoords.v1);
        cursorTriangleCell.texCoords[i * 3 + 2] = glm::vec2(texCoords.u2, texCoords.v2);

        // set this setting to 0 for all triangleAtCursor
        cursorTriangleCell.transforms[i] = glm::mat4(1.0);
    }

    glUseProgram(triangleAtCursorProgram);

    // set program uniform arrays (color transform texCoord ect)
    glBindVertexArray(cursorTriangleVAO);
    glUniformMatrix4fv(cursorTriangleTransformsLoc, cursorTriangleCell.transforms.size(), GL_FALSE,  glm::value_ptr(cursorTriangleCell.transforms[0]));
    glUniform2fv(cursorTriangleTexCoordsLoc, cursorTriangleCell.texCoords.size(),  glm::value_ptr(cursorTriangleCell.texCoords[0]));
    glUniform4fv(cursorTriangleColorsLoc, cursorTriangleCell.colors.size(),  glm::value_ptr(cursorTriangleCell.colors[0]));
}

void Starship::drawTriangleAtCursor() {
    int triangleStart = menuCursorSelect.type * 3;

    if(menuCursorSelect.canDrawTriangleAtCursor) {
        glUseProgram(triangleAtCursorProgram);

        // set cellID to draw at cursor(which cell)
        glUniform1i(glGetUniformLocation(triangleAtCursorProgram, "uCellID"), menuCursorSelect.type);

        // set translation under cursor
        glm::mat4 translation = glm::translate(glm::mat4(1.0), glm::vec3(cursorX * aspect, cursorY, 0.0));
        glUniformMatrix4fv(glGetUniformLocation(triangleAtCursorProgram, "uLocalRotation"), 1, GL_FALSE, glm::value_ptr(translation));

        // Set proj
        glUniformMatrix4fv(cursorTriangleProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // no rotation for (all) cursor cells
        float borderWidth = 0.02; /// put in header
        float currentTime = emscripten_get_now() / 1000.0f;
        glUniformMatrix3fv(cursorTriangleShipRotationLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(1.0f)));
        glUniform1f(cursorTriangleBorderWidthLoc, borderWidth);
        glUniform1f(cursorTriangleTimeLoc, currentTime);

        // Bind atlas
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cellAtlasTexture);
        glUniform1i(cursorTriangleAtlasLoc, 0);

        // Bind crack atlas
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, crackAtlasTexture);
        glUniform1i(cursorTriangleAtlasCrackLoc, 1);

        glBindVertexArray(cursorTriangleVAO); // this is not registerd into the program like uniforms, just vbo data
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 1);

        glBindVertexArray(0);
    }
}

void Starship::updateMenuTriangle() {
    glUseProgram(cellMenuShader);
    glBindVertexArray(cellMenuVAO);
    
    glUniformMatrix4fv(transformsLoc, cellMenu.transforms.size(), GL_FALSE,  glm::value_ptr(cellMenu.transforms[0]));
    glUniform2fv(texCoordsLoc, cellMenu.texCoords.size(),  glm::value_ptr(cellMenu.texCoords[0]));
    glUniform4fv(colorsLoc, cellMenu.colors.size(),  glm::value_ptr(cellMenu.colors[0]));
}

void Starship::newMenuTriangle(CellName name, int i, float x, float y) { // if I do i too far all data inbetween is corrupted and unitialized
    if(cellMenu.transforms.size() <= i) cellMenu.transforms.resize(i+1);
    if(cellMenu.colors.size() <= i) cellMenu.colors.resize(i+1);
    if(cellMenu.texCoords.size() <= i * 3) cellMenu.texCoords.resize((i+1) * 3);

    CellTexCoords texCoords;

    if(name == CellName::CELL_FIRE) {
        texCoords = getRandomAtlasCoords(ATLAS_FIRE, i);
        cellMenu.colors[i] = {1.0f, 0.5f, 0.2f, 1.0f};  // orange
    }
    else if (name == CellName::CELL_ICE) {
       texCoords = getRandomAtlasCoords(ATLAS_ICE, i);
        cellMenu.colors[i] = {0.2f, 0.6f, 1.0f, 1.0f};  // blue
    }
    else if(name == CellName::CELL_RADIOACTIVE) {
        texCoords = getRandomAtlasCoords(ATLAS_RADIOACTIVE, i);
        cellMenu.colors[i] = {0.2f, 1.0f, 0.2f, 1.0f};  // green
    }

    cellMenu.texCoords[i * 3 + 0] = glm::vec2(texCoords.u0, texCoords.v0);
    cellMenu.texCoords[i * 3 + 1] = glm::vec2(texCoords.u1, texCoords.v1);
    cellMenu.texCoords[i * 3 + 2] = glm::vec2(texCoords.u2, texCoords.v2);

    cellMenu.transforms[i] = glm::translate(glm::mat4(1), glm::vec3(x, y, 0.0));

    updateMenuTriangle();
}

void Starship::drawMenuTriangle() {
    glDisable(GL_BLEND);
    glUseProgram(cellMenuShader);

    float borderWidth = 0.02; /// put in header
    float currentTime = emscripten_get_now() / 1000.0f;
    
    // set rot
    glm::mat4 rot = glm::rotate(glm::mat4(1.0), glm::radians(currentTime * 20.0f), glm::vec3(0.0, 0.0, 1.0));
    glUniformMatrix4fv(localRotationLoc, 1, GL_FALSE, glm::value_ptr(rot));

    // Set proj
    //glm::mat4 noProj = glm::mat4(1.0f);
    glUniformMatrix4fv(menuProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glUniformMatrix3fv(menuShipRotationLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(1.0f))); // no rotation for menu
    glUniform1f(menuBorderWidthLoc, borderWidth);
    glUniform1f(menuTimeLoc, currentTime);

    // Bind atlas
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cellAtlasTexture);
    glUniform1i(atlasLoc, 0);

    // Bind crack atlas
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, crackAtlasTexture);
    glUniform1i(atlasCrackLoc, 1);

    // draw with vao that contains triangles for menu
    glBindVertexArray(cellMenuVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, cellMenu.transforms.size());

    glBindVertexArray(0);
}

void Starship::screenResize(float width, float height) {
    this->width = width;
    this->height = height;
    this->aspect = width / height;

    for(int i = 0; i < buttons.size(); ++i) {
        buttonManager->removeButton(buttons[i]);
    }

    this->buttons.clear();

    createMenuButtons(Starship::CELL_ATTACK);
}

void Starship::draw() {
    renderer2d->drawFilledRoundedRect(anchor, backWidth, backHeight, 12.0f * (height / 2000.0), glm::vec4(27.0/255.0, 27.0/255.0, 27.0/255.0, 1.0));
    renderer2d->drawRoundedRect(anchor, backWidth, backHeight, 1.0, 12.0f * (height / 2000.0), glm::vec4(0.0, 0.0, 0.0, 1.0));
}

void Starship::initMenuTriangle() {
    // Create separate shader program for menu
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &cellVertexShader, nullptr);
    glShaderSource(fragmentShader, 1, &cellFragmentShader, nullptr);
    glCompileShader(fragmentShader);
    glCompileShader(vertexShader);
    cellMenuShader = glCreateProgram();
    glAttachShader(cellMenuShader, vertexShader);
    glAttachShader(cellMenuShader, fragmentShader);
    glLinkProgram(cellMenuShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get uniform locations for menu shader
    menuTransformsLoc = glGetUniformLocation(cellMenuShader, "uTransforms");
    menuTexCoordsLoc = glGetUniformLocation(cellMenuShader, "uTexCoords");
    menuColorsLoc = glGetUniformLocation(cellMenuShader, "uColors");
    menuProjectionLoc = glGetUniformLocation(cellMenuShader, "uProjection");
    menuShipRotationLoc = glGetUniformLocation(cellMenuShader, "uShipRotation");
    menuAtlasLoc = glGetUniformLocation(cellMenuShader, "uAtlas");
    menuAtlasCrackLoc = glGetUniformLocation(cellMenuShader, "uCrackTex");
    menuTimeLoc = glGetUniformLocation(cellMenuShader, "uTime");
    menuBorderWidthLoc = glGetUniformLocation(cellMenuShader, "uBorderWidth");

    // Create VAO/VBO
    glGenVertexArrays(1, &cellMenuVAO);
    glGenBuffers(1, &cellMenuVBO);

    float half = cellSize / 3.1f;
    float triangleVerts[] = {
        -half - half/3.0f,  -half + half/3.0f,   // was (-half, -half)
        half - half/3.0f,  -half + half/3.0f,   // was (half, -half)
        half - half/3.0f,   half + half/3.0f    // was (half, half)
    };

    glBindVertexArray(cellMenuVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cellMenuVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVerts), triangleVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Starship::newAttackCell(CellName name, int cellNumber) {
    TriangleCell newCell;
    newCell = cells[cellNumber]; // get back old state, keep basic fields..
    newCell.category = CellCategory::CELL_ATTACK;
    newCell.name = name;
    newCell.cellAlive = true;
    newCell.cellNumber = cellNumber;

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
    updateCannonPositions();
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

    if(button == 0) { // left click
        if(menuCursorSelect.canDrawTriangleAtCursor) {
            bool couldPurchase = placeCell(glm::vec2(x, y));

            if(couldPurchase) printf("could purchase cell, remaing energy: %d\n", energy);
            else printf("could NOT purchase cell, remaing energy: %d\n", energy);
        }
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