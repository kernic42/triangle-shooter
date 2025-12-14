#include "starship/starship.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stbImage/stb_image.h"
#include "lineRenderer/lineRenderer.h"
#include "textRenderer/textRenderer.h"
#include "button/button.h"
#include "renderer2d/renderer2d.h"

TextRenderer textRenderer;
LineRenderer lineRenderer;
Renderer2D renderer2d;
ButtonManager buttonManager;
float g_aspect = 0;
glm::mat4 projection;
GLuint backgroundTexture = -1; 

const char* quadVertexShaderSrc = R"(#version 300 es
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aSceneUV;
layout(location = 2) in vec2 aTileUV;

out vec2 vSceneUV;
out vec2 vTileUV;

void main() {
    vSceneUV = aSceneUV;
    vTileUV  = aTileUV;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)";

 const char* quadFragmentShaderSrc = R"(#version 300 es
precision mediump float;

in vec2 vSceneUV;
in vec2 vTileUV;

out vec4 fragColor;

uniform sampler2D uSceneTexture;   // FBO texture
uniform sampler2D uTileTexture;    // Repeating background

void main() {
    vec4 bg = texture(uTileTexture, vTileUV);
    vec4 scene = texture(uSceneTexture, vSceneUV);

    fragColor = bg * (1.0 - scene.a) + scene;
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
    // Pixel coords for UI (Y flipped for OpenGL)
    float pixelX = e->targetX;
    float pixelY = app.height - e->targetY;
    
    if (e->button == 0) {
        buttonManager.fingerStart(pixelX, pixelY);
    }
    
    // Normalized coords for your ship
    float x, y;
    browserToNormalized(e->targetX, e->targetY, x, y);
    ship.onMouseDown(e->button, x, y);
    
    return EM_TRUE;
}

