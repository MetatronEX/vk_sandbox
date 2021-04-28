if not exist "%cd%/sandbox/shader" mkdir "%cd%/sandbox/shader"

%VULKAN_SDK%/Bin/glslc sandbox/shader/shader.vert -o shader/vert.spv
%VULKAN_SDK%/Bin/glslc sandbox/shader/shader.frag -o shader/frag.spv