if not exist "%cd%/sandbox/shader" mkdir "%cd%/sandbox/shader"

%VULKAN_SDK%/Bin/glslc shader/shader.vert -o sandbox/shader/vert.spv
%VULKAN_SDK%/Bin/glslc shader/shader.frag -o sandbox/shader/frag.spv