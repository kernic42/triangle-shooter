# WebGL FBO Triangle Template

A basic C++ WebGL2 template using Emscripten that demonstrates rendering a triangle to a Framebuffer Object (FBO) and then displaying it on screen.

## Features

- WebGL2 / OpenGL ES 3.0 rendering
- Framebuffer Object (FBO) with color texture and depth renderbuffer
- Colored triangle rendered to FBO
- Fullscreen quad to display FBO texture
- Clean, well-structured C++ code

## Architecture

```
┌─────────────────────────────────────────┐
│           Render Pass 1 (FBO)           │
│  ┌─────────────────────────────────┐    │
│  │      Triangle Shader            │    │
│  │  - Vertex colors (RGB)          │    │
│  │  - Renders to FBO texture       │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│         Render Pass 2 (Screen)          │
│  ┌─────────────────────────────────┐    │
│  │      Fullscreen Quad Shader     │    │
│  │  - Samples FBO texture          │    │
│  │  - Renders to default FB        │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
```

## Prerequisites

Install the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```bash
# Clone emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest SDK
./emsdk install latest
./emsdk activate latest

# Set up environment (run this in each terminal session)
source ./emsdk_env.sh
```

## Building

### Using Make (Recommended)

```bash
# Build the project
make

# Build with debug symbols
make debug

# Clean build artifacts
make clean
```

### Using CMake

```bash
mkdir build && cd build
emcmake cmake ..
emmake make
```

### Direct Compilation

```bash
emcc -std=c++17 -O2 \
    -s USE_WEBGL2=1 \
    -s FULL_ES3=1 \
    -s WASM=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    main.cpp -o main.js
```

## Running

WebGL requires a local server (browsers block WebAssembly from `file://` URLs).

```bash
# Using Python
python3 -m http.server 8000

# Using Node.js
npx http-server -p 8080

# Or use the Makefile target
make serve
```

Then open `http://localhost:8000` in your browser.

## File Structure

```
webgl_fbo_template/
├── main.cpp          # Main C++ source with WebGL code
├── index.html        # HTML page with canvas
├── Makefile          # Build configuration
├── CMakeLists.txt    # CMake configuration
└── README.md         # This file
```

## Code Overview

### main.cpp

| Function | Description |
|----------|-------------|
| `compileShader()` | Compiles a GLSL shader |
| `createProgram()` | Links vertex and fragment shaders |
| `initFBO()` | Creates framebuffer with color texture and depth buffer |
| `initTriangle()` | Sets up triangle VAO/VBO with position and color attributes |
| `initQuad()` | Sets up fullscreen quad for displaying FBO texture |
| `renderToFBO()` | Renders the triangle to the framebuffer |
| `renderToScreen()` | Displays FBO texture via fullscreen quad |
| `mainLoop()` | Main render loop called by Emscripten |

### Shaders

**Triangle Shaders:**
- Vertex: Passes through position and interpolates vertex colors
- Fragment: Outputs interpolated color

**Quad Shaders:**
- Vertex: Passes through position and texture coordinates
- Fragment: Samples and outputs FBO texture

## Extending This Template

Common modifications:

1. **Post-processing**: Modify `quadFragmentShaderSrc` to add effects
2. **Multiple render passes**: Create additional FBOs and ping-pong between them
3. **Animation**: Use `app.time` in shaders via uniforms
4. **Input handling**: Use Emscripten's input APIs
5. **Multiple triangles/objects**: Add more geometry and draw calls

## License

Public domain / MIT - use freely for any purpose.