EM_BOOL onMouseUp(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    float pixelX = e->targetX;
    float pixelY = app.height - e->targetY;
    
    if (e->button == 0) {
        buttonManager.fingerRelease(pixelX, pixelY);
    }
    
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

void updateFBOTextureUV() {
    glUseProgram(app.quadProgram);

    float tileScale = 0.5;
    float tileWidth = 208.0 * tileScale;
    float tileHeight = 138.0 * tileScale;

    float tileCountX = (float)app.width / tileWidth;   // how many cells fit over width
    float tileCountY = (float)app.height / tileHeight; // how many cells fit over height
    
    float offsetX = -fmod(tileCountX, 1.0) / 2.0 +        // offset by the fraction that cell takes
                    ((int)floor(tileCountX)+1) % 2 * 0.5; // cell count that fits is even(without remainder), then do + half offset

    float offsetY = -fmod(tileCountY, 1.0) / 2.0 +        // offset by the fraction that cell takes
                    ((int)floor(tileCountY)+1) % 2 * 0.5; // cell count that fits is even(without remainder), then do + half offset

    float vertices[] = {
        // Position      // SceneUv    // TileUv
        -1.0f,  1.0f,    0.0f, 1.0f,   offsetX,              offsetY + tileCountY, // Top Left
        -1.0f, -1.0f,    0.0f, 0.0f,   offsetX,              offsetY,              // Bottom Left
         1.0f, -1.0f,    1.0f, 0.0f,   offsetX + tileCountX, offsetY,              // Bottom Right
        
        -1.0f,  1.0f,    0.0f, 1.0f,   offsetX,              offsetY + tileCountY, // Top Left
         1.0f, -1.0f,    1.0f, 0.0f,   offsetX + tileCountX, offsetY,              // Bottom Right
         1.0f,  1.0f,    1.0f, 1.0f,   offsetX + tileCountX, offsetY + tileCountY  // Top Right
    };
    
    glBindVertexArray(app.quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, app.quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /*    float imgWidth = 208;
    float imgHeight = 138;
    float aspect = imgWidth / imgHeight;

    float height = 1.0 * aspect;
    float width = 1.0  * g_aspect;
    //printf("g_aspect: %f\n", g_aspect);
    printf("aspect: %f\n", aspect);

    float multiplyFactor = 11.0f;

    float tileHeight = height * multiplyFactor;
    float tileWidth = width * multiplyFactor;*/
}

EM_BOOL onResize(int eventType, const EmscriptenUiEvent* e, void* userData) {
    app.width = e->windowInnerWidth;
    app.height = e->windowInnerHeight;
    emscripten_set_canvas_element_size("#canvas", app.width, app.height);
    glViewport(0, 0, app.width, app.height);
    
    // Update projection matrix
    g_aspect = (float)app.width / (float)app.height;
    projection = glm::ortho(-g_aspect, g_aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    
    // Recreate FBO at new size
    if (app.fbo) glDeleteFramebuffers(1, &app.fbo);
    if (app.fboTexture) glDeleteTextures(1, &app.fboTexture);
    if (app.rbo) glDeleteRenderbuffers(1, &app.rbo);
    if (app.resolveFBO) glDeleteFramebuffers(1, &app.resolveFBO);
    if (app.msaaRBO) glDeleteRenderbuffers(1, &app.msaaRBO);
    initFBO();

    updateFBOTextureUV();
    
    return EM_TRUE;
}

void initQuad() {
    app.quadProgram = createProgram(quadVertexShaderSrc, quadFragmentShaderSrc);

    glGenVertexArrays(1, &app.quadVAO);
    glGenBuffers(1, &app.quadVBO);

    glBindVertexArray(app.quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, app.quadVBO);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // SceneUv attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // TileUv attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);

    updateFBOTextureUV();
}

int f = 0;

void renderToFBO() {
    glBindFramebuffer(GL_FRAMEBUFFER, app.fbo);
    glViewport(0, 0, app.width, app.height);

    // Clear with a dark color
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // alpha = 0
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ship.drawGrid();
    ship.drawCells();
    ship.renderCannons();

    lineRenderer.draw(glm::vec2(0.0, 0.0), glm::vec2(0.5, 0.5), glm::vec4(1.0, 1.0, 0.0, 1.0), 0.05);
    lineRenderer.draw(glm::vec2(0.0, 0.0), glm::vec2(-0.5, 0.5), glm::vec4(1.0, 0.0, 0.0, 1.0), 0.02);
    lineRenderer.flush(glm::value_ptr(projection), 0.0f);

    textRenderer.draw("Hello World", 100.0f, 500.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    textRenderer.drawCentered("Centered Text", 640.0f, 360.0f, 0.5f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    textRenderer.flush();

    buttonManager.drawButtons();

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
    glUniform1i(glGetUniformLocation(app.quadProgram, "uSceneTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glUniform1i(glGetUniformLocation(app.quadProgram, "uTileTexture"), 1);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void mainLoop() {
    app.time += 0.016f;
    
    // Render triangle to FBO
    renderToFBO();
    
    // Display FBO on screen
    renderToScreen();
}

GLuint static loadTexture(const char* path) {
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(data);
    
    printf("Loaded texture: %s (%dx%d)\n", path, width, height);
    return texture;
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
    g_aspect = (float)app.width / (float)app.height;
    projection = glm::ortho(-g_aspect, g_aspect, -1.0f, 1.0f, -1.0f, 1.0f);
g_aspect = (float)app.width / (float)app.height;
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
    
    backgroundTexture = loadTexture("background_tile.png");

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
    lineRenderer.init();
    textRenderer.initialize("fonts/Roboto-Medium.ttf", (float)app.width, (float)app.height);
    
    renderer2d.init();
    renderer2d.setScreenSize( (float)app.width, (float)app.height);
    buttonManager.init(&textRenderer, &renderer2d);

    // Create button
    Button config;
    config.x = 100;
    config.y = 100;
    config.width = 200;
    config.height = 50;
    config.text = "Click Me";
    config.textScale = 0.5f;
    config.color = glm::vec4(0.2f, 0.5f, 0.8f, 1.0f);
    config.borderRadius = 10.0f;
    config.borderColor = glm::vec4(0.0, 0.0, 0.0, 1.0); // grey
    config.borderWidth = 1.0;

    Button* myButton = buttonManager.createButton(config);
    buttonManager.setCallback(myButton, [](Button* btn) {
        static bool toggle = false;
        toggle = !toggle;
        
        if (toggle) {
            btn->color = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);  // red
        } else {
            btn->color = glm::vec4(0.2f, 0.5f, 0.8f, 1.0f);  // blue
        }
    });


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
            ship.newAttackCell(Starship::CELL_RADIOACTIVE, i);
    }

    ship.initCannons();


    // Register mouse events
    emscripten_set_mousedown_callback("#canvas", nullptr, EM_TRUE, onMouseDown);
    emscripten_set_mouseup_callback("#canvas", nullptr, EM_TRUE, onMouseUp);
    emscripten_set_mousemove_callback("#canvas", nullptr, EM_TRUE, onMouseMove);

    // Start main loop
    emscripten_set_main_loop(mainLoop, 0, 1);
    
    return 0;
}
