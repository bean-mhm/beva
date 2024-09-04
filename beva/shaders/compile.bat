@echo off

echo compiling shaders

set GLSLC_PATH="C:/Program Files/VulkanSDK/1.3.250.1/Bin/glslc.exe"

if not exist "%VS_OUT_DIR%shaders/" mkdir "%VS_OUT_DIR%shaders/"

%GLSLC_PATH% -fshader-stage=vertex "%VS_PROJ_DIR%shaders/demo_01_vert.glsl" -o "%VS_OUT_DIR%shaders/demo_01_vert.spv"
%GLSLC_PATH% -fshader-stage=fragment "%VS_PROJ_DIR%shaders/demo_01_frag.glsl" -o "%VS_OUT_DIR%shaders/demo_01_frag.spv"

%GLSLC_PATH% -fshader-stage=vertex "%VS_PROJ_DIR%shaders/demo_02_vert.glsl" -o "%VS_OUT_DIR%shaders/demo_02_vert.spv"
%GLSLC_PATH% -fshader-stage=fragment "%VS_PROJ_DIR%shaders/demo_02_frag.glsl" -o "%VS_OUT_DIR%shaders/demo_02_frag.spv"

echo finished compiling shaders

pause>nul
