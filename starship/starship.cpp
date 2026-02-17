#include "starship.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stbImage/stb_image.h"
#include <emscripten/emscripten.h>

extern glm::mat4 projection;  // access the global
extern glm::mat4 projView;
extern bool keys[256];
extern int64_t startTimes[256];
glm::mat4 shipModel;

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

float easeInQuad(float number) {
return number * number;
}

 constexpr inline bool floatsEqual(float a, float b, float epsilon = std::numeric_limits<float>::epsilon())
{
    return std::fabs(a - b) < epsilon;
}

bool sameSign(float a, float b) {
    return (a > 0) == (b > 0);
}

bool beenTrueAtLeastMs(double &lastMsTrue, bool isTrue, int64_t atLeastDuration) {
    double now = emscripten_get_now();

    if(!isTrue) lastMsTrue = DBL_MAX;
    if(isTrue && lastMsTrue == DBL_MAX) lastMsTrue = now;
    if(now - lastMsTrue >= atLeastDuration) return true;

    return false;
}

void Starship::draw() {
    // update ship data struct before sending it
    shipData.shipRot = currentRotation;
    shipRenderer.render(&shipData, 1, glm::vec2(cursorX, cursorY));

    // get normalized direction from input
    float xLength = -(float)keys['a'] + (float)keys['d'];
    float yLength = -(float)keys['s'] + (float)keys['w'];
    float lineLength = sqrt(xLength*xLength + yLength*yLength);
    glm::vec2 normalizedDirection{0};
    if(lineLength) normalizedDirection = glm::vec2(xLength/lineLength, yLength/lineLength);

    // 
    float acceleration = 0.001; // idk if useful but v = d/t and acceleration(I think) a = d/t^2
    float maxSpeed = 0.01 / 4.0;
    int64_t now = emscripten_get_now();
    float speedIncreaseDuration = 650; // speed increase following 
    
    float progressY = (now - std::max(startTimes['s'], startTimes['w'])) / speedIncreaseDuration;
    float progressX = (now - std::max(startTimes['a'], startTimes['d'])) / speedIncreaseDuration;

    float progress = 0.0;
    if((keys['s'] || keys['w']) && (keys['a'] || keys['d'])) {
        progress = std::max(progressY, progressX);
    } else if(keys['s'] || keys['w']) {
        progress = progressY;
    } else if(keys['a'] || keys['d']) {
        progress = progressX;
    }

    float translateSpeed = progress;//easeInQuad(progress);
    if(translateSpeed > 1.0) translateSpeed = 1.0;

    // later need to add translateSpeed into view, then send that projView
    float advanceX = normalizedDirection.x * translateSpeed * 1.0 / 60.0; // needs to rely on delta time instead of 1/60
    float advanceY = normalizedDirection.y * translateSpeed * 1.0 / 60.0; // we want to advance a specific distance in a specific delta time, normalize delta time, multiply by distance we want, delta time is ease
                                                                          // I just realized since ease is x*x it kinda looks like d/t^2 
                                                                          // at first I'd like to translate slower
                                                                          // then I'd like to follow a constant speed
                                                                          // so velocity is ease * (now - start)
                                                                          
                                                                          // of I ever want to calculate how much ship will move in x time, idk how because of ease at beginning
                                                                          // maybe if measurement under speedDuration just pass that to the ease
                                                                          // if measurment above speedDuration calculate where user is at speedDuration, then remove that from total, then calculate + remainder at constant speed

    // directional drag needs to accumulate in direction that user goes in, until some threshold
    float dragRange = 0.042;
    float dragIncrement = 0.0012; // should actually be linked to delta time.......
    float dragIncrementToMiddle = 0.0005;

    static float diff = 0;
    static int tick = 0;
    float delta = (float(tick % 120) / 120.0); // fake delta time for now
    float circleRadius = 0.0057;
    float speedupDist = dragRange + circleRadius*1.4;
    f++;
    tick++;

    // either this or allow first movement to span larger(until it reach max, then it needs to reach smaller max again once it exits it)
    static glm::vec2 directionalDrag{0,0};    
    static glm::vec2 dirAtRadius{0,0};
    static bool firstMaxX = false;
    static bool firstMaxY = false;

    // set dirRadius when firstMax
    float dirRadiusX = 0;
    float dirRadiusY = 0;
    if(firstMaxX) dirRadiusX = dirAtRadius.x;
    if(firstMaxY) dirRadiusY = dirAtRadius.y;

    static bool reachedEndYetX = false;
    static bool reachedEndYetY = false;
    static bool startedWithAlignedWasd = false;

    static glm::vec2 lastPosSnapshot{0};
    static float totalDistTraveled = 0;

    // this effect needs to dissapear once the user moved from some amount
    // each loop do diff between last snapshot and when it reach some amount stop 
    static bool wasIdle = false;
    glm::vec2 distDiff = directionalDrag - lastPosSnapshot;
    totalDistTraveled += sqrt(distDiff.x*distDiff.x + distDiff.y*distDiff.y);

    float dirDiffCircle = 1.0;
    if(startedWithAlignedWasd && !wasIdle && totalDistTraveled < speedupDist) {
        dirDiffCircle = 1.5; // 1.5x quicker
    }
    
    lastPosSnapshot = directionalDrag;

    // it always allow for an additional offset in the direction it came from in the max drag window range at first
    directionalDrag += normalizedDirection * dragIncrement * dirDiffCircle;
    if(directionalDrag.x > dragRange + dirRadiusX) {directionalDrag.x = dragRange + dirRadiusX; if(firstMaxX && !reachedEndYetX) { reachedEndYetX = true; printf("reachedEndYetX\n"); }}
    if(directionalDrag.x < -dragRange + dirRadiusX) {directionalDrag.x = -dragRange + dirRadiusX;  if(firstMaxX && !reachedEndYetX) { reachedEndYetX = true; printf("reachedEndYetX\n"); }}
    if(directionalDrag.y > dragRange + dirRadiusY) {directionalDrag.y = dragRange + dirRadiusY;  if(firstMaxY && !reachedEndYetY) { reachedEndYetY = true; printf("reachedEndYetY\n"); }}
    if(directionalDrag.y < -dragRange + dirRadiusY) {directionalDrag.y = -dragRange + dirRadiusY;  if(firstMaxY && !reachedEndYetY) { reachedEndYetY = true; printf("reachedEndYetY\n"); }}

    // when reached maxDist at least once, if detect under maxDist, set firstMax to false
    bool reachedSmallestMaxDistX = std::abs(directionalDrag.x) <= dragRange;
    if(reachedSmallestMaxDistX && reachedEndYetX) {
        reachedEndYetX = false;
        firstMaxX = false;
        printf("underLegitMaxDistX\n");
    } 

    bool reachedSmallestMaxDistY = std::abs(directionalDrag.y) <= dragRange;
    if(reachedSmallestMaxDistY && reachedEndYetY) {
        reachedEndYetY = false;
        firstMaxY = false;
        printf("underLegitMaxDistY\n");
    } 

    // want to go in direction toward origin specifically
    float dragDist = sqrt(directionalDrag.x*directionalDrag.x + directionalDrag.y*directionalDrag.y);
    glm::vec2 dirDrag = glm::vec2(directionalDrag.x/dragDist, directionalDrag.y/dragDist);

    // then stop when within radius
    bool notDraggingTowardMiddle = true;
    bool notPressingX = !normalizedDirection.x;
    bool notPressingY = !normalizedDirection.y;
    bool shipIdleCloseToMiddle = dragDist <= circleRadius;

    if(notPressingX && !shipIdleCloseToMiddle) directionalDrag.x -= dragIncrementToMiddle * dirDrag.x; // will reach 0
    if(notPressingY && !shipIdleCloseToMiddle) directionalDrag.y -= dragIncrementToMiddle * dirDrag.y; 

    static double st = 0;
    static glm::vec2 workingDir{0};
    static bool wasPressingX = false;
    static bool wasPressingY = false;

    bool justPressed = false;
    if(normalizedDirection.x && !wasPressingX) { justPressed = true; wasPressingX = true; }
    if(normalizedDirection.y && !wasPressingY) { justPressed = true; wasPressingY = true; }

    if(normalizedDirection.x) wasPressingX = true;
    else wasPressingX = false;
    
    if(normalizedDirection.y) wasPressingY = true;
    else wasPressingY = false;

    if(justPressed) {
        justPressed = false;
        workingDir.x = cos(diff + (delta * 2.0 * 3.14159)) * circleRadius*0.9999998;
        workingDir.y = sin(diff + (delta * 2.0 * 3.14159)) * circleRadius*0.9999998;
    } 

    if(beenTrueAtLeastMs(st, shipIdleCloseToMiddle, 50)) {
        directionalDrag.x = cos(diff + (delta * 2.0 * 3.14159)) * circleRadius*0.9999998;
        directionalDrag.y = sin(diff + (delta * 2.0 * 3.14159)) * circleRadius*0.9999998;

        if(!wasIdle) { // just done moving
            firstMaxX = 0;
            firstMaxY = 0;
            startedWithAlignedWasd = false; 
            totalDistTraveled = 0.0;
            lastPosSnapshot = {0,0};
            printf("just done moving\n\n\n");
            wasIdle = true;
        }
    } else {
        diff = -(delta * 2.0 * 3.14159) + atan2f(directionalDrag.y, directionalDrag.x);

        if(wasIdle) {
            dirAtRadius = workingDir * 4.0f;
            firstMaxX = true;   
            firstMaxY = true;
            
            // I want to check when snapshot(toward back) and dir(with back drag) 
            if(glm::dot(glm::normalize(workingDir), normalizedDirection) < -0.3f) { startedWithAlignedWasd = false; printf("startedWithAlignedWasd = true\n");}
            else { startedWithAlignedWasd = true; printf("startedWithAlignedWasd = false\n"); }
            printf("dot: %f\n", glm::dot(glm::normalize(workingDir), glm::normalize(-normalizedDirection)));
            printf("glm::normalize(workingDir) x=%f, y=%f &&&&& glm::normalize(normalizedDirection) x=%f, %f\n", glm::normalize(workingDir).x, glm::normalize(workingDir).y, glm::normalize(-normalizedDirection).x ,glm::normalize(-normalizedDirection).y);

            // maybe add back the cosine but for the when its comming from front size so that it looks quicker than the circle path
            printf("wasIdle\n");
            wasIdle = false; 
        }
    }

    // new idea: freeze middle offset, but when reach newEnd can remove it from expanded window, then when come back go back at angle. also add up velocity that it already add to circle dir on top of final velocity 
    shipTranslate.x -= advanceX; // we want to remove from view
    shipTranslate.y -= advanceY;

    glm::mat4 view = glm::translate(glm::mat4(1), glm::vec3(shipTranslate.x, shipTranslate.y, 0.0));
    projView = projection * view;
    shipModel = glm::translate(glm::mat4(1.0), glm::vec3(-shipTranslate.x - directionalDrag.x, -shipTranslate.y - directionalDrag.y, 0.0));
    
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
        glm::vec4 vert1 = projView * shipModel * shipRotation * cells[i].transform * triangleVerts[0]; // vertex, local translate, local rotate, shipModel translate, projView to origin 
        glm::vec4 vert2 = projView * shipModel * shipRotation * cells[i].transform * triangleVerts[1];
        glm::vec4 vert3 = projView * shipModel * shipRotation * cells[i].transform * triangleVerts[2];

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
        // eventPopup(enum::warningSign, "place new cell next to a neighboring cell");
        printf("place new cell next to a neighboring cell\n");
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
    float dirX = cursorX * aspect; // prob needs to be a property inside ship struct instead
    float dirY = cursorY;
    float cannonAngle = atan2f(dirY, dirX);

    bulletData_t bullet;
    bullet.direction = glm::vec2(cos(cannonAngle), sin(cannonAngle));
    bullet.gridIndex = 0;
    bullet.startTime = emscripten_get_now() / 1000.0f;
    bullet.velocity = 0.2;
    
    for(int i = 0; i < cells.size(); ++i) {
        if(cells[i].cellAlive) {
            bullet.shipTranslate = glm::vec2(shipModel[3][0], shipModel[3][1]);
            bullet.origin = cells[i].middleOfTriangle;
            bullet.shipRotation = currentRotation;

            shipData.bulletData[shipData.bulletDataCount] = bullet;
            shipData.bulletDataCount += 1;
        }
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