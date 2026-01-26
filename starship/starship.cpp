#include "starship.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stbImage/stb_image.h"
#include <emscripten/emscripten.h>

extern glm::mat4 projection;  // access the global

//texture(uCrackTex, vLocalUV).r;
static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}

Starship::Starship() { }
Starship::~Starship() { }

void Starship::updateCannonPositions() {
    glm::vec2 cannonPositions[MAX_CANNONS]; // I think MAX_CANNON is redudant with MAX_CANNON_COUNT
    float cannonColors[MAX_CANNONS];
    cannonCount = 0;

    for (int i = 0; i < cells.size() && cannonCount < MAX_CANNONS; ++i) {
        if (cells[i].cellAlive) {
            cannonPositions[cannonCount] = cells[i].middleOfTriangle;

            // temporary..
            if(cells[i].color == glm::vec4(1.0f, 0.5f, 0.2f, 1.0f)) { // orange
                cannonColors[cannonCount] = 0;
            } else if(cells[i].color == glm::vec4(0.2f, 0.6f, 1.0f, 1.0f)) { // blue
                cannonColors[cannonCount] = 1;
            } else if(cells[i].color == glm::vec4(0.2f, 1.0f, 0.2f, 1.0f)) { // green
                cannonColors[cannonCount] = 2;
            }
    
            cannonCount++;
        }
    }

    shipData.configChanged = true;
    shipData.cannonData.count = cannonCount;
    memcpy(&shipData.cannonData.pos[0], &cannonPositions[0], cannonCount * sizeof(glm::vec2));
    memcpy(&shipData.cannonData.colors[0], &cannonColors[0], cannonCount * sizeof(float));
}

void Starship::updateCellUniforms() {
    std::vector<glm::mat4> transforms(cells.size());
    std::vector<glm::mat3x2> texCoords(cells.size());
    std::vector<glm::vec4> colors(cells.size());
    
    int aliveCount = 0;

    for(size_t i = 0; i < cells.size(); ++i) {
        if(cells[i].cellAlive) {
            transforms[aliveCount] = cells[i].transform;
            
            texCoords[aliveCount] = glm::mat3x2(glm::vec2(cells[i].texCoords.u0, cells[i].texCoords.v0),
                                                glm::vec2(cells[i].texCoords.u1, cells[i].texCoords.v1),
                                                glm::vec2(cells[i].texCoords.u2, cells[i].texCoords.v2));

            colors[aliveCount] = glm::vec4(cells[i].color.r, cells[i].color.g, cells[i].color.b, cells[i].color.a);

            aliveCount += 1;
        }
    }
    
    shipData.configChanged = true;
    shipData.cellHullData.count = aliveCount;
    memcpy(shipData.cellHullData.model, &transforms[0], transforms.size() * sizeof(glm::mat4));
    memcpy(shipData.cellHullData.texCoords, &texCoords[0], texCoords.size() * sizeof(glm::mat3x2));
    memcpy(shipData.cellHullData.colors, &colors[0], colors.size() * sizeof(glm::vec4)); // when this works replace memcpy by just filling right field in struct
}

void Starship::init() { // function where we should init everything..
    shipRenderer.initBullets();
    shipRenderer.initGrid();
    shipRenderer.initCannons();
    shipRenderer.initCellRendering();
    shipMenu.init();
}

void Starship::draw() {
    // update ship data struct before sending it
    shipData.shipRot = currentRotation;
    shipRenderer.render(&shipData, 1, glm::vec2(cursorX, cursorY));
    
    shipMenu.render();
}

void Starship::setAspect(float aspect, float width, float height) {
    this->aspect = aspect;
    this->width = width;
    this->height = height;

    shipRenderer.setAspect(width, height);
    shipMenu.setAspect(width, height);
}

