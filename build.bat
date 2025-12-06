@echo off
cd /d %~dp0
call C:\Users\Nicolas\emsdk\emsdk_env.bat
emcc -std=c++17 -O2 -s USE_WEBGL2=1 -s FULL_ES3=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -I. -I./glm --preload-file atlas.png --preload-file crack_mask.png --preload-file cannon2.png --preload-file cannon.png main.cpp starship/starship.cpp -o main.js
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)
echo Build complete!
pause