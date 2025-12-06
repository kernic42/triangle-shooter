cmake_minimum_required(VERSION 3.10)
project(webgl_fbo_template)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    main.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Emscripten-specific settings
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    
    # Emscripten link flags
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "\
            -s USE_WEBGL2=1 \
            -s FULL_ES3=1 \
            -s WASM=1 \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s NO_EXIT_RUNTIME=1 \
            -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
            --shell-file ${CMAKE_SOURCE_DIR}/index.html \
        "
    )
endif()
