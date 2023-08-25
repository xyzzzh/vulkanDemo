@echo off
setlocal

:: 当前脚本所在目录
set "SCRIPT_DIR=%~dp0"

:: 设置Vulkan SDK路径为环境变量，或者你可以直接将其添加到系统环境变量中
set "VULKAN_SDK=C:/VulkanSDK/1.3.250.1"
:: 路径正规化，去除最后的斜杠（如果存在）
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
"%VULKAN_SDK%\Bin\glslc.exe" "%SCRIPT_DIR%/../shaders/triangle.vert" -o "%SCRIPT_DIR%/../shaders/bin/triangle.vert.spv"
"%VULKAN_SDK%\Bin\glslc.exe" "%SCRIPT_DIR%/../shaders/triangle.frag" -o "%SCRIPT_DIR%/../shaders/bin/triangle.frag.spv"
pause
