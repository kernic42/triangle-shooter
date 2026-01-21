#pragma once
#include "stbImage/stb_image.h"
#include <fstream>
#include <sstream>
#include <string>
const int MAX_CANNON_COUNT = 9*9 * 2;

// Grid settings
constexpr int gridWidth = 9;
constexpr int gridHeight = 9;
constexpr float cellSize = 0.120;
constexpr float originX = -(gridWidth*cellSize)/2.0;
constexpr float originY = -(gridHeight*cellSize)/2.0; // maybe set these field from within starship class, into renderer? since its supposed to hold the truth of the cells layout, and the renderer renders

// maybe should share this in .h file
typedef struct {
    float cannonRot;                        // dynamic each frame
    glm::vec2 pivot;                        // static
    glm::vec2 size;                         // static
    glm::vec2 pos[MAX_CANNON_COUNT];        // static
    float colors[MAX_CANNON_COUNT]; // should be an enum
    int count = 0;
} cannonData_t;

typedef struct {
    glm::mat4 model[MAX_CANNON_COUNT];       // static
    glm::mat3x2 texCoords[MAX_CANNON_COUNT]; // static
    glm::vec4 colors[MAX_CANNON_COUNT];      // static
    int count = 0;
} cellHullData_t;

typedef struct {
    bool configChanged = false;
    int shipID;                              // static
    float shipRot;                           // dynamic each frame
    cannonData_t cannonData;
    cellHullData_t cellHullData;
} shipData_t;

////////////////////////////////////////////////////////

 enum CellCategory {
    CELL_ATTACK,
    CELL_DEFENSE,
    CELL_UTILITY,
    CELL_JET,
    CELL_CUSTOM
};

enum CellName {
    // Attack
    CELL_FIRE,
    CELL_ICE,
    CELL_RADIOACTIVE,
    CELL_PROJECTILE_GUN,
    CELL_LASER_GUN,
    CELL_MISSILE_GUN,
    CELL_PLASMA_GUN,
    CELL_RAPID_FIRE_PROJECTILE,

    // Defense
    CELL_KINETIC_BARRIER,
    CELL_ENERGY_SHIELD,
    CELL_HYBRID_SHIELD,
    CELL_REFLECTIVE_SHIELD,
    CELL_REGEN_SHIELD,
    CELL_SPIKE_ARMOR,
    CELL_CLOAKING_FIELD,
    CELL_FORCE_BUBBLE,

    // Utility
    CELL_SENSOR,
    CELL_REPAIR_DRONE,
    CELL_SCANNER,
    CELL_JAMMER,
    CELL_CARGO_HOLD,
    CELL_BATTERY,
    CELL_ANALYZER,
    CELL_ENERGY_CORE,

    // Jet
    CELL_FORWARD_THRUST_JET,
    CELL_OMNI_BOOST_JET,
    CELL_TURN_JET,
    CELL_BURST_JET,
    CELL_EFFICIENCY_JET,
    CELL_OVERDRIVE_JET,
    CELL_STABILIZER_JET,

    // Custom
    CELL_HOMING_MISSILE,
    CELL_AREA_DENIAL_MINE,
    CELL_STEAM_LASER,
    CELL_SWITCH_BLASTER,
    CELL_NONE
};

struct CellTexCoords {
    float u0, v0;  // bottom-left
    float u1, v1;  // bottom-right
    float u2, v2;  // top-right
    float pad0, pad1;  // padding to align to 32 bytes (std140)
};

enum AtlasSprite {
    ATLAS_FIRE = 0,
    ATLAS_ICE = 1,
    ATLAS_RADIOACTIVE = 2,
};

// Shader with rotation matrix
inline const char* gridVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;
uniform mat4 uProjection;
uniform mat3 uRotation;
void main() {
    vec3 rotated = uRotation * vec3(aPos, 1.0);
    gl_Position = uProjection * vec4(rotated.xy, 0.0, 1.0);
}
)";

inline const char* gridFragmentShader = R"(#version 300 es
precision mediump float;
out vec4 fragColor;
void main() {
    fragColor = vec4(0.3, 0.8, 0.3, 1.0);
}
)";

inline const char* cursorCellVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;

uniform mat4 uTransforms[256];
uniform vec2 uTexCoords[768];
uniform vec4 uColors[256];

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

inline const char* cellVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;

uniform mat4 uTransforms[256];
uniform vec2 uTexCoords[768];
uniform vec4 uColors[256];

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

inline const char* cellFragmentShader = R"(#version 300 es
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

inline std::string loadTextFile(const char* path) {
    FILE* f = fopen(path, "rb");  // try binary mode
    if (!f) {
        printf("Failed to open: %s (errno: %d - %s)\n", path, errno, strerror(errno));
        return {};
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    std::string content(size, '\0');
    fread(&content[0], 1, size, f);
    fclose(f);
    
    return content;
}

inline GLuint loadTexture(const char* path) {
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

inline GLuint loadTextureBlurry(const char* path) {
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

inline GLuint loadTextureRepeat(const char* path) {
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