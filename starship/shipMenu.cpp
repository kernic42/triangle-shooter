#include "shipMenu.h"

extern glm::mat4 projection;

void ShipMenu::screenResize(float width, float height) {
    this->width = width;
    this->height = height;
    this->aspect = width / height;

    for(int i = 0; i < buttons.size(); ++i) {
        buttonManager->removeButton(buttons[i]);
    }

    this->buttons.clear();

    createMenuButtons(CELL_ATTACK);
}

void ShipMenu::setAspect(float width, float height) {
    this->aspect = width / height;
    this->width = width;
    this->height = height;
}

void ShipMenu::setButtonManager(ButtonManager *buttonManager, Renderer2D *renderer2d) {
    this->buttonManager = buttonManager;
    this->renderer2d = renderer2d;
}

void ShipMenu::setCursor(glm::vec2 cursorPos) {
    this->cursorX = cursorPos.x;
    this->cursorY = cursorPos.y;
}

void ShipMenu::init() {
    // Load atlas texture
    cellAtlasTexture = loadTexture("atlas.png");
    crackAtlasTexture = loadTexture("crack_mask.png");

    initTriangleAtCursor();
    initMenuTriangle();
    createMenuButtons(CELL_ATTACK);
}

void ShipMenu::render() {
    renderer2d->drawFilledRoundedRect(anchor, backWidth, backHeight, 12.0f * (height / 2000.0), glm::vec4(27.0/255.0, 27.0/255.0, 27.0/255.0, 1.0));
    renderer2d->drawRoundedRect(anchor, backWidth, backHeight, 1.4, 12.0f * (height / 2000.0), glm::vec4(0.0, 0.0, 0.0, 1.0));
    renderer2d->flush();
    buttonManager->drawButtons();
    drawTriangleAtCursor(); 
    drawMenuTriangle();
}

CellTexCoords ShipMenu::getRandomAtlasCoords(AtlasSprite sprite, int cellNumber) {
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

void ShipMenu::initTriangleAtCursor() {
    // Create separate shader program for initTriangleAtCursor
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &cursorCellVertexShader, nullptr);
    glShaderSource(fragmentShader, 1, &cellFragmentShader, nullptr);
    glCompileShader(fragmentShader);
    glCompileShader(vertexShader);
    triangleAtCursorProgram = glCreateProgram();
    glAttachShader(triangleAtCursorProgram, vertexShader);
    glAttachShader(triangleAtCursorProgram, fragmentShader);
    glLinkProgram(triangleAtCursorProgram);
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

void ShipMenu::drawTriangleAtCursor() {
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

void ShipMenu::updateMenuTriangle() {
    glUseProgram(cellMenuShader);
    glBindVertexArray(cellMenuVAO);
    
    glUniformMatrix4fv(menuTransformsLoc, cellMenu.transforms.size(), GL_FALSE,  glm::value_ptr(cellMenu.transforms[0]));
    glUniform2fv(menuTexCoordsLoc, cellMenu.texCoords.size(),  glm::value_ptr(cellMenu.texCoords[0]));
    glUniform4fv(menuColorsLoc, cellMenu.colors.size(),  glm::value_ptr(cellMenu.colors[0]));
}

void ShipMenu::newMenuTriangle(CellName name, int i, float x, float y) { // if I do i too far all data inbetween is corrupted and unitialized
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

    printf("updateMenuTriangle();\n");
    updateMenuTriangle();
}

void ShipMenu::drawMenuTriangle() {
    glDisable(GL_BLEND);
    
    // Check shader
    if (cellMenuShader == 0) {
        printf("ERROR: cellMenuShader is 0\n");
    }
    glUseProgram(cellMenuShader);

    float borderWidth = 0.02;
    float currentTime = emscripten_get_now() / 1000.0f;
    
    // Check uniform locations
    if (localRotationLoc == -1) printf("ERROR: localRotationLoc not found\n");
    if (menuProjectionLoc == -1) printf("ERROR: menuProjectionLoc not found\n");
    if (menuShipRotationLoc == -1) printf("ERROR: menuShipRotationLoc not found\n");
    if (menuBorderWidthLoc == -1) printf("ERROR: menuBorderWidthLoc not found\n");
    if (menuTimeLoc == -1) printf("ERROR: menuTimeLoc not found\n");
    if (menuAtlasLoc== -1) printf("ERROR: menuAtlasLoc not found\n");
    if (menuAtlasCrackLoc == -1) printf("ERROR: menuAtlasCrackLoc not found\n");

    glm::mat4 rot = glm::rotate(glm::mat4(1.0), glm::radians(currentTime * 20.0f), glm::vec3(0.0, 0.0, 1.0));
    glUniformMatrix4fv(localRotationLoc, 1, GL_FALSE, glm::value_ptr(rot));
    glUniformMatrix4fv(menuProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix3fv(menuShipRotationLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(1.0f)));
    glUniform1f(menuBorderWidthLoc, borderWidth);
    glUniform1f(menuTimeLoc, currentTime);

    // Check textures
    if (cellAtlasTexture == 0) printf("ERROR: cellAtlasTexture is 0\n");
    if (crackAtlasTexture == 0) printf("ERROR: crackAtlasTexture is 0\n");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cellAtlasTexture);
    glUniform1i(menuAtlasLoc, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, crackAtlasTexture);
    glUniform1i(menuAtlasCrackLoc, 1);

    // Check VAO and instance count
    if (cellMenuVAO == 0) printf("ERROR: cellMenuVAO is 0\n");
    if (cellMenu.transforms.size() == 0) printf("WARNING: transforms.size() is 0, nothing to draw\n");
    
    printf("Drawing %zu menu triangles\n", cellMenu.transforms.size());

    glBindVertexArray(cellMenuVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, cellMenu.transforms.size());

    // Check for GL errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("GL Error after draw: %d\n", err);
    }

    glBindVertexArray(0);
}

void ShipMenu::initMenuTriangle() {
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

    localRotationLoc = glGetUniformLocation(cellMenuShader, "uLocalRotation");

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

void ShipMenu::createMenuButtons(CellCategory category) {
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
        config.borderWidth = 1.4;
        config.drawImage = "top";
        config.textureId = 0; // still need to draw with img centering logic when no texture, just don't call renderer
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