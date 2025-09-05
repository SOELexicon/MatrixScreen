cmake_minimum_required(VERSION 3.20)
project(MatrixScreensaver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows-specific settings
if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE -D_UNICODE)
endif()

# DirectX libraries (Windows SDK)
if(WIN32)
    set(DIRECTX_LIBS d3d11 dxgi d2d1 dwrite windowscodecs)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/matrix_screensaver.cpp
    src/matrix_renderer.cpp
    src/config_dialog.cpp
    src/settings_manager.cpp
    src/mask_loader.cpp
    src/MatrixScreensaver.rc
)

set(HEADERS
    src/matrix_screensaver.h
    src/matrix_renderer.h
    src/config_dialog.h
    src/settings_manager.h
    src/mask_loader.h
    src/common.h
    src/resource.h
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Link libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} 
        ${DIRECTX_LIBS}
        comctl32
        gdi32
        user32
        kernel32
        ole32
        oleaut32
        uuid
        comdlg32
    )
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W3 /permissive- /Zc:__cplusplus
    )
endif()

# Set output name to .scr for screensaver
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "MatrixScreensaver")

# Post-build: rename to .scr
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
    "$<TARGET_FILE:${PROJECT_NAME}>" 
    "${CMAKE_BINARY_DIR}/MatrixScreensaver.scr"
)