CellTexCoords Starship::getRandomAtlasCoords(AtlasSprite sprite, int cellNumber) {
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
        if(isCursorInsideCell(cursorPos, vert1, vert2, vert3))
            return cells[i].cellNumber;
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

    shipMenu.setButtonManager(buttonManager, renderer2d);
}

bool Starship::neighborsAlive(int cellId) {
    bool canPlaceCell = false;

    bool livingCellLeft = false;
    bool livingCellRight = false;
    bool livingCellTop = false;
    bool livingCellBottom = false;

    int topId = cellId - gridWidth*2 + 1; // get back 1 row from same id
    int bottomId = cellId + gridWidth*2 - 1; // get front 1 row from same id
    if(cellId-1 >= 0) livingCellLeft = cells[cellId-1].cellAlive; 
    if(cellId+1 < cells.size()) livingCellRight = cells[cellId+1].cellAlive;
    if(topId < cells.size()) livingCellTop = cells[topId].cellAlive;
    if(bottomId >= 0) livingCellBottom = cells[bottomId].cellAlive;

    if(cellId % 2 == 0) canPlaceCell = livingCellLeft || livingCellRight || livingCellTop; // can place cell if a living cell is left, bottom, or right to cell
    if(cellId % 2 == 1) canPlaceCell = livingCellLeft || livingCellRight || livingCellBottom; // can place cell if a living cell is left, top, or right to cell

    return canPlaceCell;
}

int Starship::getPrice(CellName type, int cannonCount) {
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

    bool clickedOutShip = cellId == -1;
    bool clickedOutMenu = !shipMenu.isCursorInsideMenu(cursorPos);

    // couldn't place cell..
    if(clickedOutShip && clickedOutMenu) {
        shipMenu.unselectButtons();
        return false;
    }

    // if inside cell, check if player has enough energy to buy it & at least 1 neighbors exist
    CellName type = shipMenu.menuCursorSelect.type;
    int cannonCount = shipMenu.menuCursorSelect.cannonCount;

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

void Starship::screenResize(float width, float height) {
    shipMenu.screenResize(width, height);
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

void Starship::shotBullet() {
    // for now just try if it works by making a bunch of bullet spawn from the middle of the screen with some direction, just append to buffer when click
    bulletData_t bullet;
    bullet.direction = glm::vec2(0.707 * aspect, 0.707);
    bullet.gridIndex = 0;
    bullet.startTime = emscripten_get_now() / 1000.0f;
    bullet.velocity = 1.2;
    
    int bulletCount = 5;
    for(int i = 0; i < bulletCount; ++i) {
        bullet.origin = glm::vec2(0.0, -0.2 + (i*0.1));

        shipData.bulletData[shipData.bulletDataCount] = bullet;
        shipData.bulletDataCount += 1;
    }

    shipData.configChanged = true;
}

void Starship::onMouseDown(int button, float x, float y) {
    if (button == 2) { // right click
        isDragging = true;
        dragStartRotation = currentRotation;
        
        // Calculate center of grid
        float centerX = originX + (gridWidth * cellSize) / 2.0f;
        float centerY = originY + (gridHeight * cellSize) / 2.0f;
        
        // Store starting angle from center to mouse
        dragStartX = atan2f(y - centerY, x - centerX);
    }
    
    // goal: ship knows what state the menu is in, ship still detects when click inside ship and act accordingly to menu state
    if(button == 0) { // left click
        shotBullet();

        if(shipMenu.menuCursorSelect.canDrawTriangleAtCursor) {
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
    shipMenu.setCursor(glm::vec2(x, y));

    if (!isDragging) return;

    // Calculate center of grid
    float centerX = originX + (gridWidth * cellSize) / 2.0f;
    float centerY = originY + (gridHeight * cellSize) / 2.0f;
    
    // Current angle from center to mouse
    float currentAngle = atan2f(y - centerY, x - centerX);
    
    // Rotation = stored rotation + angle delta
    currentRotation = dragStartRotation + (currentAngle - dragStartX);
}