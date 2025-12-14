@echo off
cd /d %~dp0
call C:\Users\Nicolas\emsdk\emsdk_env.bat
emcc -std=c++17 -O2 ^
 -s USE_WEBGL2=1 ^
 -s FULL_ES3=1 ^
 -s WASM=1 ^
 -s ALLOW_MEMORY_GROWTH=1 ^
 -s USE_FREETYPE=1 ^
 -I. ^
 -I./glm ^
 -I./button ^
 -I./stbImage ^
 -I./lineRenderer ^
 -I./textRenderer ^
 -I./renderer2d ^
 --preload-file atlas.png ^
 --preload-file background_tile.png ^
 --preload-file crack_mask.png ^
 --preload-file cannon.png ^
 --preload-file assets/fonts/roboto/Roboto-Medium.ttf@fonts/Roboto-Medium.ttf ^
 main.cpp ^
 starship/starship.cpp ^
 lineRenderer/lineRenderer.cpp ^
 textRenderer/textRenderer.cpp ^
 renderer2d/renderer2d.cpp ^
 button/button.cpp ^
 -o main.js
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)
echo Build complete!
pause