#include "starship/starship.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

glm::mat4 projection;

const char* quadVertexShaderSrc = R"(#version 300 es
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)";

const char* quadFragmentShaderSrc = R"(#version 300 es
precision mediump float;
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D uTexture;
void main() {
    fragColor = texture(uTexture, vTexCoord);
}
)";

// Global state
struct AppState {
    int width = 800;
    int height = 600;
    
    // FBO
    GLuint fbo = 0;
    GLuint fboTexture = 0;
    GLuint rbo = 0;
    GLuint resolveFBO = 0;  
    GLuint msaaRBO = 0;   
    
    // Triangle program
    GLuint triangleProgram = 0;
    GLuint triangleVAO = 0;
    GLuint triangleVBO = 0;
    
    // Fullscreen quad program
    GLuint quadProgram = 0;
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;
    
    float time = 0.0f;
};

AppState app;
Starship ship;

// Convert browser coords to normalized (-1 to 1)
void browserToNormalized(float browserX, float browserY, float& outX, float& outY) {
    outX = (browserX / app.width) * 2.0f - 1.0f;
    outY = 1.0f - (browserY / app.height) * 2.0f;
}

EM_BOOL onMouseDown(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    float x, y;
    browserToNormalized(e->targetX, e->targetY, x, y);
    ship.onMouseDown(e->button, x, y);
    return EM_TRUE;
}

EM_BOOL onMouseUp(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    float x, y;
    browserToNormalized(e->targetX, e->targetY, x, y);
    ship.onMouseUp(e->button, x, y);
    return EM_TRUE;
}

EM_BOOL onMouseMove(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    float x, y;
    browserToNormalized(e->targetX, e->targetY, x, y);
    ship.onMouseMove(x, y);
    return EM_TRUE;
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("Shader compilation error: %s\n", infoLog);
    }
    return shader;
}

GLuint createProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        printf("Program linking error: %s\n", infoLog);
    }
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    return program;
}

void initFBO() {
    int samples = 4;  // 4x MSAA
    
    // Multisampled renderbuffer for color
    GLuint msaaRBO;
    glGenRenderbuffers(1, &msaaRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, msaaRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, app.width, app.height);
    
    // Multisampled renderbuffer for depth
    glGenRenderbuffers(1, &app.rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, app.rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT16, app.width, app.height);
    
    // Multisampled FBO
    glGenFramebuffers(1, &app.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, app.fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaRBO);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, app.rbo);
    
    // Regular texture for resolved output
    glGenTextures(1, &app.fboTexture);
    glBindTexture(GL_TEXTURE_2D, app.fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, app.width, app.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Resolve FBO (non-multisampled, for blitting to)
    GLuint resolveFBO;
    glGenFramebuffers(1, &resolveFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app.fboTexture, 0);
    
    // Store for later
    app.resolveFBO = resolveFBO;
    app.msaaRBO = msaaRBO;
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initQuad() {
    app.quadProgram = createProgram(quadVertexShaderSrc, quadFragmentShaderSrc);
    
    // Fullscreen quad vertices: position (x, y), texcoord (u, v)
    float vertices[] = {
        // Position      // TexCoord
        -1.0f,  1.0f,    0.0f, 1.0f,  // Top Left
        -1.0f, -1.0f,    0.0f, 0.0f,  // Bottom Left
         1.0f, -1.0f,    1.0f, 0.0f,  // Bottom Right
        
        -1.0f,  1.0f,    0.0f, 1.0f,  // Top Left
         1.0f, -1.0f,    1.0f, 0.0f,  // Bottom Right
         1.0f,  1.0f,    1.0f, 1.0f   // Top Right
    };
    
    glGenVertexArrays(1, &app.quadVAO);
    glGenBuffers(1, &app.quadVBO);
    
    glBindVertexArray(app.quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, app.quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

int f = 0;

void renderToFBO() {
    glBindFramebuffer(GL_FRAMEBUFFER, app.fbo);
    glViewport(0, 0, app.width, app.height);
    
    // Clear with a dark color
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ship.drawGrid();
    ship.drawCells();

    int aliveCount = 0;
    std::vector<glm::vec2> positions;
    std::vector<int> drawnIndex;
    

    static int loopCount = 0;
    if(f % 60 == 0) {
        loopCount += 1;
    }
    
    for(int i = 139; i < 139 + loopCount; ++i) {
        // ship.renderCannons(ship.cells[i].middleOfTriangle.x, ship.cells[i].middleOfTriangle.y);
    }

    for(int i = 0; i < ship.cells.size(); ++i) {
        if(ship.cells[i].cellAlive) {
            printf("alive i=%d\n", i);

            aliveCount++;
            ship.renderCannons(ship.cells[i].middleOfTriangle.x, ship.cells[i].middleOfTriangle.y);
            positions.push_back(ship.cells[i].middleOfTriangle);
            drawnIndex.push_back(i);
        }
    }

    //for(int i = 139; i < 162; ++i) {
    //     ship.renderCannons(ship.cells[i].middleOfTriangle.x, ship.cells[i].middleOfTriangle.y);
    //}

    //ship.renderCannons(ship.cells[139 - 81].middleOfTriangle.x, ship.cells[139 - 81].middleOfTriangle.y);
    // ship.renderCannons(ship.cells[140 - 81].middleOfTriangle.x, ship.cells[140 - 81].middleOfTriangle.y);
    //ship.renderCannons(ship.cells[141 - 81].middleOfTriangle.x, ship.cells[141 - 81].middleOfTriangle.y);

    //ship.renderCannons(ship.cells[129+3 - 81].middleOfTriangle.x, ship.cells[129+3 - 81].middleOfTriangle.y);
   //  ship.renderCannons(ship.cells[130+3 - 81].middleOfTriangle.x, ship.cells[130+3 - 81].middleOfTriangle.y);
   // ship.renderCannons(ship.cells[131+3 - 81].middleOfTriangle.x, ship.cells[131+3 - 81].middleOfTriangle.y);


    for(int i = 40 + 20; i < 60 + 20; ++i) {
      //  ship.renderCannons(ship.cells[i].middleOfTriangle.x, ship.cells[i].middleOfTriangle.y);
    }


    f++;
    if(f % 200 == 0) {
        printf("ship.cells.size(): %d\n", ship.cells.size());
        printf("aliveCount: %d\n", aliveCount);

        for(int i = 0; i < drawnIndex.size(); ++i) {
            printf("drawnIndex[%d] = %d\n", i, drawnIndex[i]);
            printf("position[%d]: x=%f, y=%f\n", i, positions[i].x, positions[i].y);
        }
    }


    glBindFramebuffer(GL_READ_FRAMEBUFFER, app.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, app.resolveFBO);
    glBlitFramebuffer(0, 0, app.width, app.height, 0, 0, app.width, app.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderToScreen() {
    glViewport(0, 0, app.width, app.height);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Draw fullscreen quad with FBO texture
    glUseProgram(app.quadProgram);
    glBindVertexArray(app.quadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app.fboTexture);
    glUniform1i(glGetUniformLocation(app.quadProgram, "uTexture"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void mainLoop() {
    app.time += 0.016f;
    
    // Render triangle to FBO
    renderToFBO();
    
    // Display FBO on screen
    renderToScreen();
}

EM_BOOL onResize(int eventType, const EmscriptenUiEvent* e, void* userData) {
    app.width = e->windowInnerWidth;
    app.height = e->windowInnerHeight;
    emscripten_set_canvas_element_size("#canvas", app.width, app.height);
    glViewport(0, 0, app.width, app.height);
    
    // Update projection matrix
    float aspect = (float)app.width / (float)app.height;
    projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    
    // Recreate FBO at new size
    if (app.fbo) glDeleteFramebuffers(1, &app.fbo);
    if (app.fboTexture) glDeleteTextures(1, &app.fboTexture);
    if (app.rbo) glDeleteRenderbuffers(1, &app.rbo);
    if (app.resolveFBO) glDeleteFramebuffers(1, &app.resolveFBO);
    if (app.msaaRBO) glDeleteRenderbuffers(1, &app.msaaRBO);
    initFBO();
    
    return EM_TRUE;
}

int main() {
    // Set size FIRST
    app.width = EM_ASM_INT({ 
        var canvas = document.getElementById('canvas');
        canvas.width = canvas.clientWidth;
        canvas.height = canvas.clientHeight;
        return canvas.width;
    });
    app.height = EM_ASM_INT({ 
        return document.getElementById('canvas').height;
    });
    emscripten_set_canvas_element_size("#canvas", app.width, app.height);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, onResize);

    // Orthographic projection that corrects aspect ratio
    // This maps (-aspect, -1) to (aspect, 1) in world space to (-1, -1) to (1, 1) in clip space
    float aspect = (float)app.width / (float)app.height;
    projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

    // Create WebGL2 context
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.alpha = false;
    attrs.depth = true;
    attrs.stencil = false;
    attrs.antialias = true;
    attrs.premultipliedAlpha = true;
    attrs.preserveDrawingBuffer = false;
    
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attrs);
    if (ctx <= 0) {
        printf("Failed to create WebGL2 context\n");
        return -1;
    }
    emscripten_webgl_make_context_current(ctx);
    
    printf("WebGL2 context created successfully\n");
    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
    
    // Initialize resources
    initFBO();
    initQuad();
    
    printf("Initialization complete. Starting render loop...\n");
    
    // prevent right click from popping pop up
    EM_ASM({
    document.getElementById('canvas').addEventListener('contextmenu', function(e) {
        e.preventDefault();
        });
    });

    ship.setAspect((float)app.width / (float)app.height);
    ship.initStarshipCells();
    ship.initCellMiddlePoints();
    ship.initGrid();
    ship.initCellRendering();
    ship.initCannons();

    /*ship.newAttackCell(Starship::CELL_ICE, 1);
    ship.newAttackCell(Starship::CELL_ICE, 2);
    ship.newAttackCell(Starship::CELL_FIRE, 3);
    ship.newAttackCell(Starship::CELL_RADIOACTIVE, 4);

    ship.newAttackCell(Starship::CELL_ICE, 6);
    ship.newAttackCell(Starship::CELL_ICE, 7);

    ship.newAttackCell(Starship::CELL_ICE, 21);
    ship.newAttackCell(Starship::CELL_ICE, 22);
    ship.newAttackCell(Starship::CELL_ICE, 23);
    ship.newAttackCell(Starship::CELL_RADIOACTIVE, 24);
    ship.newAttackCell(Starship::CELL_RADIOACTIVE, 25);
    ship.newAttackCell(Starship::CELL_RADIOACTIVE, 26);
    ship.newAttackCell(Starship::CELL_RADIOACTIVE, 27);
    ship.newAttackCell(Starship::CELL_RADIOACTIVE, 28);*/

    for(int i = 29; i < 34; ++i) {
            ship.newAttackCell(Starship::CELL_FIRE, i);
    }

    for(int i = 55; i < 60; ++i) {
            ship.newAttackCell(Starship::CELL_FIRE, i);
    }

     for(int i = 60; i < 104; ++i) {
            ship.newAttackCell(Starship::CELL_FIRE, i);
    }

         for(int i = 139; i < 164; ++i) {
            ship.newAttackCell(Starship::CELL_FIRE, i);
    }


    // Register mouse events
    emscripten_set_mousedown_callback("#canvas", nullptr, EM_TRUE, onMouseDown);
    emscripten_set_mouseup_callback("#canvas", nullptr, EM_TRUE, onMouseUp);
    emscripten_set_mousemove_callback("#canvas", nullptr, EM_TRUE, onMouseMove);

    // Start main loop
    emscripten_set_main_loop(mainLoop, 0, 1);
    
    return 0;
}
